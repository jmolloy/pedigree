/*
 * Copyright (c) 2008 James Molloy, Jörg Pfähler, Matthew Iselin
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

/** Ported 3c90x driver from Etherboot.
  * I've changed it around to fit into our structure, and
  * to be a little more portable - Matt
  */

/*
 * 3c90x.c -- This file implements the 3c90x driver for etherboot.  Written
 * by Greg Beeley, Greg.Beeley@LightSys.org.  Modified by Steve Smith,
 * Steve.Smith@Juno.Com. Alignment bug fix Neil Newell (nn@icenoir.net).
 *
 * This program Copyright (C) 1999 LightSys Technology Services, Inc.
 * Portions Copyright (C) 1999 Steve Smith
 *
 * This program may be re-distributed in source or binary form, modified,
 * sold, or copied for any purpose, provided that the above copyright message
 * and this text are included with all source copies or derivative works, and
 * provided that the above copyright message and this text are included in the
 * documentation of any binary-only distributions.  This program is distributed
 * WITHOUT ANY WARRANTY, without even the warranty of FITNESS FOR A PARTICULAR
 * PURPOSE or MERCHANTABILITY.  Please read the associated documentation
 * "3c90x.txt" before compiling and using this driver.
 *
 * --------
 *
 * Program written with the assistance of the 3com documentation for
 * the 3c905B-TX card, as well as with some assistance from the 3c59x
 * driver Donald Becker wrote for the Linux kernel, and with some assistance
 * from the remainder of the Etherboot distribution.
 *
 * REVISION HISTORY:
 *
 * v0.10	1-26-1998	GRB	Initial implementation.
 * v0.90	1-27-1998	GRB	System works.
 * v1.00pre1	2-11-1998	GRB	Got prom boot issue fixed.
 * v2.0		9-24-1999	SCS	Modified for 3c905 (from 3c905b code)
 *					Re-wrote poll and transmit for
 *					better error recovery and heavy
 *					network traffic operation
 * v2.01    5-26-2993 NN Fixed driver alignment issue which
 *                  caused system lockups if driver structures
 *                  not 8-byte aligned.
 *
 */

#include "3Com90x.h"
#include "3Com90xConstants.h"
#include <Log.h>
#include <machine/Machine.h>
#include <machine/Network.h>
#include <network-stack/NetworkStack.h>
#include <processor/Processor.h>
#include <process/Scheduler.h>
#include <machine/IrqManager.h>

#include <network-stack/Ethernet.h>

int Nic3C90x::issueCommand(int cmd, int param)
{
    uint32_t val;

    /** Build the cmd. **/
    val = cmd;
    val <<= 11;
    val |= param;

    /** Send the cmd to the cmd register */
    m_pBase->write16(val, regCommandIntStatus_w);

    /** Wait for the cmd to complete, if necessary **/
    while (m_pBase->read16(regCommandIntStatus_w) & INT_CMDINPROGRESS);

    return 0;
}

int Nic3C90x::setWindow(int window)
{
    /** Window already as set? **/
    if (m_CurrentWindow == window) return 0;

    /** Issue the window command **/
    issueCommand(cmdSelectRegisterWindow, window);
    m_CurrentWindow = window;

    return 0;
}

uint16_t Nic3C90x::readEeprom(int address)
{
    uint16_t val;

    /** Select correct window **/
    setWindow(winEepromBios0);

    /** Make sure the eeprom isn't busy **/
    while ((1<<15) & m_pBase->read16(regEepromCommand_0_w));

    /** Read the value */
    m_pBase->write16(address + (0x02 << 6), regEepromCommand_0_w);
    while ((1<<15) & m_pBase->read16(regEepromCommand_0_w));
    val = m_pBase->read16(regEepromData_0_w);

    return val;
}

