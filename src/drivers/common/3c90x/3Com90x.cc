/*
* Copyright (c) 2008 James Molloy, Jörg Pfähler, Matthew Iselin, eddyb
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
#include <network/NetworkStack.h>
#include <processor/Processor.h>

#include <network/Ethernet.h>

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
  while(m_pBase->read16(regCommandIntStatus_w) & INT_CMDINPROGRESS);

  return 0;
}

int Nic3C90x::setWindow(int window)
{
  /** Window already as set? **/
  if(m_CurrentWindow == window) return 0;

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
  while((1<<15) & m_pBase->read16(regEepromCommand_0_w));

  /** Read the value */
  m_pBase->write16(address + (0x02 << 6), regEepromCommand_0_w);
  while((1<<15) & m_pBase->read16(regEepromCommand_0_w));
  val = m_pBase->read16(regEepromData_0_w);

  return val;
}

int Nic3C90x::writeEepromWord(int address, uint16_t value)
{
  /** Select register window **/
  setWindow(winEepromBios0);

  /** Verify Eeprom not busy **/
  while((1<<15) & m_pBase->read16(regEepromCommand_0_w));

  /** Issue WriteEnable, and wait for completion **/
  m_pBase->write16(0x30, regEepromCommand_0_w);
  while((1<<15) & m_pBase->read16(regEepromCommand_0_w));

  /** Issue EraseReigster, and wait for completion **/
  m_pBase->write16(address + (0x03 << 6), regEepromCommand_0_w);
  while((1<<15) & m_pBase->read16(regEepromCommand_0_w));

  /** Send the new data to the eeprom, and wait for completion **/
  m_pBase->write16(value, regEepromData_0_w);
  m_pBase->write16(0x30, regEepromCommand_0_w);
  while((1<<15) & m_pBase->read16(regEepromCommand_0_w));

  /** Burn the new data into the eeprom, and wait for completion **/
  m_pBase->write16(address + (0x01 << 6), regEepromCommand_0_w);
  while((1<<15) & m_pBase->read16(regEepromCommand_0_w));

  return 0;
}

int Nic3C90x::writeEeprom(int address, uint16_t value)
{
  int cksum = 0, v;
  int i;
  int maxAddress, cksumAddress;

  if(m_isBrev)
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
  if(writeEepromWord(address, value) == -1)
    return -1;

  /** Recompute the checksum **/
  for(i = 0; i <= maxAddress; i++)
  {
    v = readEeprom(i);
    cksum ^= (v & 0xff);
    cksum ^= ((v >> 8) & 0xff);
  }

  /** Write the checksum to the location in the eeprom **/
  if(writeEepromWord(cksumAddress, cksum) == -1)
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
  NOTICE("3C90x: Issuing RESET:");
  issueCommand(cmdGlobalReset, 0);

  /** Wait for reset command to complete **/
  while(m_pBase->read16(regCommandIntStatus_w) & INT_CMDINPROGRESS);

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
  if((cfg & 0x0300) == 0x0300)
    issueCommand(cmdEnableDcConverter, 0);
#endif

  /** Issue transmit reset, wait for command completion **/
  issueCommand(cmdTxReset, 0);
  while(m_pBase->read16(regCommandIntStatus_w) & INT_CMDINPROGRESS);

  if(!m_isBrev)
    m_pBase->write8(0x01, regTxFreeThresh_b);
  issueCommand(cmdTxEnable, 0);

  /** Reset of the receiver on B-revision cards re-negotiates the link
    * Takes several seconds
    */
  if(m_isBrev)
    issueCommand(cmdRxReset, 0x04);
  else
    issueCommand(cmdRxReset, 0x00);
  while(m_pBase->read16(regCommandIntStatus_w) & INT_CMDINPROGRESS);

  issueCommand(cmdRxEnable, 0);

  /// \todo I need to actually read the Linux driver to find out the right
  ///       IRQ and all that
  issueCommand(cmdSetInterruptEnable, 0);
  issueCommand(cmdSetIndicationEnable, 0x0014);
  issueCommand(cmdAcknowledgeInterrupt, 0x661);
}

