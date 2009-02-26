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

#include <Module.h>
#include <Log.h>
#include <network/NetworkStack.h>
#include <network/UdpManager.h>

#define MAX_OPTIONS_SIZE    (1500 - 28 /* UDP header + IP header */ - 236 /* DhcpPacket size, below */)

/** Defines a DHCP packet (RFC 2131) */
struct DhcpPacket
{
  uint8_t   opcode;
  uint8_t   htype;
  uint8_t   hlen;
  uint8_t   hops;
  uint32_t  xid;
  uint16_t  secs;
  uint16_t  flags;
  uint32_t  ciaddr;
  uint32_t  yiaddr;
  uint32_t  siaddr;
  uint32_t  giaddr;
  uint8_t   chaddr[16];
  uint8_t   sname[64];
  uint8_t   file[128];
  char      options[MAX_OPTIONS_SIZE];
} __attribute__((packed));

#define OP_BOOTREQUEST    1
#define OP_BOOTREPLY      2

enum DhcpMessageTypes
{
  DISCOVER = 1,
  OFFER,
  REQUEST,
  DECLINE,
  ACK,
  NACK,
  RELEASE,
  INFORM
};

/** Defines the beginning of a DHCP option */
struct DhcpOption
{
  uint8_t   code;
  uint8_t   len;
} __attribute__((packed));

/** Magic cookie (RFC1048 extensions) */
#define MAGIC_COOKIE 0x63538263
struct DhcpOptionMagicCookie
{
  DhcpOptionMagicCookie() :
    cookie(MAGIC_COOKIE)
  {};
  
  uint32_t cookie;
} __attribute__((packed));

/** Defines the Subnet-Mask DHCP option (code=1) */
#define DHCP_SUBNETMASK    1
struct DhcpOptionSubnetMask
{
  DhcpOptionSubnetMask() :
    code(DHCP_SUBNETMASK), len(4), a1(0), a2(0), a3(0), a4(0)
  {};
  
  uint8_t code;
  uint8_t len;
  uint8_t a1;
  uint8_t a2;
  uint8_t a3;
  uint8_t a4;
} __attribute__((packed));

/** Defines the Default-Gateway DHCP option (code=3) */
#define DHCP_DEFGATEWAY    3
struct DhcpOptionDefaultGateway
{
  DhcpOptionDefaultGateway() :
    code(DHCP_DEFGATEWAY), len(4), a1(0), a2(0), a3(0), a4(0)
  {};
  
  uint8_t code;
  uint8_t len;
  uint8_t a1;
  uint8_t a2;
  uint8_t a3;
  uint8_t a4;
} __attribute__((packed));

/** Defines the Adddress Request DHCP option (code=50) */
#define DHCP_ADDRREQ    50
struct DhcpOptionAddrReq
{
  DhcpOptionAddrReq() :
    code(DHCP_ADDRREQ), len(4), a1(0), a2(0), a3(0), a4(0)
  {};
  
  uint8_t code;
  uint8_t len;
  uint8_t a1;
  uint8_t a2;
  uint8_t a3;
  uint8_t a4;
} __attribute__((packed));

/** Defines the Message Type DHCP option (code=53) */
#define DHCP_MSGTYPE    53
struct DhcpOptionMsgType
{
  DhcpOptionMsgType() :
    code(DHCP_MSGTYPE), len(1), opt(DISCOVER)
  {};
  
  uint8_t   code; // = 0x53
  uint8_t   len; // = 1
  uint8_t   opt;
} __attribute__((packed));

/** Defines the Server Identifier DHCP option (code=54) */
#define DHCP_SERVERIDENT  54
struct DhcpOptionServerIdent
{
  DhcpOptionServerIdent() :
    code(DHCP_SERVERIDENT), len(4), a1(0), a2(0), a3(0), a4(0)
  {};
  
  uint8_t code;
  uint8_t len;
  uint8_t a1;
  uint8_t a2;
  uint8_t a3;
  uint8_t a4;
} __attribute__((packed));

/** Defines the End DHCP option (code=0xff) */
#define DHCP_MSGEND   0xff
struct DhcpOptionEnd
{
  DhcpOptionEnd() :
    code(DHCP_MSGEND)
  {};
  