int Nic3C90x::writeEepromWord(int address, uint16_t value)
{
    /** Select register window **/
    setWindow(winEepromBios0);

    /** Verify Eeprom not busy **/
    while ((1<<15) & m_pBase->read16(regEepromCommand_0_w));

    /** Issue WriteEnable, and wait for completion **/
    m_pBase->write16(0x30, regEepromCommand_0_w);
    while ((1<<15) & m_pBase->read16(regEepromCommand_0_w));

    /** Issue EraseReigster, and wait for completion **/
    m_pBase->write16(address + (0x03 << 6), regEepromCommand_0_w);
    while ((1<<15) & m_pBase->read16(regEepromCommand_0_w));

    /** Send the new data to the eeprom, and wait for completion **/
    m_pBase->write16(value, regEepromData_0_w);
    m_pBase->write16(0x30, regEepromCommand_0_w);
    while ((1<<15) & m_pBase->read16(regEepromCommand_0_w));

    /** Burn the new data into the eeprom, and wait for completion **/
    m_pBase->write16(address + (0x01 << 6), regEepromCommand_0_w);
    while ((1<<15) & m_pBase->read16(regEepromCommand_0_w));

    return 0;
}

int Nic3C90x::writeEeprom(int address, uint16_t value)
{
    int cksum = 0, v;
    int i;
    int maxAddress, cksumAddress;

    if (m_isBrev)
    {
        maxAddress = 0x1f;
        cksumAddress = 0x20;
    }
    else
    {
        maxAddress = 0x16;
        cksumAddress = 0x17;
    }

    /** Write the value. **/
    if (writeEepromWord(address, value) == -1)
        return -1;

    /** Recompute the checksum **/
    for (i = 0; i <= maxAddress; i++)
    {
        v = readEeprom(i);
        cksum ^= (v & 0xff);
        cksum ^= ((v >> 8) & 0xff);
    }

    /** Write the checksum to the location in the eeprom **/
    if (writeEepromWord(cksumAddress, cksum) == -1)
        return -1;

    return 0;
}

void Nic3C90x::reset()
{
#ifdef	CFG_3C90X_PRESERVE_XCVR
    int cfg;

    /** Read the current InternalConfig value **/
    setWindow(winTxRxOptions3);
    cfg = m_pBase->read32(regInternalConfig_3_l);
#endif

    /** Send the reset command to the card **/
    NOTICE("3C90x: Issuing RESET");
    issueCommand(cmdGlobalReset, 0);

    /** Wait for reset command to complete **/
    while (m_pBase->read16(regCommandIntStatus_w) & INT_CMDINPROGRESS);

    /** Global reset command resets station mask, non-B revision cards
      * require explicit reset of values
      */
    setWindow(winAddressing2);
    m_pBase->write16(0, regStationAddress_2_3w + 0);
    m_pBase->write16(0, regStationAddress_2_3w + 2);
    m_pBase->write16(0, regStationAddress_2_3w + 4);

#ifdef	CFG_3C90X_PRESERVE_XCVR
    /** Reset the original InternalConfig value from before reset **/
    setWindow(winTxRxOptions3);
    m_pBase->write32(cfg, regInternalConfig_3_l);

    /** Enable DC converter for 10-Base-T **/
    if ((cfg & 0x0300) == 0x0300)
        issueCommand(cmdEnableDcConverter, 0);
#endif

    /** Issue transmit reset, wait for command completion **/
    issueCommand(cmdTxReset, 0);
    while (m_pBase->read16(regCommandIntStatus_w) & INT_CMDINPROGRESS);

    if (!m_isBrev)
        m_pBase->write8(0x01, regTxFreeThresh_b);
    issueCommand(cmdTxEnable, 0);

    /** Reset of the receiver on B-revision cards re-negotiates the link
      * Takes several seconds
      */
    if (m_isBrev)
        issueCommand(cmdRxReset, 0x04);
    else
        issueCommand(cmdRxReset, 0x00);
    while (m_pBase->read16(regCommandIntStatus_w) & INT_CMDINPROGRESS);

    issueCommand(cmdRxEnable, 0);

    /** Set indication and interrupt flags, ack any IRQs **/
    issueCommand(cmdSetInterruptEnable, ENABLED_INTS);
    issueCommand(cmdSetIndicationEnable, ENABLED_INTS); //0x0014);
    issueCommand(cmdAcknowledgeInterrupt, 0xff); //0x661);

}