bool Nic3C90x::send(size_t nBytes, uintptr_t buffer)
{
  uint8_t status;
  uint32_t i, retries;

  for(retries = 0; retries < XMIT_RETRIES; retries++)
  {
    /** Stall the download engine **/
    issueCommand(cmdStallCtl, 2);

    /** Make sure the card is not waiting on us **/
    m_pBase->read16(regCommandIntStatus_w);
    m_pBase->read16(regCommandIntStatus_w);
    while(m_pBase->read16(regCommandIntStatus_w) & INT_CMDINPROGRESS);

    /** Setup the DPD (download descriptor) **/
    m_TransmitDPD.DnNextPtr = 0;

    /** Set notification for transmission complete (bit 15) **/
    m_TransmitDPD.FrameStartHeader = nBytes | 0x8000;
    m_TransmitDPD.HdrAddr = /* convert to physical */ buffer;
    m_TransmitDPD.HdrLength = Ethernet::instance().ethHeaderSize();
    m_TransmitDPD.DataAddr = /* convert to physical */ buffer + m_TransmitDPD.HdrLength;
    m_TransmitDPD.DataLength = (nBytes - m_TransmitDPD.HdrLength) + (1 << 31);

    /** Send the packet **/
    m_pBase->write32(/* convert to physical */ reinterpret_cast<uint32_t>(&m_TransmitDPD), regDnListPtr_l);

    /** End Stall and Wait for upload to complete. **/
    issueCommand(cmdStallCtl, 3);
    while(m_pBase->read32(regDnListPtr_l) != 0);

    /** Wait for NIC transmit to complete **/
    /// \todo Meant to be a 10 ms timeout on this
    while(!(m_pBase->read16(regCommandIntStatus_w) & 0x0004));

    if(!(m_pBase->read16(regCommandIntStatus_w) & 0x0004))
    {
      ERROR("3C90x: TX Timeout");
      continue;
    }

    status = m_pBase->read8(regTxStatus_b);

    /** acknowledge transmit interrupt by writing status **/
    m_pBase->write8(0, regTxStatus_b);

    /** Successful completion **/
    if((status & 0xbf) == 0x80)
      return true;

    /** Check errors */
    NOTICE("3C90x: Status (" << status << ")");
    if(status & 0x02)
    {
      ERROR("3C90x: TX Reclaim Error");
      reset();
    }
    else if(status & 0x04)
    {
      ERROR("3C90x: TX Status Overflow");
      for(i = 0; i < 32; i++)
        m_pBase->write8(0, regTxStatus_b);
      issueCommand(cmdTxEnable, 0);
    }
    else if(status & 0x08)
    {
      ERROR("3C90x: TX Max Collisions");
      issueCommand(cmdTxEnable, 0);
    }
    else if(status & 0x10)
    {
      ERROR("3C90x: TX Underrun");
      reset();
    }
    else if(status & 0x20)
    {
      ERROR("3C90x: TX Jabber");
      reset();
    }
    else if((status & 0x80) != 0x80)
    {
      ERROR("3C90x: Internal Error - Incomplete Transmission");
      reset();
    }
  }

  ERROR("3C90x: Failed to send after " << Dec << retries << Hex << " retries!");

  return false;
}

#if 0
/*** a3c90x_poll: exported routine that waits for a certain length of time
 *** for a packet, and if it sees none, returns 0.  This routine should
 *** copy the packet to nic->packet if it gets a packet and set the size
 *** in nic->packetlen.  Return 1 if a packet was found.
 ***/
static int
a3c90x_poll(struct nic *nic)
    {
    int i, errcode;

    if (!(inw(INF_3C90X.IOAddr + regCommandIntStatus_w)&0x0010))
	{
	return 0;
	}

    /** we don't need to acknowledge rxComplete -- the upload engine
     ** does it for us.
     **/

    /** Build the up-load descriptor **/
    INF_3C90X.ReceiveUPD.UpNextPtr = 0;
    INF_3C90X.ReceiveUPD.UpPktStatus = 0;
    INF_3C90X.ReceiveUPD.DataAddr = virt_to_bus(nic->packet);
    INF_3C90X.ReceiveUPD.DataLength = 1536 + (1<<31);

    /** Submit the upload descriptor to the NIC **/
    outl(virt_to_bus(&(INF_3C90X.ReceiveUPD)),
         INF_3C90X.IOAddr + regUpListPtr_l);

    /** Wait for upload completion (upComplete(15) or upError (14)) **/
    for(i=0;i<40000;i++);
    while((INF_3C90X.ReceiveUPD.UpPktStatus & ((1<<14) | (1<<15))) == 0)
	for(i=0;i<40000;i++);

    /** Check for Error (else we have good packet) **/
    if (INF_3C90X.ReceiveUPD.UpPktStatus & (1<<14))
	{
	errcode = INF_3C90X.ReceiveUPD.UpPktStatus;
	if (errcode & (1<<16))
	    printf("3C90X: Rx Overrun (%hX)\n",errcode>>16);
	else if (errcode & (1<<17))
	    printf("3C90X: Runt Frame (%hX)\n",errcode>>16);
	else if (errcode & (1<<18))
	    printf("3C90X: Alignment Error (%hX)\n",errcode>>16);
	else if (errcode & (1<<19))
	    printf("3C90X: CRC Error (%hX)\n",errcode>>16);
	else if (errcode & (1<<20))
	    printf("3C90X: Oversized Frame (%hX)\n",errcode>>16);
	else
	    printf("3C90X: Packet error (%hX)\n",errcode>>16);
	return 0;
	}

    /** Ok, got packet.  Set length in nic->packetlen. **/
    nic->packetlen = (INF_3C90X.ReceiveUPD.UpPktStatus & 0x1FFF);

    return 1;
    }
#endif

