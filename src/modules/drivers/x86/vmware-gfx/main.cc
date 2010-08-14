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

// Can refer to http://sourceware.org/ml/ecos-devel/2006-10/msg00008/README.xfree86
// for information about programming this device, as well as the Haiku driver.

class VmwareGraphics : public Display
{
    friend class VmwareFramebuffer;
    
    public:
        VmwareGraphics(Device *pDev) :
            Device(pDev), Display(pDev), m_pIo(0),
            m_Framebuffer(0), m_CommandRegion(0)
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
            }
            else
            {
                m_Framebuffer = static_cast<MemoryMappedIo*>(m_Addresses[2]->m_Io);
                m_CommandRegion = static_cast<MemoryMappedIo*>(m_Addresses[1]->m_Io);
            }
            
            // Set mode
            writeRegister(SVGA_REG_WIDTH, 1024);
            writeRegister(SVGA_REG_HEIGHT, 768);
            writeRegister(SVGA_REG_BITS_PER_PIXEL, 32);
            
            size_t fbOffset = readRegister(SVGA_REG_FB_OFFSET);
            size_t bytesPerLine = readRegister(SVGA_REG_BYTES_PER_LINE);
            size_t width = readRegister(SVGA_REG_WIDTH);
            size_t height = readRegister(SVGA_REG_HEIGHT);
            size_t depth = readRegister(SVGA_REG_DEPTH);
            
            NOTICE("vmware-gfx entered mode " << Dec << width << "x" << height << "x" << depth << Hex << ", mode framebuffer is " << (fbBase + fbOffset));
            
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
            
            // Try some hardware accelerated stuff
            if(caps & SVGA_CAP_RECT_FILL)
            {
                NOTICE("vmware-gfx rect fill supported");
            
                writeFifo(SVGA_CMD_RECT_FILL); // RECT FILL
                writeFifo(0xFFFFFFFF); // White
                writeFifo(64); // X
                writeFifo(64); // Y
                writeFifo(256); // Width
                writeFifo(256); // Height
            }
            else
                NOTICE("vmware-gfx no rect fill available");
            
            if(caps & SVGA_CAP_RECT_COPY)
            {
                NOTICE("vmware-gfx rect copy supported");
                
                writeFifo(SVGA_CMD_RECT_COPY); // RECT COPY
                writeFifo(8); // Source X
                writeFifo(8); // Source Y
                writeFifo(256); // Dest X
                writeFifo(256); // Dest Y
                writeFifo(256); // Width
                writeFifo(256); // Height
            }
            else
                NOTICE("vmware-gfx no rect copy available");
            
            // Start running the FIFO
            writeRegister(SVGA_REG_CONFIG_DONE, 1);
            
            m_pFramebuffer = new VmwareFramebuffer(reinterpret_cast<uintptr_t>(m_Framebuffer->virtualAddress()), this);
            
            GraphicsService::GraphicsProvider *pProvider = new GraphicsService::GraphicsProvider;
            pProvider->pDisplay = this;
            pProvider->pFramebuffer = m_pFramebuffer;
            pProvider->maxWidth = maxWidth;
            pProvider->maxHeight = maxHeight;
            pProvider->maxDepth = 32;
            
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
        
        /** Returns the current screen mode.
            \return True if operation succeeded, false otherwise. */
        virtual bool getCurrentScreenMode(ScreenMode &sm)
        {
            sm.width = readRegister(SVGA_REG_WIDTH);
            sm.height = readRegister(SVGA_REG_HEIGHT);
            sm.pf.nBpp = readRegister(SVGA_REG_BITS_PER_PIXEL);
            
            sm.pf2 = Graphics::Bits24_Rgb;
            sm.bytesPerPixel = sm.pf.nBpp / 8;
            sm.bytesPerLine = readRegister(SVGA_REG_BYTES_PER_LINE);
            
            return true;
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
                
                VmwareFramebuffer(uintptr_t fb, Display *pDisplay) :
                    Framebuffer()
                {
                    setFramebuffer(fb);
                    setDisplay(pDisplay);
                }
                
                virtual ~VmwareFramebuffer()
                {
                }
                
                /** Performs an update of a region of this framebuffer. This function
                 *  can be used by drivers to request an area of the framebuffer be
                 *  redrawn, but is useless for non-hardware-accelerated devices.
                 *  \param x leftmost x co-ordinate of the redraw area, ~0 for "invalid"
                 *  \param y topmost y co-ordinate of the redraw area, ~0 for "invalid"
                 *  \param w width of the redraw area, ~0 for "invalid"
                 *  \param h height of the redraw area, ~0 for "invalid" */
                virtual void redraw(size_t x = ~0UL, size_t y = ~0UL,
                                    size_t w = ~0UL, size_t h = ~0UL)
                {
                    static_cast<VmwareGraphics*>(m_pDisplay)->redraw(x, y, w, h);
                }
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
};

#if 0
#include "abc.c"
#endif

void callback(Device *pDevice)
{
    VmwareGraphics *pGraphics = new VmwareGraphics(pDevice);

#if 0
    GraphicsService::GraphicsProvider pProvider;

    // Testing...
    ServiceFeatures *pFeatures = ServiceManager::instance().enumerateOperations(String("graphics"));
    Service         *pService  = ServiceManager::instance().getService(String("graphics"));
    if(pFeatures->provides(ServiceFeatures::probe))
        if(pService)
            pService->serve(ServiceFeatures::probe, reinterpret_cast<void*>(&pProvider), sizeof(pProvider));

    Framebuffer *pFramebuffer = pProvider.pFramebuffer;
    
    // Pixel conversion test - RGB565 to 32-bit ARGB, should be green
    pFramebuffer->rect(64, 64, 256, 256, 0x7E0, Graphics::Bits16_Rgb565);
    pFramebuffer->redraw(64, 64, 256, 256);
    
    // Software copy test
    pFramebuffer->copy(64, 64, 512, 256, 64, 64);
    pFramebuffer->redraw(512, 256, 64, 64);
    
    // Big 24-bit to 32-bit blit test
    Graphics::Buffer *pBuffer = pFramebuffer->createBuffer(gimp_image.pixel_data, Graphics::Bits24_Bgr, gimp_image.width, gimp_image.height);
    pFramebuffer->blit(pBuffer, 0, 0, 128, 128, gimp_image.width, gimp_image.height);
    
    pFramebuffer->redraw(128, 128, gimp_image.width, gimp_image.height);
    
    pFramebuffer->destroyBuffer(pBuffer);
    
    // Hardware-accelerated copy test, with a redraw afterwards
    pGraphics->copy(64, 64, 512, 512, 64, 64);
    pFramebuffer->redraw(512, 512, 64, 64);

    pFramebuffer->line(64, 512, 128, 512, 0xff0000, Graphics::Bits24_Rgb);
    pFramebuffer->redraw(63, 511, 66, 3);
#endif
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