bool Nic3C90x::send(size_t nBytes, uintptr_t buffer)
{
    uint32_t retries;

    for (retries = 0; retries < XMIT_RETRIES; retries++)
    {
        /** Stall the download engine **/
        issueCommand(cmdStallCtl, 2);

        /** Make sure the card is not waiting on us **/
        m_pBase->read16(regCommandIntStatus_w);
        m_pBase->read16(regCommandIntStatus_w);
        while (m_pBase->read16(regCommandIntStatus_w) & INT_CMDINPROGRESS);

        physical_uintptr_t destPtr = m_pTxBuffPhys;
        size_t dud = 0;
        if (Processor::information().getVirtualAddressSpace().isMapped(reinterpret_cast<void*>(buffer)))
        {
            Processor::information().getVirtualAddressSpace().getMapping(reinterpret_cast<void*>(buffer), destPtr, dud);
            destPtr += buffer & 0xFFF;
        }
        else
            memcpy(m_pTxBuffVirt, reinterpret_cast<void *>(buffer), nBytes);

        /** Setup the DPD (download descriptor) **/
        m_TransmitDPD->DnNextPtr = 0;

        /** Set notification for transmission complete (bit 15) **/
        m_TransmitDPD->FrameStartHeader = nBytes | 0x8000;
        // m_TransmitDPD->HdrAddr = m_pTxBuffPhys;
        // m_TransmitDPD->HdrLength = Ethernet::instance().ethHeaderSize();
        m_TransmitDPD->DataAddr = static_cast<uint32_t>(destPtr); //m_pTxBuffPhys; // + m_TransmitDPD->HdrLength;
        m_TransmitDPD->DataLength = (nBytes /* - m_TransmitDPD->HdrLength */) + (1 << 31);

        /** Send the packet **/
        m_pBase->write32(m_pDPD, regDnListPtr_l);

        /** End Stall and Wait for upload to complete. **/
        issueCommand(cmdStallCtl, 3);
        while (m_pBase->read32(regDnListPtr_l) != 0);

        m_TxMutex.acquire();

        return true;
    }

    ERROR("3C90x: Failed to send after " << Dec << retries << Hex << " retries!");

    return false;
}