Nic3C90x::Nic3C90x(Network* pDev) :
  Network(pDev), m_pBase(0), m_StationInfo(), m_isBrev(0), m_CurrentWindow(0), m_TransmitDPD(), m_ReceiveUPD()
{
    setSpecificType(String("3c90x-card"));

    int i, c;
    uint16_t eeprom[0x21];
    uint32_t cfg;
    uint32_t mopt;
    uint32_t mstat;
    uint16_t linktype;
#define HWADDR_OFFSET 10

    // grab the IO ports
    m_pBase = m_Addresses[0]->m_Io;

    m_CurrentWindow = 255;

    switch(readEeprom(0x03))
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
    if(m_isBrev)
    {
      for(i = 0; i < 0x20; i++)
        eeprom[i] = readEeprom(i);

#ifdef CFG_3C90X_BOOTROM_FIX
      /** Set xcvrSelect in InternalConfig in eeprom. **/
      /* only necessary for 3c905b revision cards with boot PROM bug!!! */
      writeEeprom(0x13, 0x0160);
#endif

#ifdef	CFG_3C90X_XCVR
	    /** Clear the LanWorks register **/
      if(CFG_3C90X_XCVR == 255)
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
      for(i = 0; i < 0x17; i++)
        eeprom[i] = readEeprom(i);
    }

    /** Get the hardware address */
    m_StationInfo.mac.setMac(reinterpret_cast<uint8_t*>(eeprom));
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
    if((mstat & (1 << 11)) == 0)
    {
      ERROR("3C90x: Valid link not established!");
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
    if(!m_isBrev)
      mopt &= 0x7f;

    NOTICE("3C90x connectors present:");
    c = 0;
    linktype = 0x0008;
    if(mopt & 0x01)
    {
      NOTICE(((c++) ? ", " : "") << "100BASE-T4");
      linktype = 0x0006;
    }
    if(mopt & 0x04)
    {
      NOTICE(((c++) ? ", " : "") << "100BASE-FX");
      linktype = 0x0005;
    }
    if(mopt & 0x10)
    {
      NOTICE(((c++) ? ", " : "") << "10BASE2");
      linktype = 0x0003;
    }
    if(mopt & 0x20)
    {
      NOTICE(((c++) ? ", " : "") << "AUI");
      linktype = 0x0001;
    }
    if(mopt & 0x40)
    {
      NOTICE(((c++) ? ", " : "") << "MII");
      linktype = 0x0006;
    }
    if((mopt & 0xA) == 0xA)
    {
      NOTICE(((c++) ? ", " : "") << "10BASE-T / 100BASE-TX");
      linktype = 0x0008;
    }
    else if((mopt & 0xa) == 0x2)
    {
      NOTICE(((c++) ? ", " : "") << "100BASE-TX");
      linktype = 0x0008;
    }
    else if((mopt & 0xa) == 0x8)
    {
      NOTICE(((c++) ? ", " : "") << "10BASE-T");
      linktype = 0x0008;
    }

    /** Determine transceiver type to use, depending on value stored in
      * eeprom 0x16
      */
    if(m_isBrev)
    {
      if((eeprom[0x16] & 0xff00) == XCVR_MAGIC)
        linktype = eeprom[0x16] & 0x000f;
    }
    else
    {
#ifdef CFG_3C90X_XCVR
      if(CFG_3C90X_XCVR != 255)
        linktype = CFG_3C90X_XCVR;
#endif

      if(linktype == 0x0009)
      {
        if(m_isBrev)
          WARNING("3C90x: MII External MAC mode only supported on B-revision cards! Falling back to MII mode.");
        linktype = 0x0006;
      }
    }

    /** Enable DC converter for 10-BASE-T **/
    if(linktype == 0x0003)
      issueCommand(cmdEnableDcConverter, 0);

    /** Set the link to the type we just determined **/
    setWindow(winTxRxOptions3);
    cfg = m_pBase->read32(regInternalConfig_3_l);
    cfg &= ~(0xF << 20);
    cfg |= (linktype << 20);
    m_pBase->write32(cfg, regInternalConfig_3_l);

    /** Now that we've set the xcvr type, reset TX and RX, re-enable **/
    issueCommand(cmdTxReset, 0);
    while(m_pBase->read16(regCommandIntStatus_w) & INT_CMDINPROGRESS);

    if(!m_isBrev)
      m_pBase->write8(0x01, regTxFreeThresh_b);
    issueCommand(cmdTxEnable, 0);

    /** reset of the receiver on B-revision cards re-negotiates the link
      * takes several seconds
      */
    if(m_isBrev)
      issueCommand(cmdRxReset, 0x04);
    else
      issueCommand(cmdRxReset, 0x00);
    while(m_pBase->read16(regCommandIntStatus_w) & INT_CMDINPROGRESS);

    /** Set the RX filter = receive only individual packets & multicast & broadcast **/
    issueCommand(cmdSetRxFilter, 0x01 + 0x02 + 0x04);
    issueCommand(cmdRxEnable, 0);

    /** Set indication and interrupt flags, ack any IRQs **/
    issueCommand(cmdSetInterruptEnable, 0);
    issueCommand(cmdSetIndicationEnable, 0x0014);
    issueCommand(cmdAcknowledgeInterrupt, 0x661);
}
