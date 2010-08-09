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
#include <processor/PhysicalMemoryManager.h>
#include <processor/VirtualAddressSpace.h>
#include <utilities/StaticString.h>
#include <processor/MemoryRegion.h>
#include <utilities/List.h>
#include <machine/Machine.h>
#include <machine/Display.h>
#include <config/Config.h>

// PCI IDs
#define PCI_VENDOR_VMWARE       0x15AD
#define PCI_DEVICE_VMWARE_SVGA1 0x710 // Has ports hardcoded to specific locations
#define PCI_DEVICE_VMWARE_SVGA2 0x405

// Index/value pair port offsets
#define SVGA_INDEX_PORT         0
#define SVGA_VALUE_PORT         1

#define SVGA_MAGIC		        0x900000UL
#define SVGA_MAKE_ID(ver)	    (SVGA_MAGIC << 8 | (ver))

#define SVGA_CAP_NONE   			0x00000
#define SVGA_CAP_RECT_FILL  		0x00001
#define SVGA_CAP_RECT_COPY  		0x00002
#define SVGA_CAP_RECT_PAT_FILL  	0x00004
#define SVGA_CAP_LEGACY_OFFSCREEN   0x00008
#define SVGA_CAP_RASTER_OP  		0x00010
#define SVGA_CAP_CURSOR 			0x00020
#define SVGA_CAP_CURSOR_BYPASS  	0x00040
#define SVGA_CAP_CURSOR_BYPASS_2	0x00080
#define SVGA_CAP_8BIT_EMULATION 	0x00100
#define SVGA_CAP_ALPHA_CURSOR   	0x00200
#define SVGA_CAP_GLYPH  			0x00400
#define SVGA_CAP_GLYPH_CLIPPING 	0x00800
#define SVGA_CAP_OFFSCREEN_1		0x01000
#define SVGA_CAP_ALPHA_BLEND		0x02000
#define SVGA_CAP_3D 				0x04000
#define SVGA_CAP_EXTENDED_FIFO  	0x08000
#define SVGA_CAP_MULTIMON   		0x10000
#define SVGA_CAP_PITCHLOCK  		0x20000