Nic3C90x::Nic3C90x(Network* pDev) :
        Network(pDev), m_pBase(0), m_isBrev(0), m_CurrentWindow(0),
        m_pRxBuffVirt(0), m_pTxBuffVirt(0), m_pRxBuffPhys(0), m_pTxBuffPhys(0),
        m_RxBuffMR("3c90x-rxbuffer"), m_TxBuffMR("3c90x-txbuffer"),
        m_pDPD(0), m_DPDMR("3c90x-dpd"), m_pUPD(0), m_UPDMR("3c90x-upd"),
        m_TransmitDPD(0), m_ReceiveUPD(0), m_RxMutex(0), m_TxMutex(0), m_PendingPackets()
{
    setSpecificType(String("3c90x-card"));

    int i, c;
    uint16_t eeprom[0x21];
    uint32_t cfg;
    uint32_t mopt;
    uint32_t mstat;
    uint16_t linktype;
#define HWADDR_OFFSET 10

    // allocate the rx and tx buffers
    if (!PhysicalMemoryManager::instance().allocateRegion(m_RxBuffMR, (MAX_PACKET_SIZE / 0x1000) + 1, PhysicalMemoryManager::continuous, VirtualAddressSpace::Write, -1))
    {
        ERROR("3C90x: Couldn't allocate Rx Buffer!");
        return;
    }
    if (!PhysicalMemoryManager::instance().allocateRegion(m_TxBuffMR, (MAX_PACKET_SIZE / 0x1000) + 1, PhysicalMemoryManager::continuous, VirtualAddressSpace::Write, -1))
    {
        ERROR("3C90x: Couldn't allocate Tx Buffer!");
        return;
    }
    m_pRxBuffVirt = static_cast<uint8_t *>(m_RxBuffMR.virtualAddress());
    m_pTxBuffVirt = static_cast<uint8_t *>(m_TxBuffMR.virtualAddress());
    m_pRxBuffPhys = m_RxBuffMR.physicalAddress();
    m_pTxBuffPhys = m_TxBuffMR.physicalAddress();

    if (!PhysicalMemoryManager::instance().allocateRegion(m_DPDMR, 2, PhysicalMemoryManager::continuous, VirtualAddressSpace::Write, -1))
    {
        ERROR("3C90x: Couldn't allocated buffer for DPD\n");
        return;
    }
    if (!PhysicalMemoryManager::instance().allocateRegion(m_UPDMR, 2, PhysicalMemoryManager::continuous, VirtualAddressSpace::Write, -1))
    {
        ERROR("3C90x: Couldn't allocated buffer for UPD\n");
        return;
    }
    m_pDPD = m_DPDMR.physicalAddress();
    m_pUPD = m_UPDMR.physicalAddress();
    m_TransmitDPD = reinterpret_cast<TXD*>(m_DPDMR.virtualAddress());
    m_ReceiveUPD = reinterpret_cast<RXD*>(m_UPDMR.virtualAddress());

    // configure the UPD... turn it into a list with a well-defined beginning and end
    for (size_t iUpd = 0; iUpd < NUM_UPDS; iUpd++)
    {
        if ((iUpd + 1) == NUM_UPDS)
            m_ReceiveUPD[iUpd].UpNextPtr = 0;
        else
            m_ReceiveUPD[iUpd].UpNextPtr = m_pUPD + ((iUpd + 1) * sizeof(RXD));
        m_ReceiveUPD[iUpd].UpPktStatus = 0;
        m_ReceiveUPD[iUpd].DataAddr = m_pRxBuffPhys + (iUpd * 1536);
        m_ReceiveUPD[iUpd].DataLength = 1536 + (1 << 31);
    }

    // grab the IO ports
    m_pBase = m_Addresses[0]->m_Io;

    m_CurrentWindow = 255;

    reset();

    switch (readEeprom(0x03))
    {
    case 0x9000: /** 10 Base TPO **/
    case 0x9001: /** 10/100 T4 **/
    case 0x9050: /** 10/100 TPO **/
    case 0x9051: /** 10 Base Combo **/
        m_isBrev = 0;
        break;

    case 0x9004: /** 10 Base TPO **/
    case 0x9005: /** 10 Base Combo **/
    case 0x9006: /** 10 Base TPO and Base2 **/
    case 0x900A: /** 10 Base FL **/
    case 0x9055: /** 10/100 TPO **/
    case 0x9056: /** 10/100 T4 **/
    case 0x905A: /** 10 Base FX **/
    default:
        m_isBrev = 1;
        break;
    }

    /** Load EEPROM contents **/
    if (m_isBrev)
    {
        for (i = 0; i < 0x20; i++)
            eeprom[i] = readEeprom(i);

#ifdef CFG_3C90X_BOOTROM_FIX
        /** Set xcvrSelect in InternalConfig in eeprom. **/
        /* only necessary for 3c905b revision cards with boot PROM bug!!! */
        writeEeprom(0x13, 0x0160);
#endif

#ifdef	CFG_3C90X_XCVR
        /** Clear the LanWorks register **/
        if (CFG_3C90X_XCVR == 255)
            writeEeprom(0x16, 0);

        /** Set the selected permanent-xcvrSelect in the
         ** LanWorks register
         **/
        else
            writeEeprom(0x16, XCVR_MAGIC + ((CFG_3C90X_XCVR) & 0x000F));
#endif

    }
    else
    {
        for (i = 0; i < 0x17; i++)
            eeprom[i] = readEeprom(i);
    }

    /** Get the hardware address */
    m_StationInfo.mac.setMac(reinterpret_cast<uint8_t*>(eeprom), true);
    NOTICE("3C90x MAC: " <<
           m_StationInfo.mac[0] << ":" <<
           m_StationInfo.mac[1] << ":" <<
           m_StationInfo.mac[2] << ":" <<
           m_StationInfo.mac[3] << ":" <<
           m_StationInfo.mac[4] << ":" <<
           m_StationInfo.mac[5] << ".");

    /* Test if the link is good, if so continue */
    setWindow(winDiagnostics4);
    mstat = m_pBase->read16(regMediaStatus_4_w);
    if ((mstat & (1 << 11)) == 0)
    {
        ERROR("3C90x: Valid link not established");
        return;
    }

    /** Program the MAC address into the station address registers */
    setWindow(winAddressing2);
    m_pBase->write16(HOST_TO_BIG16(eeprom[HWADDR_OFFSET + 0]), regStationAddress_2_3w);
    m_pBase->write16(HOST_TO_BIG16(eeprom[HWADDR_OFFSET + 1]), regStationAddress_2_3w + 2);
    m_pBase->write16(HOST_TO_BIG16(eeprom[HWADDR_OFFSET + 2]), regStationAddress_2_3w + 4);
    m_pBase->write16(0, regStationMask_2_3w);
    m_pBase->write16(0, regStationMask_2_3w + 2);
    m_pBase->write16(0, regStationMask_2_3w + 4);

    /** Read the media options register, print a message and set default
      * xcvr.
      *
      * Uses Media Option command on B revision, Reset Option on non-B
      * revision cards -- same register address
      */
    setWindow(winTxRxOptions3);
    mopt = m_pBase->read16(regResetMediaOptions_3_w);

    /** mask out VCO bit that is defined as 10 base FL bit on B-rev cards **/
    if (!m_isBrev)
        mopt &= 0x7f;

    NOTICE("3C90x connectors present:");
    c = 0;
    linktype = 0x0008;
    if (mopt & 0x01)
    {
        NOTICE(((c++) ? ", " : "") << "100BASE-T4");
        linktype = 0x0006;
    }
    if (mopt & 0x04)
    {
        NOTICE(((c++) ? ", " : "") << "100BASE-FX");
        linktype = 0x0005;
    }
    if (mopt & 0x10)
    {
        NOTICE(((c++) ? ", " : "") << "10BASE2");
        linktype = 0x0003;
    }
    if (mopt & 0x20)
    {
        NOTICE(((c++) ? ", " : "") << "AUI");
        linktype = 0x0001;
    }
    if (mopt & 0x40)
    {
        NOTICE(((c++) ? ", " : "") << "MII");
        linktype = 0x0006;
    }
    if ((mopt & 0xA) == 0xA)
    {
        NOTICE(((c++) ? ", " : "") << "10BASE-T / 100BASE-TX");
        linktype = 0x0008;
    }
    else if ((mopt & 0xa) == 0x2)
    {
        NOTICE(((c++) ? ", " : "") << "100BASE-TX");
        linktype = 0x0008;
    }
    else if ((mopt & 0xa) == 0x8)
    {
        NOTICE(((c++) ? ", " : "") << "10BASE-T");
        linktype = 0x0008;
    }

    /** Determine transceiver type to use, depending on value stored in
      * eeprom 0x16
      */
    if (m_isBrev)
    {
        if ((eeprom[0x16] & 0xff00) == XCVR_MAGIC)
            linktype = eeprom[0x16] & 0x000f;
    }
    else
    {
#ifdef CFG_3C90X_XCVR
        if (CFG_3C90X_XCVR != 255)
            linktype = CFG_3C90X_XCVR;
#endif

        if (linktype == 0x0009)
        {
            if (m_isBrev)
                WARNING("3C90x: MII External MAC mode only supported on B-revision cards! Falling back to MII mode.");
            linktype = 0x0006;
        }
    }

    /** Enable DC converter for 10-BASE-T **/
    if (linktype == 0x0003)
        issueCommand(cmdEnableDcConverter, 0);

    /** Set the link to the type we just determined **/
    setWindow(winTxRxOptions3);
    cfg = m_pBase->read32(regInternalConfig_3_l);
    cfg &= ~(0xF << 20);
    cfg |= (linktype << 20);
    m_pBase->write32(cfg, regInternalConfig_3_l);

    /** Now that we've set the xcvr type, reset TX and RX, re-enable **/
    issueCommand(cmdTxReset, 0);
    while (m_pBase->read16(regCommandIntStatus_w) & INT_CMDINPROGRESS);

    if (!m_isBrev)
        m_pBase->write8(0x01, regTxFreeThresh_b);
    issueCommand(cmdTxEnable, 0);

    /** reset of the receiver on B-revision cards re-negotiates the link
      * takes several seconds
      */
    if (m_isBrev)
        issueCommand(cmdRxReset, 0x04);
    else
        issueCommand(cmdRxReset, 0x00);
    while (m_pBase->read16(regCommandIntStatus_w) & INT_CMDINPROGRESS);

    /** Set the RX filter = receive only individual packets & multicast & broadcast **/
    issueCommand(cmdSetRxFilter, 0x01 + 0x02 + 0x04);
    issueCommand(cmdRxEnable, 0);

    /** Set indication and interrupt flags, ack any IRQs **/
    issueCommand(cmdSetInterruptEnable, ENABLED_INTS);
    issueCommand(cmdSetIndicationEnable, ENABLED_INTS); //0x0014);
    issueCommand(cmdAcknowledgeInterrupt, 0xff); //0x661);

    // Set the location for the UPD
    m_pBase->write32(m_pUPD, regUpListPtr_l);

    // register the packet queue handler
#ifdef THREADS
    new Thread(Processor::information().getCurrentThread()->getParent(),
               reinterpret_cast<Thread::ThreadStartFunc> (&trampoline),
               reinterpret_cast<void*> (this));
#endif

    // install the IRQ
    Machine::instance().getIrqManager()->registerIsaIrqHandler(getInterruptNumber(), static_cast<IrqHandler*>(this));
    NetworkStack::instance().registerDevice(this);
}