  uint8_t   code;
} __attribute__((packed));

size_t addOption(void* opt, size_t sz, size_t offset, void* dest)
{
  memcpy(reinterpret_cast<char*>(dest) + offset, opt, sz);
  return offset + sz;
}

DhcpOption* getNextOption(DhcpOption* opt, size_t* currOffset)
{
  DhcpOption* ret = reinterpret_cast<DhcpOption*>(reinterpret_cast<uintptr_t>(opt) + (*currOffset)); // skip code and len as well
  (*currOffset) += (opt->len + 2);
  return ret;
}

void entry()
{
  // retrieve DHCP addresses for all NICs in the system
  NOTICE("DHCP Client: Iterating through all devices");
  for(size_t i = 0; i < NetworkStack::instance().getNumDevices(); i++)
  {
    NOTICE("DHCP Client: Configuring card " << Dec << i << Hex << ".");
    Network* pCard = NetworkStack::instance().getDevice(i);
    StationInfo info = pCard->getStationInfo();
  
    IpAddress broadcast(0xffffffff);
    Endpoint* e = UdpManager::instance().getEndpoint(broadcast, 68, 67);
        
    #define BUFFSZ 2048
    uint8_t* buff = new uint8_t[BUFFSZ];
    
    // can be used for anything
    DhcpPacket dhcp;
    
    enum DhcpState
    {
      DISCOVER_SENT = 0, // we've sent DHCP DISCOVER
      OFFER_RECVD, // we've received an offer
      REQUEST_SENT, // we've sent the request
      ACK_RECVD, // we've received the final ACK
      UNKNOWN
    } currentState;
      
    
    if(e)
    {
      // DHCP DISCOVER to find potential DHCP servers
      e->acceptAnyAddress(true);
      
      Endpoint::RemoteEndpoint remoteHost;
      remoteHost.remotePort = 67;
      remoteHost.ip = broadcast;
      
      currentState = DISCOVER_SENT;
      
      // zero out and fill the initial packet
      memset(&dhcp, 0, sizeof(DhcpPacket));
      dhcp.opcode = OP_BOOTREQUEST;
      dhcp.htype = 1; // ethernet
      dhcp.hlen = 6; // 6 bytes in a MAC address
      dhcp.xid = HOST_TO_BIG32(12345);
      memcpy(dhcp.chaddr, info.mac, 6);
      
      DhcpOptionMagicCookie cookie;
      DhcpOptionMsgType msgTypeOpt;
      DhcpOptionEnd endOpt;
      
      msgTypeOpt.opt = DISCOVER;
      
      size_t byteOffset = 0;
      byteOffset = addOption(&cookie, sizeof(cookie), byteOffset, dhcp.options);
      byteOffset = addOption(&msgTypeOpt, sizeof(msgTypeOpt), byteOffset, dhcp.options);
      byteOffset = addOption(&endOpt, sizeof(endOpt), byteOffset, dhcp.options);

      // throw into the send buffer and send it out
      memcpy(buff, &dhcp, sizeof(dhcp));
      bool success = e->send((sizeof(dhcp) - MAX_OPTIONS_SIZE) + byteOffset, reinterpret_cast<uintptr_t>(buff), remoteHost, true, pCard);
      if(!success)
      {
        WARNING("Couldn't send DHCP DISCOVER packet on interface " << i << "!");
        UdpManager::instance().returnEndpoint(e);
        continue;
      }
      
      // Find and respond to the first DHCP offer
      
      uint32_t myIpWillBe = 0;
      DhcpOptionServerIdent dhcpServer;
      
      size_t n = 0;
      if(e->dataReady(true) == false)
      {
        WARNING("Did not receive a reply to DHCP DISCOVER (timed out), interface " << i << "!");
        UdpManager::instance().returnEndpoint(e);
        continue;
      }
      while((n = e->recv(reinterpret_cast<uintptr_t>(buff), BUFFSZ, &remoteHost)))
      {
        DhcpPacket* incoming = reinterpret_cast<DhcpPacket*>(buff);
        
        if(incoming->opcode != OP_BOOTREPLY)
          continue;
        
        myIpWillBe = incoming->yiaddr;
        
        size_t dhcpSizeWithoutOptions = n - MAX_OPTIONS_SIZE;
        if(dhcpSizeWithoutOptions == 0)
        {
          // no magic cookie, so extensions aren't available
          dhcpServer.a4 = (incoming->siaddr & 0xFF000000) >> 24;
          dhcpServer.a3 = (incoming->siaddr & 0x00FF0000) >> 16;
          dhcpServer.a2 = (incoming->siaddr & 0x0000FF00) >> 8;
          dhcpServer.a1 = (incoming->siaddr & 0x000000FF);
          currentState = OFFER_RECVD;
          break;
        }
        DhcpOption* opt = reinterpret_cast<DhcpOption*>(incoming->options + sizeof(cookie));
        DhcpOptionMagicCookie thisCookie = *reinterpret_cast<DhcpOptionMagicCookie*>(incoming->options);
        if(thisCookie.cookie != MAGIC_COOKIE)
        {
          NOTICE("Magic cookie incorrect!");
          break;
        }
        
        // check the options for the magic cookie and DHCP OFFER
        byteOffset = 0;
        while((byteOffset + sizeof(cookie) + (sizeof(DhcpPacket) - MAX_OPTIONS_SIZE)) < sizeof(DhcpPacket))
        {
          opt = reinterpret_cast<DhcpOption*>(incoming->options + byteOffset + sizeof(cookie));
          if(opt->code == DHCP_MSGEND)
            break;
          else if(opt->code == DHCP_MSGTYPE)
          {
            DhcpOptionMsgType* p = reinterpret_cast<DhcpOptionMsgType*>(opt);
            if(p->opt == OFFER)
              currentState = OFFER_RECVD;
          }
          else if(opt->code == DHCP_SERVERIDENT)
          {
            DhcpOptionServerIdent* p = reinterpret_cast<DhcpOptionServerIdent*>(opt);
            dhcpServer = *p;
          }
          
          byteOffset += opt->len + 2;
        }
        
        // if YOUR-IP is set and no DHCP Message Type was given, assume this is an offer
        if(myIpWillBe && (currentState != OFFER_RECVD))
          currentState = OFFER_RECVD;
      }
            
      if(currentState != OFFER_RECVD)
      {
        WARNING("Couldn't get a valid offer packet.");
        UdpManager::instance().returnEndpoint(e);
        continue;
      }
      
      // We want to accept this offer by requesting it from the DHCP server
            
      currentState = REQUEST_SENT;
      
      DhcpOptionAddrReq addrReq;
      
      addrReq.a4 = (myIpWillBe & 0xFF000000) >> 24;
      addrReq.a3 = (myIpWillBe & 0x00FF0000) >> 16;
      addrReq.a2 = (myIpWillBe & 0x0000FF00) >> 8;
      addrReq.a1 = (myIpWillBe & 0x000000FF);
      
      msgTypeOpt.opt = REQUEST;
      
      byteOffset = 0;
      byteOffset = addOption(&cookie, sizeof(cookie), byteOffset, dhcp.options);
      byteOffset = addOption(&msgTypeOpt, sizeof(msgTypeOpt), byteOffset, dhcp.options);
      byteOffset = addOption(&addrReq, sizeof(addrReq), byteOffset, dhcp.options);
      byteOffset = addOption(&dhcpServer, sizeof(dhcpServer), byteOffset, dhcp.options);
      byteOffset = addOption(&endOpt, sizeof(endOpt), byteOffset, dhcp.options);
      
      // throw into the send buffer and send it out
      memcpy(buff, &dhcp, sizeof(dhcp));
      success = e->send((sizeof(dhcp) - MAX_OPTIONS_SIZE) + byteOffset, reinterpret_cast<uintptr_t>(buff), remoteHost, true, pCard);
      if(!success)
      {
        WARNING("Couldn't send DHCP REQUEST packet on interface " << i << "!");
        UdpManager::instance().returnEndpoint(e);
        continue;
      }
      
      // Grab the ACK and update the card
      
      DhcpOptionSubnetMask subnetMask;
      bool subnetMaskSet = false;
      
      DhcpOptionDefaultGateway defGateway;
      bool gatewaySet = false;
      
      if(e->dataReady(true) == false)
      {
        WARNING("Did not receive a reply to DHCP REQUST (timed out), interface " << i << "!");
        UdpManager::instance().returnEndpoint(e);
        continue;
      }
      while((n = e->recv(reinterpret_cast<uintptr_t>(buff), BUFFSZ, &remoteHost)))
      {        
        DhcpPacket* incoming = reinterpret_cast<DhcpPacket*>(buff);
        
        if(incoming->opcode != OP_BOOTREPLY)
          continue;
        
        myIpWillBe = incoming->yiaddr;
        
        size_t dhcpSizeWithoutOptions = n - MAX_OPTIONS_SIZE;
        if(dhcpSizeWithoutOptions == 0)
        {
          // no magic cookie, so extensions aren't available
          currentState = ACK_RECVD;
          break;
        }
        DhcpOption* opt = reinterpret_cast<DhcpOption*>(incoming->options + sizeof(cookie));
        DhcpOptionMagicCookie thisCookie = *reinterpret_cast<DhcpOptionMagicCookie*>(incoming->options);
        if(thisCookie.cookie != MAGIC_COOKIE)
        {
          NOTICE("Magic cookie incorrect!");
          break;
        }
        
        // check the options for the magic cookie and DHCP OFFER
        byteOffset = 0;
        while((byteOffset + sizeof(cookie) + (sizeof(DhcpPacket) - MAX_OPTIONS_SIZE)) < sizeof(DhcpPacket))
        {
          opt = reinterpret_cast<DhcpOption*>(incoming->options + byteOffset + sizeof(cookie));
          if(opt->code == DHCP_MSGEND)
            break;
          else if(opt->code == DHCP_MSGTYPE)
          {
            DhcpOptionMsgType* p = reinterpret_cast<DhcpOptionMsgType*>(opt);
            if(p->opt == ACK)
              currentState = ACK_RECVD;
          }
          else if(opt->code == DHCP_SUBNETMASK)
          {
            DhcpOptionSubnetMask* p = reinterpret_cast<DhcpOptionSubnetMask*>(opt);
            subnetMask = *p;
            subnetMaskSet = true;
          }
          else if(opt->code == DHCP_DEFGATEWAY)
          {
            DhcpOptionDefaultGateway* p = reinterpret_cast<DhcpOptionDefaultGateway*>(opt);
            defGateway = *p;
            gatewaySet = true;
          }
          
          byteOffset += opt->len + 2;
        }
        
        // if YOUR-IP is set and no DHCP Message Type was given, assume this is an ack
        if(myIpWillBe && (currentState != ACK_RECVD))
          currentState = ACK_RECVD;
      }
      
      if(currentState != ACK_RECVD)
      {
        WARNING("Couldn't get a valid ack packet.");
        UdpManager::instance().returnEndpoint(e);
        continue;
      }
      
      // set it up
      StationInfo host;
      host.ipv4.setIp(Network::convertToIpv4(addrReq.a1, addrReq.a2, addrReq.a3, addrReq.a4));
      
      if(subnetMaskSet)
        host.subnetMask.setIp(Network::convertToIpv4(subnetMask.a1, subnetMask.a2, subnetMask.a3, subnetMask.a4));
      else
      {
        NOTICE("Subnet mask fail");
        host.subnetMask.setIp(Network::convertToIpv4(255, 255, 255, 0));
      }
      if(gatewaySet)
        host.gateway.setIp(Network::convertToIpv4(defGateway.a1, defGateway.a2, defGateway.a3, defGateway.a4));
      else
      {
        NOTICE("Gateway fail");
        host.gateway.setIp(Network::convertToIpv4(192, 168, 0, 1)); /// \todo Autoconfiguration IPv4 address
      }
      pCard->setStationInfo(host);
      
      UdpManager::instance().returnEndpoint(e);
    }
    
    delete [] buff;
  }
  NOTICE("DHCP Client: Complete");
}

void exit()
{
}

MODULE_NAME("dhcpclient");
MODULE_ENTRY(&entry);
MODULE_EXIT(&exit);
MODULE_DEPENDS("NetworkStack");