class VmwareGraphics : public Display
{
    public:
        VmwareGraphics(Device *pDev) :
            Device(pDev), Display(pDev), m_pIo(0),
            m_Framebuffer("vmware-gfx-framebuffer"),
            m_CommandRegion("vmware-gfx-command")
        {
            m_pIo = m_Addresses[0]->m_Io;
            
            // Support ID2, as we are expecting an SVGA2 functional device
            writeRegister(SVGA_REG_ID, SVGA_MAKE_ID(2));
            if(readRegister(SVGA_REG_ID) != SVGA_MAKE_ID(2))
            {
                WARNING("vmware-gfx: not a compatible SVGA device");
                return;
            }
            
            // Read the framebuffer base (should match BAR1)
            uintptr_t fbBase = readRegister(SVGA_REG_FBSTART);
            size_t fbSize = readRegister(SVGA_REG_VRAMSZ);
            
            // Read the command FIFO (should match BAR2)
            uintptr_t cmdBase = readRegister(SVGA_REG_MEMSTART);
            size_t cmdSize = readRegister(SVGA_REG_MEMSZ);
            
            // Find the capabilities of the device
            size_t caps = readRegister(SVGA_REG_CAPS);
            
            // Read maximum resolution
            size_t maxWidth = readRegister(SVGA_REG_MAX_WIDTH);
            size_t maxHeight = readRegister(SVGA_REG_MAX_HEIGHT);
            
            // Tell VMware what OS we are - "Other"
            writeRegister(SVGA_REG_GUESTID, 0x500A);
            
            // Debug notification
            NOTICE("vmware-gfx found, caps=" << caps << ", maximum resolution is " << Dec << maxWidth << "x" << maxHeight << Hex);
            NOTICE("vmware-gfx        framebuffer at " << fbBase << " - " << (fbBase + fbSize) << ", command FIFO at " << cmdBase);
            
            PhysicalMemoryManager::instance().allocateRegion(m_Framebuffer, (fbSize / 0x1000) + 1, PhysicalMemoryManager::continuous, VirtualAddressSpace::KernelMode | VirtualAddressSpace::Write, fbBase);
            PhysicalMemoryManager::instance().allocateRegion(m_CommandRegion, (cmdSize / 0x1000) + 1, PhysicalMemoryManager::continuous, VirtualAddressSpace::KernelMode | VirtualAddressSpace::Write, cmdBase);
            
            // Set mode
            writeRegister(SVGA_REG_WIDTH, 1024);
            writeRegister(SVGA_REG_HEIGHT, 768);
            writeRegister(SVGA_REG_BPP, 32);
            
            size_t fbOffset = readRegister(SVGA_REG_FBOFFSET);
            size_t bytesPerLine = readRegister(SVGA_REG_BPL);
            size_t depth = readRegister(SVGA_REG_DEPTH);
            
            NOTICE("vmware-gfx entered mode 1024x768x16, mode framebuffer is " << (fbBase + fbOffset));
            
            // Disable the command fifo
            writeRegister(SVGA_REG_CFGDONE, 0);
            
            // Enable the SVGA now
            writeRegister(SVGA_REG_ENABLE, 1);
            
            // Initialise the FIFO
            volatile uint32_t *fifo = reinterpret_cast<volatile uint32_t*>(m_CommandRegion.virtualAddress());
            
            size_t fifoBase = 0;
            
            if(caps & SVGA_CAP_EXTENDED_FIFO)
            {
                size_t numFifoRegs = readRegister(SVGA_REG_MEMREGS);
                size_t fifoExtendedBase = numFifoRegs * 4;
            
                fifo[SVGA_FIFO_MIN] = fifoExtendedBase; // Start right after this information block
                fifo[SVGA_FIFO_MAX] = cmdSize & ~0x3; // Permit the full FIFO to be used
                fifo[SVGA_FIFO_NEXT_CMD] = fifo[SVGA_FIFO_MIN]; // Empty FIFO
                fifo[SVGA_FIFO_STOP] = fifo[SVGA_FIFO_MIN];
                
                NOTICE("vmware-gfx using extended fifo, caps=" << fifo[SVGA_FIFO_CAPS] << ", flags=" << fifo[SVGA_FIFO_FLAGS]);
                NOTICE("vmware-gfx there are " << Dec << numFifoRegs << Hex << " FIFO registers");
                
                fifoBase = numFifoRegs;
            }
            else
            {
                fifo[SVGA_FIFO_MIN] = 16; // Start right after this information block
                fifo[SVGA_FIFO_MAX] = cmdSize & ~0x3; // Permit the full FIFO to be used
                fifo[SVGA_FIFO_NEXT_CMD] = fifo[SVGA_FIFO_STOP] = 16; // Empty FIFO
                
                fifoBase = 16 / sizeof(uint32_t);
            }
            
            // Enable the FIFO, we're configured now
            writeRegister(SVGA_REG_CFGDONE, 1);
            
            // Try some hardware accelerated stuff
            writeRegister(SVGA_REG_CFGDONE, 0);
            
            if(caps & SVGA_CAP_RECT_FILL)
            {
                NOTICE("vmware-gfx rect fill supported");
            
                writeFifo(2); // RECT FILL
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
                
                writeFifo(1); // RECT COPY
                writeFifo(8); // Source X
                writeFifo(8); // Source Y
                writeFifo(256); // Dest X
                writeFifo(256); // Dest Y
                writeFifo(256); // Width
                writeFifo(256); // Height
            }
            
            // Start running the FIFO
            writeRegister(SVGA_REG_CFGDONE, 1);
        }
        
        virtual ~VmwareGraphics()
        {
        }
        
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
            volatile uint32_t *fifo = reinterpret_cast<volatile uint32_t*>(m_CommandRegion.virtualAddress());
            
            // Check for sync conditions
            if(((fifo[SVGA_FIFO_STOP] + 4) == fifo[SVGA_FIFO_NEXT_CMD]) ||
                (fifo[SVGA_FIFO_STOP] == (fifo[SVGA_FIFO_MAX] - 4)))
            {
                NOTICE("vmware-gfx synchronising full fifo");
                syncFifo();
            }
            