Nic3C90x::~Nic3C90x()
{
}

int Nic3C90x::trampoline(void *p)
{
    Nic3C90x *pNic = reinterpret_cast<Nic3C90x*> (p);
    pNic->receiveThread();
    return 0;
}

void Nic3C90x::receiveThread()
{
    while (true)
    {
        m_RxMutex.acquire();

        // When we come here, the UpListPtr register will hold the *next* UPD...
        // What we want is the one that it used! That's ok, it's not difficult
        // to find that out...
        // However, if the next is zero, the IRQ notified us of the *last* UPD in
        // the list. That needs to be handled properly too.
        uintptr_t currUpdPhys = reinterpret_cast<uintptr_t>(m_PendingPackets.popFront());
        uintptr_t myNum;
        if (currUpdPhys != 0)
        {
            uintptr_t myOffset = (currUpdPhys - m_pUPD);
            myNum = (myOffset / sizeof(RXD)) - 1;
        }
        else
            myNum = NUM_UPDS - 1;
        RXD *usedUpd = &m_ReceiveUPD[myNum];

        if (usedUpd->UpPktStatus & (1 << 14))
        {
            // an error occurred
            ERROR("3C90x: error, UpPktStatus = " << usedUpd->UpPktStatus << ".");
            return;
        }

        size_t packLen = usedUpd->UpPktStatus & 0x1FFF;

        NetworkStack::instance().receive(packLen, reinterpret_cast<uintptr_t>(m_pRxBuffVirt + (myNum * 1536)), this, 0);

        // reset the UPD's status so it can be used again
        usedUpd->UpPktStatus = 0;

        // Reset the location for the UPD, if we're stalling
        if (currUpdPhys == 0)
            m_pBase->write32(m_pUPD, regUpListPtr_l);
    }
}


