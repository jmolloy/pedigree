/*
 * Copyright (c) 2010 Eduard Burtescu
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
#ifndef EHCI_H
#define EHCI_H

#include <machine/IrqHandler.h>
#include <processor/InterruptManager.h>
#include <processor/MemoryRegion.h>
#include <processor/PhysicalMemoryManager.h>
#include <processor/types.h>
#include <usb/UsbHub.h>
#include <utilities/ExtensibleBitmap.h>
#include <utilities/MemoryAllocator.h>

/** Device driver for the Ehci class */
class Ehci : public UsbHub,
#ifdef X86_COMMON
    public IrqHandler,
#else
    public InterruptHandler,
#endif
    public RequestQueue
{
    public:
        Ehci(Device* pDev);
        ~Ehci();

        typedef struct qTD
        {
            uint32_t bNextInvalid : 1;
            uint32_t res0 : 4;
            uint32_t next : 27;
            uint32_t bAltNextInvalid : 1;
            uint32_t res1 : 4;
            uint32_t alt_next : 27;
            uint32_t status : 8;
            uint32_t nPid : 2;
            uint32_t cerr : 2;
            uint32_t cpage : 3;
            uint32_t ioc : 1;
            uint32_t nBytes : 15;
            uint32_t data_toggle : 1;
            uint32_t coff : 12;
            uint32_t page0 : 20;
            uint32_t res2 : 12;
            uint32_t page1 : 20;
            uint32_t res3 : 12;
            uint32_t page2 : 20;
            uint32_t res4 : 12;
            uint32_t page3 : 20;
            uint32_t res5 : 12;
            uint32_t page4 : 20;
        } PACKED qTD;

        typedef struct QH
        {
            uint32_t bNextInvalid : 1;
            uint32_t next_type : 2;
            uint32_t res0 : 2;
            uint32_t next : 27;
            uint32_t nAddress : 7;
            uint32_t inactive_next : 1;
            uint32_t nEndpoint : 4;
            uint32_t speed : 2;
            uint32_t dtc : 1;
            uint32_t hrcl : 1;
            uint32_t maxpacksz : 11;
            uint32_t ctrlend : 1;
            uint32_t nakcrl : 4;
            uint32_t ism : 8;
            uint32_t scm : 8;
            uint32_t hub : 7;
            uint32_t port : 7;
            uint32_t mult : 2;
            uint32_t res1 : 5;
            uint32_t qtd_ptr : 27;
            qTD overlay;
            uint32_t pCallback;
            uint32_t pParam;
            uint32_t pBuffer;
            uint16_t size;
            uint16_t offset;
        } PACKED QH;

        virtual void getName(String &str)
        {
            str = "EHCI";
        }

        virtual void doAsync(UsbEndpoint endpointInfo, uint8_t nPid, uintptr_t pBuffer, uint16_t nBytes, void (*pCallback)(uintptr_t, ssize_t)=0, uintptr_t pParam=0);
        virtual void addInterruptInHandler(uint8_t nAddress, uint8_t nEndpoint, uintptr_t pBuffer, uint16_t nBytes, void (*pCallback)(uintptr_t, ssize_t), uintptr_t pParam=0);

        virtual UsbSpeed getSpeed()
        {
            return HighSpeed;
        }

        // IRQ handler callback.
#ifdef X86_COMMON
        virtual bool irq(irq_id_t number, InterruptState &state);
#else
        virtual void interrupt(size_t number, InterruptState &state);
#endif

    protected:
        uint64_t executeRequest(uint64_t p1, uint64_t p2, uint64_t p3, uint64_t p4, uint64_t p5,
                                    uint64_t p6, uint64_t p7, uint64_t p8);

    private:

        enum EhciConstants {
            EHCI_CMD = 0x00,            // Command register
            EHCI_STS = 0x04,            // Status register
            EHCI_INTR = 0x08,           // Intrerrupt Enable register
            EHCI_FRINDEX = 0x0c,        // Periodic Frame Index register
            EHCI_PERIODICLP = 0x14,     // Periodic List Pointer register
            EHCI_ASYNCLP = 0x18,        // Async List Pointer register
            EHCI_CFGFLAG = 0x40,        // Config Flag register
            EHCI_PORTSC = 0x44,         // Port Status/Control registers

            EHCI_CMD_ASYNCLE = 0x20,    // Async List Enable bit
            EHCI_CMD_PERIODICLE = 0x10, // Periodic List Enable bit
            EHCI_CMD_HCRES = 0x02,      // Host Controller Reset bit
            EHCI_CMD_RUN = 0x01,        // Run bit

            EHCI_STS_HALTED = 0x1000,   // Host Controller Halted bit
            EHCI_STS_PORTCH = 0x4,      // Port Change Detect bit
            EHCI_STS_INT = 0x1,         // On Completition Interrupt bit

            EHCI_PORTSC_PRES = 0x100,   // Port Reset bit
            EHCI_PORTSC_ENCH = 0x8,     // Port Enable/Disable Change bit
            EHCI_PORTSC_CSCH = 0x2,     // Port Connect Status Change bit
            EHCI_PORTSC_CONN = 0x1,     // Port Connected bit
        };

        IoBase *m_pBase;

        void pause();
        void resume();

        uint8_t m_nOpRegsOffset;

        Mutex m_Mutex;

        QH *m_pQHList;
        uint8_t *m_pQHListVirt;
        uintptr_t m_pQHListPhys;
        ExtensibleBitmap m_QHBitmap;

        uint32_t *m_pFrameList;
        uintptr_t m_pFrameListPhys;

        qTD *m_pqTDList;
        uint8_t *m_pqTDListVirt;
        uintptr_t m_pqTDListPhys;

        uint8_t *m_pTransferPagesVirt;
        uintptr_t m_pTransferPagesPhys;
        MemoryAllocator m_TransferPagesAllocator;

        MemoryRegion m_EhciMR;

        Ehci(const Ehci&);
        void operator =(const Ehci&);
};

#endif