            DEBUG_LOG("vmware-gfx fifo write at " << fifo[SVGA_FIFO_STOP] << " val=" << value);
            DEBUG_LOG("vmware-gfx min is at " << fifo[SVGA_FIFO_MIN] << " and current position is " << fifo[SVGA_FIFO_NEXT_CMD]);
            
            // Load the new item into the FIFO
            fifo[fifo[SVGA_FIFO_STOP] / 4] = value;
            if(fifo[SVGA_FIFO_STOP] == (fifo[SVGA_FIFO_MAX] - 4))
                fifo[SVGA_FIFO_STOP] = fifo[SVGA_FIFO_MIN];
            else
                fifo[SVGA_FIFO_STOP] += 4;
            
            asm volatile("" : : : "memory");
        }
        
        void syncFifo()
        {
            writeRegister(SVGA_REG_SYNC, 1);
            while(readRegister(SVGA_REG_BUSY));
        }
        
        MemoryRegion m_Framebuffer;
        MemoryRegion m_CommandRegion;

        enum SvgaRegisters
        {
            SVGA_REG_ID = 0,
            SVGA_REG_ENABLE = 1,
            SVGA_REG_WIDTH = 2,
            SVGA_REG_HEIGHT = 3,
            SVGA_REG_MAX_WIDTH = 4,
            SVGA_REG_MAX_HEIGHT = 5,
            SVGA_REG_DEPTH = 6,
            SVGA_REG_BPP = 7,
            SVGA_REG_PSUEDOCOLOUR = 8,
            SVGA_REG_RED_MASK = 9,
            SVGA_REG_GREEN_MASK = 10,
            SVGA_REG_BLUE_MASK = 11,
            SVGA_REG_BPL = 12, // Bytes per line
            SVGA_REG_FBSTART = 13,
            SVGA_REG_FBOFFSET = 14,
            SVGA_REG_VRAMSZ = 15,
            SVGA_REG_FBSIZE = 16,
            
            SVGA_REG_CAPS = 17,
            SVGA_REG_MEMSTART = 18,
            SVGA_REG_MEMSZ = 19,
            SVGA_REG_CFGDONE = 20,
            SVGA_REG_SYNC = 21,
            SVGA_REG_BUSY = 22,
            SVGA_REG_GUESTID = 23,
            SVGA_REG_CURSORID = 24,
            SVGA_REG_CURSORX = 25,
            SVGA_REG_CURSORY = 26,
            SVGA_REG_CURSORON = 27,
            SVGA_REG_HOST_BPP = 28,
            SVGA_REG_SCRATCHSZ = 29,
            SVGA_REG_MEMREGS = 30,
            SVGA_REG_NUM_DISPLAYS = 31,
            SVGA_REG_PITCHLOCK = 32,
            SVGA_REG_IRQMASK = 33,
            SVGA_REG_NUM_GUEST_DISPLAYS = 34,
            SVGA_REG_DISPLAYID = 35,
            SVGA_REG_DISPLAYPRIM = 36,
            SVGA_REG_DISPLAYX = 37,
            SVGA_REG_DISPLAYY = 38,
            SVGA_REG_DISPLAYWIDTH = 39,
            SVGA_REG_DISPLAYHEIGHT = 40,
        };
        
        enum Fifo
        {
            SVGA_FIFO_MIN = 0,
            SVGA_FIFO_MAX = 1,
            SVGA_FIFO_NEXT_CMD = 2,
            SVGA_FIFO_STOP = 3,
            
            SVGA_FIFO_CAPS = 4,
            SVGA_FIFO_FLAGS = 5,
            SVGA_FIFO_FENCE = 6,
            SVGA_FIFO_3D_HWVER = 7,
            SVGA_FIFO_PITCHLOCK,
        };
};

void callback(Device *pDevice)
{
    NOTICE("Found a vmware-gfx device");
    
    VmwareGraphics *pGraphics = new VmwareGraphics(pDevice);
}

void entry()
{
    NOTICE("vmware-gfx");
    
    // Don't care about non-SVGA2 devices, just use VBE for them.
    Device::root().searchByVendorIdAndDeviceId(PCI_VENDOR_VMWARE, PCI_DEVICE_VMWARE_SVGA2, callback);
    
    while(1) asm volatile("hlt");
}

void exit()
{
}

MODULE_INFO("vmware-gfx", &entry, &exit, "pci", "config");