bool Nic3C90x::irq(irq_id_t number, InterruptState &state)
{
    // disable interrupts
    issueCommand(cmdSetInterruptEnable, 0);

    while (1)
    {
        uint16_t status = m_pBase->read16(regCommandIntStatus_w);

        // check that one of the enabled IRQs is triggered
        if ((status & ENABLED_INTS) == 0)
            break;

        // acknowledge the interrupts
        issueCommand(cmdAcknowledgeInterrupt, (status & ENABLED_INTS));

        // handle...
        if (status & INT_UPCOMPLETE)
        {
            void *currPhys = reinterpret_cast<void*>(m_pBase->read32(regUpListPtr_l));
            m_PendingPackets.pushBack(currPhys);
            m_RxMutex.release();
        }

        if (status & INT_TXCOMPLETE)
        {
            m_TxMutex.release();

            uint8_t txStatus = m_pBase->read8(regTxStatus_b);

            // ack it
            m_pBase->write8(0, regTxStatus_b);

            if ((txStatus & 0xbf) == 0x80)
                continue;

            if (txStatus & 0x02)
            {
                ERROR("3C90x: TX Reclaim Error");
                reset();
            }
            else if (txStatus & 0x04)
            {
                ERROR("3C90x: TX Status Overflow");
                for (int i = 0; i < 32; i++)
                    m_pBase->write8(0, regTxStatus_b);
                issueCommand(cmdTxEnable, 0);
            }
            else if (txStatus & 0x08)
            {
                ERROR("3C90x: TX Max Collisions");
                issueCommand(cmdTxEnable, 0);
            }
            else if (txStatus & 0x10)
            {
                ERROR("3C90x: TX Underrun");
                reset();
            }
            else if (txStatus & 0x20)
            {
                ERROR("3C90x: TX Jabber");
                reset();
            }
            else if ((txStatus & 0x80) != 0x80)
            {
                ERROR("3C90x: Internal Error - Incomplete Transmission");
                reset();
            }
        }

        if (status & INT_HOSTERROR)
        {
            NOTICE("Host error IRQ");
            reset();
        }

        if (status & INT_UPDATESTATS)
            NOTICE("UpdateStats IRQ");
    }

    // Re-enable interrupts
    issueCommand(cmdSetInterruptEnable, ENABLED_INTS);

    /*
    XL_SEL_WIN(7);

      if (ifp->if_snd.ifq_head != NULL)
      	xl_start(ifp);
    */

    return true;
}

