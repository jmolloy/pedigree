/*
 * Copyright (c) 2010 Matthew Iselin
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <Log.h>
#include <Module.h>
#include <processor/types.h>
#include <processor/Processor.h>
#include <processor/MemoryMappedIo.h>
#include <processor/PhysicalMemoryManager.h>
#include <processor/VirtualAddressSpace.h>
#include <utilities/StaticString.h>
#include <processor/MemoryRegion.h>
#include <utilities/List.h>
#include <machine/Machine.h>
#include <machine/Display.h>
#include <config/Config.h>

#include <machine/Framebuffer.h>
#include <graphics/Graphics.h>
#include <graphics/GraphicsService.h>

#include "vm_device_version.h"
#include "svga_reg.h"

#define DEBUG_VMWARE_GFX        0

class VmwareFramebuffer;

static struct mode
{
    size_t id;
    size_t width;
    size_t height;
    Graphics::PixelFormat fmt;
} g_VbeIndexedModes[] = {
    {0x117, 1024, 768, Graphics::Bits16_Rgb555}, // Format is used only for byte count
    {0, 80, 25, Graphics::Bits8_Idx} /// Mode zero = "disable"
};

#define SUPPORTED_MODES_SIZE (sizeof(g_VbeIndexedModes) / sizeof(g_VbeIndexedModes[0]))

// Can refer to http://sourceware.org/ml/ecos-devel/2006-10/msg00008/README.xfree86
// for information about programming this device, as well as the Haiku driver.

class VmwareGraphics : public Display
{
    friend class VmwareFramebuffer;
    
    public:
        VmwareGraphics(Device *pDev) :
            Display(pDev), m_pIo(0),
            m_Framebuffer(0), m_CommandRegion(0),
            m_pFramebufferRawAddress(0)
        {
            m_pIo = m_Addresses[0]->m_Io;
            
            // Support ID2, as we are expecting an SVGA2 functional device
            writeRegister(SVGA_REG_ID, SVGA_MAKE_ID(2));
            if(readRegister(SVGA_REG_ID) != SVGA_MAKE_ID(2))
            {
                WARNING("vmware-gfx not a compatible SVGA device");
                return;
            }
            
            // Read the framebuffer base (should match BAR1)
            uintptr_t fbBase = readRegister(SVGA_REG_FB_START);
            size_t fbSize = readRegister(SVGA_REG_VRAM_SIZE);
            
            // Read the command FIFO (should match BAR2)
            uintptr_t cmdBase = readRegister(SVGA_REG_MEM_START);
            size_t cmdSize = readRegister(SVGA_REG_MEM_SIZE);
            
            // Find the capabilities of the device
            size_t caps = readRegister(SVGA_REG_CAPABILITIES);
            
            // Read maximum resolution
            size_t maxWidth = readRegister(SVGA_REG_MAX_WIDTH);
            size_t maxHeight = readRegister(SVGA_REG_MAX_HEIGHT);
            
            // Tell VMware what OS we are - "Other"
            writeRegister(SVGA_REG_GUEST_ID, 0x500A);
            
            // Debug notification
            NOTICE("vmware-gfx found, caps=" << caps << ", maximum resolution is " << Dec << maxWidth << "x" << maxHeight << Hex);
            NOTICE("vmware-gfx        framebuffer at " << fbBase << " - " << (fbBase + fbSize) << ", command FIFO at " << cmdBase);
            
            if(m_Addresses[1]->m_Address == fbBase)
            {
                m_Framebuffer = static_cast<MemoryMappedIo*>(m_Addresses[1]->m_Io);
                m_CommandRegion = static_cast<MemoryMappedIo*>(m_Addresses[2]->m_Io);
                m_Addresses[2]->map();
                
                m_pFramebufferRawAddress = m_Addresses[1];
            }
            else
            {
                m_Framebuffer = static_cast<MemoryMappedIo*>(m_Addresses[2]->m_Io);
                m_CommandRegion = static_cast<MemoryMappedIo*>(m_Addresses[1]->m_Io);
                m_Addresses[1]->map();
                
                m_pFramebufferRawAddress = m_Addresses[2];
            }
            
            // Disable the command FIFO in case it was already enabled
            writeRegister(SVGA_REG_CONFIG_DONE, 0);
            
            // Enable the SVGA now
            writeRegister(SVGA_REG_ENABLE, 1);
            
            // Initialise the FIFO
            volatile uint32_t *fifo = reinterpret_cast<volatile uint32_t*>(m_CommandRegion->virtualAddress());
            
            if(caps & SVGA_CAP_EXTENDED_FIFO)
            {
                size_t numFifoRegs = readRegister(SVGA_REG_MEM_REGS);
                size_t fifoExtendedBase = numFifoRegs * 4;
            
                fifo[SVGA_FIFO_MIN] = fifoExtendedBase; // Start right after this information block
                fifo[SVGA_FIFO_MAX] = cmdSize & ~0x3; // Permit the full FIFO to be used
                fifo[SVGA_FIFO_NEXT_CMD] = fifo[SVGA_FIFO_STOP] = fifo[SVGA_FIFO_MIN]; // Empty FIFO
                
                NOTICE("vmware-gfx using extended fifo, caps=" << fifo[SVGA_FIFO_CAPABILITIES] << ", flags=" << fifo[SVGA_FIFO_FLAGS]);
            }
            else
            {
                fifo[SVGA_FIFO_MIN] = 16; // Start right after this information block
                fifo[SVGA_FIFO_MAX] = cmdSize & ~0x3; // Permit the full FIFO to be used
                fifo[SVGA_FIFO_NEXT_CMD] = fifo[SVGA_FIFO_STOP] = 16; // Empty FIFO
            }
            
            // Start running the FIFO
            writeRegister(SVGA_REG_CONFIG_DONE, 1);
            
            m_pFramebuffer = new VmwareFramebuffer(reinterpret_cast<uintptr_t>(m_Framebuffer->virtualAddress()), this);
            
            GraphicsService::GraphicsProvider *pProvider = new GraphicsService::GraphicsProvider;
            pProvider->pDisplay = this;
            pProvider->pFramebuffer = m_pFramebuffer;
            pProvider->maxWidth = maxWidth;
            pProvider->maxHeight = maxHeight;
            pProvider->maxDepth = 32;
            pProvider->bHardwareAccel = true;
            pProvider->bTextModes = false;
            
            // Register with the graphics service
            ServiceFeatures *pFeatures = ServiceManager::instance().enumerateOperations(String("graphics"));
            Service         *pService  = ServiceManager::instance().getService(String("graphics"));
            if(pFeatures->provides(ServiceFeatures::touch))
                if(pService)
                    pService->serve(ServiceFeatures::touch, reinterpret_cast<void*>(pProvider), sizeof(*pProvider));
        }
        
        virtual ~VmwareGraphics()
        {
        }
        
        virtual void getName(String &str)
        {
            str = "vmware-gfx";
        }

        virtual void dump(String &str)
        {
            str = "vmware guest tools, graphics card";
        }
        
        /** Returns the current screen mode.
            \return True if operation succeeded, false otherwise. */
        virtual bool getCurrentScreenMode(Display::ScreenMode &sm)
        {
            sm.width = readRegister(SVGA_REG_WIDTH);
            sm.height = readRegister(SVGA_REG_HEIGHT);
            sm.pf.nBpp = readRegister(SVGA_REG_BITS_PER_PIXEL);
            
            size_t redMask = readRegister(SVGA_REG_RED_MASK);
            size_t greenMask = readRegister(SVGA_REG_GREEN_MASK);
            size_t blueMask = readRegister(SVGA_REG_BLUE_MASK);
            
            /// \todo Not necessarily always correct for boundary cases
            switch(sm.pf.nBpp)
            {
                case 24:
                    if(redMask > blueMask)
                        sm.pf2 = Graphics::Bits24_Rgb;
                    else
                        sm.pf2 = Graphics::Bits24_Bgr;
                    break;
                case 16:
                    if((redMask == greenMask) && (greenMask == blueMask))
                    {
                        if(blueMask == 0xF)
                            sm.pf2 = Graphics::Bits16_Argb;
                        else
                            sm.pf2 = Graphics::Bits16_Rgb555;
                    }
                    else
                        sm.pf2 = Graphics::Bits16_Rgb565;
                    break;
                default:
                    sm.pf2 = Graphics::Bits32_Argb;
                    break;
            }
            
            sm.bytesPerPixel = sm.pf.nBpp / 8;
            sm.bytesPerLine = readRegister(SVGA_REG_BYTES_PER_LINE);
            
            return true;
        }
        
        /** Fills the given List with all of the available screen modes.
            \return True if operation succeeded, false otherwise. */
        virtual bool getScreenModes(List<Display::ScreenMode*> &sms)
        {
            for(size_t i = 0; i < SUPPORTED_MODES_SIZE; i++)
            {
                Display::ScreenMode *pMode = new Display::ScreenMode;
                pMode->id = g_VbeIndexedModes[i].id;
                pMode->width = g_VbeIndexedModes[i].width;
                pMode->height = g_VbeIndexedModes[i].height;
                pMode->pf2 = g_VbeIndexedModes[i].fmt;
                
                sms.pushBack(pMode);
            }
            
            return true;
        }

        /** Sets the current screen mode.
            \return True if operation succeeded, false otherwise. */
        virtual bool setScreenMode(Display::ScreenMode sm)
        {
            if(sm.id == 0)
            {
                // Disable SVGA instead of setting a mode
                writeRegister(SVGA_REG_ENABLE, 0);
            }
            else
            {
                setMode(sm.width, sm.height, Graphics::bitsPerPixel(sm.pf2));
                Machine::instance().getVga(0)->setMode(sm.id); // Remember this new mode
            }
            return true;
        }
        
        virtual bool setScreenMode(size_t nWidth, size_t nHeight, size_t nBpp)
        {
            // Read maximum resolution
            size_t maxWidth = readRegister(SVGA_REG_MAX_WIDTH);
            size_t maxHeight = readRegister(SVGA_REG_MAX_HEIGHT);
            
            // Check the passed resolution is within these boundaries: if not,
            // modify it. Applications that use framebuffers from us should use
            // their width/height methods in order to handle any potential
            // screen resolution.
            if(nWidth > maxWidth)
                nWidth = maxWidth;
            if(nHeight > maxHeight)
                nHeight = maxHeight;
            
            setMode(nWidth, nHeight, nBpp);
            return true;
        }
        
        void setMode(size_t w, size_t h, size_t bpp)
        {
            // Set mode
            writeRegister(SVGA_REG_WIDTH, w);
            writeRegister(SVGA_REG_HEIGHT, h);
            writeRegister(SVGA_REG_BITS_PER_PIXEL, bpp);
            
            size_t fbOffset = readRegister(SVGA_REG_FB_OFFSET);
            size_t width = readRegister(SVGA_REG_WIDTH);
            size_t height = readRegister(SVGA_REG_HEIGHT);
            size_t depth = readRegister(SVGA_REG_DEPTH);
            uintptr_t fbBase = readRegister(SVGA_REG_FB_START);
            size_t fbSize = readRegister(SVGA_REG_VRAM_SIZE);
            
            size_t redMask = readRegister(SVGA_REG_RED_MASK);
            size_t greenMask = readRegister(SVGA_REG_GREEN_MASK);
            size_t blueMask = readRegister(SVGA_REG_BLUE_MASK);
            
            size_t bytesPerLine = readRegister(SVGA_REG_BYTES_PER_LINE);
            size_t bytesPerPixel = bytesPerLine / w;
            
            m_pFramebuffer->setWidth(w);
            m_pFramebuffer->setHeight(h);
            m_pFramebuffer->setBytesPerPixel(bytesPerPixel);
            m_pFramebuffer->setBytesPerLine(bytesPerLine);

            Graphics::PixelFormat pf;
            switch(bpp)
            {
                case 24:
                    if(redMask > blueMask)
                        pf = Graphics::Bits24_Rgb;
                    else
                        pf = Graphics::Bits24_Bgr;
                    break;
                case 16:
                    if((redMask == greenMask) && (greenMask == blueMask))
                    {
                        if(blueMask == 0xF)
                            pf = Graphics::Bits16_Argb;
                        else
                            pf = Graphics::Bits16_Rgb555;
                    }
                    else
                        pf = Graphics::Bits16_Rgb565;
                    break;
                default:
                    pf = Graphics::Bits32_Argb;
                    break;
            }
            m_pFramebuffer->setFormat(pf);
            m_pFramebuffer->setXPos(0); m_pFramebuffer->setYPos(0);
            m_pFramebuffer->setParent(0);
            
            /// \todo If we switch to any mode after the first one has been set,
            ///       the old region needs to be deallocated.
            m_pFramebufferRawAddress->map(height * bytesPerLine, true);
            m_pFramebuffer->setFramebuffer(reinterpret_cast<uintptr_t>(m_Framebuffer->virtualAddress()));

            // Blank the framebuffer, new mode
            m_pFramebuffer->rect(0, 0, w, h, 0);
            
            NOTICE("vmware-gfx entered mode " << Dec << width << "x" << height << "x" << depth << Hex << ", mode framebuffer is " << (fbBase + fbOffset));
        }
        
        void redraw(size_t x, size_t y, size_t w, size_t h)
        {
            // Disable the command FIFO while we link the command
            writeRegister(SVGA_REG_CONFIG_DONE, 0);
            
            writeFifo(SVGA_CMD_UPDATE);
            writeFifo(x);
            writeFifo(y);
            writeFifo(w);
            writeFifo(h);
            
            // Start running the FIFO
            writeRegister(SVGA_REG_CONFIG_DONE, 1);
        }
        
        void copy(size_t x1, size_t y1, size_t x2, size_t y2, size_t w, size_t h)
        {
            // Disable the command FIFO while we link the command
            writeRegister(SVGA_REG_CONFIG_DONE, 0);
        
            writeFifo(SVGA_CMD_RECT_COPY); // RECT COPY
            writeFifo(x1); // Source X
            writeFifo(y1); // Source Y
            writeFifo(x2); // Dest X
            writeFifo(y2); // Dest Y
            writeFifo(w); // Width
            writeFifo(h); // Height
            
            // Start running the FIFO
            writeRegister(SVGA_REG_CONFIG_DONE, 1);
        }

        class VmwareFramebuffer : public Framebuffer
        {
            public:
                VmwareFramebuffer() : Framebuffer()
                {
                }
                
                VmwareFramebuffer(uintptr_t fb, VmwareGraphics *pDisplay) :
                    Framebuffer(), m_pDisplay(pDisplay)
                {
                    setFramebuffer(fb);
                }
                
                virtual ~VmwareFramebuffer()
                {
                }
                
                virtual void hwRedraw(size_t x = ~0UL, size_t y = ~0UL,
                                      size_t w = ~0UL, size_t h = ~0UL)
                {
                    if(x == ~0UL)
                        x = 0;
                    if(y == ~0UL)
                        y = 0;
                    if(w == ~0UL)
                        w = getWidth();
                    if(h == ~0UL)
                        h = getHeight();
                    m_pDisplay->redraw(x, y, w, h);
                }
                
                virtual inline void copy(size_t srcx, size_t srcy,
                                         size_t destx, size_t desty,
                                         size_t w, size_t h)
                {
                    /// \todo Caps to determine whether to fall back to software
                    if(1)
                        m_pDisplay->copy(srcx, srcy, destx, desty, w, h);
                    else
                        swCopy(srcx, srcy, destx, desty, w, h);
                }
            
            private:
            
                VmwareGraphics *m_pDisplay;
        };
        
    private:
    
        IoBase *m_pIo;
        
        /// \todo Lock these
        
        size_t readRegister(size_t offset)
        {
            m_pIo->write32(offset, SVGA_INDEX_PORT);
            return m_pIo->read32(SVGA_VALUE_PORT);
        }
        
        void writeRegister(size_t offset, uint32_t value)
        {
            m_pIo->write32(offset, SVGA_INDEX_PORT);
            m_pIo->write32(value, SVGA_VALUE_PORT);
        }
        
        void writeFifo(uint32_t value)
        {
            volatile uint32_t *fifo = reinterpret_cast<volatile uint32_t*>(m_CommandRegion->virtualAddress());
            
            // Check for sync conditions
            if(((fifo[SVGA_FIFO_NEXT_CMD] + 4) == fifo[SVGA_FIFO_STOP]) ||
                (fifo[SVGA_FIFO_NEXT_CMD] == (fifo[SVGA_FIFO_MAX] - 4)))
            {
#if DEBUG_VMWARE_GFX
                DEBUG_LOG("vmware-gfx synchronising full fifo");
#endif
                syncFifo();
            }
            
#if DEBUG_VMWARE_GFX
            DEBUG_LOG("vmware-gfx fifo write at " << fifo[SVGA_FIFO_NEXT_CMD] << " val=" << value);
            DEBUG_LOG("vmware-gfx min is at " << fifo[SVGA_FIFO_MIN] << " and current position is " << fifo[SVGA_FIFO_STOP]);
#endif
            
            // Load the new item into the FIFO
            fifo[fifo[SVGA_FIFO_NEXT_CMD] / 4] = value;
            if(fifo[SVGA_FIFO_NEXT_CMD] == (fifo[SVGA_FIFO_MAX] - 4))
                fifo[SVGA_FIFO_NEXT_CMD] = fifo[SVGA_FIFO_MIN];
            else
                fifo[SVGA_FIFO_NEXT_CMD] += 4;
            
            asm volatile("" : : : "memory");
        }
        
        void syncFifo()
        {
            writeRegister(SVGA_REG_SYNC, 1);
            while(readRegister(SVGA_REG_BUSY));
        }
        
        MemoryMappedIo *m_Framebuffer;
        MemoryMappedIo *m_CommandRegion;
        
        VmwareFramebuffer *m_pFramebuffer;
        
        Device::Address *m_pFramebufferRawAddress;
};

void callback(Device *pDevice)
{
    VmwareGraphics *pGraphics = new VmwareGraphics(pDevice);
}

void entry()
{
    // Don't care about non-SVGA2 devices, just use VBE for them.
    Device::root().searchByVendorIdAndDeviceId(PCI_VENDOR_ID_VMWARE, PCI_DEVICE_ID_VMWARE_SVGA2, callback);
}

void exit()
{
}

MODULE_INFO("vmware-gfx", &entry, &exit, "pci", "config");