bool Nic3C90x::setStationInfo(StationInfo info)
{
    // free the old DNS servers list, if there is one
    if (m_StationInfo.dnsServers)
        delete [] m_StationInfo.dnsServers;

    // MAC isn't changeable, so set it all manually
    m_StationInfo.ipv4 = info.ipv4;
    NOTICE("3C90x: Setting ipv4, " << info.ipv4.toString() << ", " << m_StationInfo.ipv4.toString() << "...");
    m_StationInfo.ipv6 = info.ipv6;

    m_StationInfo.subnetMask = info.subnetMask;
    NOTICE("3C90x: Setting subnet mask, " << info.subnetMask.toString() << ", " << m_StationInfo.subnetMask.toString() << "...");
    m_StationInfo.gateway = info.gateway;
    NOTICE("3C90x: Setting gateway, " << info.gateway.toString() << ", " << m_StationInfo.gateway.toString() << "...");

    // Callers do not free their dnsServers memory
    m_StationInfo.dnsServers = info.dnsServers;
    m_StationInfo.nDnsServers = info.nDnsServers;
    NOTICE("3C90x: Setting DNS servers [" << Dec << m_StationInfo.nDnsServers << Hex << " servers being set]...");

    return true;
}

StationInfo Nic3C90x::getStationInfo()
{
    return m_StationInfo;
}
