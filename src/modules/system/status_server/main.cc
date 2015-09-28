/*
 * Copyright (c) 2008-2014, Pedigree Developers
 *
 * Please see the CONTRIB file in the root of the source tree for a full
 * list of contributors.
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
#include <Version.h>
#include <machine/Network.h>
#include <processor/Processor.h>
#include <network-stack/Endpoint.h>
#include <network-stack/TcpManager.h>
#include <network-stack/RoutingTable.h>
#include <network-stack/NetworkStack.h>
#include <network-stack/ConnectionBasedEndpoint.h>
#include <vfs/VFS.h>
#include <vfs/Filesystem.h>

#define LISTEN_PORT     1234

int clientThread(void *p)
{
    if(!p)
        return 0;

    ConnectionBasedEndpoint *pClient = reinterpret_cast<ConnectionBasedEndpoint*>(p);

    // Wait for data from the client - block.
    if(!pClient->dataReady(true))
    {
        WARNING("Client from IP: " << pClient->getRemoteIp().toString() << " wasn't a proper HTTP client.");
        pClient->close();
        return 0;
    }

    // Read all incoming data from the client. Assume no nulls.
    /// \todo Wait until last 4 bytes are \r\n\r\n - that's the end of the HTTP header.
    String inputData("");
    while(pClient->dataReady(false))
    {
        char buff[512];
        pClient->recv(reinterpret_cast<uintptr_t>(buff), 512, true, false);
        inputData += buff;
    }

    // No longer want any content from the server
    pClient->shutdown(Endpoint::ShutReceiving);

    bool bHeadRequest = false;

    List<String*> firstLine = inputData.tokenise(' ');
    String operation = *firstLine.popFront();
    if(!(operation == String("GET")))
    {
        if(operation == String("HEAD"))
            bHeadRequest = true;
        else
        {
            NOTICE("Unsupported HTTP method: '" << operation << "'");
            pClient->close();
            return 0;
        }
    }

    bool bNotFound = false;

    String path = *firstLine.popFront();
    if(!(path == String("/")))
        bNotFound = true;

    // Got a heap of information now - prepare to return
    size_t code = bNotFound ? 404 : 200;
    NormalStaticString statusLine;
    statusLine = "HTTP/1.0 ";
    statusLine += code;
    statusLine += " ";
    statusLine += bNotFound ? "Not Found" : "OK";

    // Build the response
    String response;
    response = statusLine;
    response += "\r\nContent-type: text/html";
    response += "\r\nConnection: close";
    response += "\r\n\r\n";

    // Do we need data there?
    if(!bHeadRequest)
    {
        // Formulate data itself
        if(bNotFound)
        {
            response += "Error 404: Page not found.";
        }
        else
        {
            response += "<html><head><title>Pedigree - Live System Status Report</title></head><body>";
            response += "<h1>Pedigree Live Status Report</h1>";
            response += "<p>This is a live status report from a running Pedigree system.</p>";
            response += "<h3>Current Build</h3><pre>";

            {
                HugeStaticString str;
                str += "Pedigree - revision ";
                str += g_pBuildRevision;
                str += "<br />===========================<br />Built at ";
                str += g_pBuildTime;
                str += " by ";
                str += g_pBuildUser;
                str += " on ";
                str += g_pBuildMachine;
                str += "<br />Build flags: ";
                str += g_pBuildFlags;
                str += "<br />";
                response += str;
            }

            response += "</pre>";

            response += "<h3>Network Interfaces</h3>";
            response += "<table border='1'><tr><th>Interface #</th><th>IP Address</th><th>Subnet Mask</th><th>Gateway</th><th>DNS Servers</th><th>Driver Name</th><th>MAC address</th><th>Statistics</th></tr>";
            size_t i;
            for (i = 0; i < NetworkStack::instance().getNumDevices(); i++)
            {
                Network* card = NetworkStack::instance().getDevice(i);
                StationInfo info = card->getStationInfo();

                // Interface number
                response += "<tr><td>";
                NormalStaticString s;
                s.append(i);
                response += s;
                if(card == RoutingTable::instance().DefaultRoute())
                    response += " <b>(default route)</b>";
                response += "</td>";

                // IP address
                response += "<td>";
                response += info.ipv4.toString();
                if(info.nIpv6Addresses)
                {
                    for(size_t nAddress = 0; nAddress < info.nIpv6Addresses; nAddress++)
                    {
                        response += "<br />";
                        response += info.ipv6[nAddress].toString();
                    }
                }
                response += "</td>";

                // Subnet mask
                response += "<td>";
                response += info.subnetMask.toString();
                response += "</td>";

                // Gateway
                response += "<td>";
                response += info.gateway.toString();
                response += "</td>";

                // DNS servers
                response += "<td>";
                if(!info.nDnsServers)
                    response += "(none)";
                else
                {
                    for(size_t j = 0; j < info.nDnsServers; j++)
                    {
                        response += info.dnsServers[j].toString();
                        if((j + 1) < info.nDnsServers)
                            response += "<br />";
                    }
                }
                response += "</td>";

                // Driver name
                response += "<td>";
                String cardName; card->getName(cardName);
                response += cardName;
                response += "</td>";

                // MAC
                response += "<td>";
                response += info.mac.toString();
                response += "</td>";

                // Statistics
                response += "<td>";
                s.clear();
                s += "Packets: ";
                s.append(info.nPackets);
                s += "<br />Dropped: ";
                s.append(info.nDropped);
                s += "<br />RX Errors: ";
                s.append(info.nBad);
                response += s;
                response += "</td>";

                response += "</tr>";
            }
            response += "</table>";

            response += "<h3>VFS</h3>";
            response += "<table border='1'><tr><th>VFS Alias</th><th>Disk</th></tr>";

            typedef List<String*> StringList;
            typedef Tree<Filesystem*, List<String*>*> VFSMountTree;

            VFSMountTree &mounts = VFS::instance().getMounts();

            for(VFSMountTree::Iterator i = mounts.begin();
                i != mounts.end();
                i++)
            {
                Filesystem *pFs = i.key();
                StringList *pList = i.value();
                Disk *pDisk = pFs->getDisk();

                for(StringList::Iterator j = pList->begin();
                    j != pList->end();
                    j++)
                {
                    String mount = **j;
                    String diskInfo, temp;

                    if(pDisk)
                    {
                        pDisk->getName(temp);
                        pDisk->getParent()->getName(diskInfo);

                        diskInfo += " -- ";
                        diskInfo += temp;
                    }
                    else
                        diskInfo = "(no disk)";

                    response += "<tr><td>";
                    response += mount;
                    response += "</td><td>";
                    response += diskInfo;
                    response += "</td></tr>";
                }
            }

            response += "</table>";
            response += "</body></html>";
        }
    }

    // Send to the client
    pClient->send(response.length(), reinterpret_cast<uintptr_t>(static_cast<const char*>(response)));

    // Nothing more to send, bring down our side of the connection
    pClient->shutdown(Endpoint::ShutSending);

    pClient->shutdown(Endpoint::ShutBoth);

    // Leave the endpoint to be cleaned up when the connection is shut down cleanly
    /// \todo Does it get cleaned up if we do this?
    return 0;
}

int mainThread(void *p)
{
    ConnectionBasedEndpoint *pEndpoint = reinterpret_cast<ConnectionBasedEndpoint *>(p);

    pEndpoint->listen();

    while(1)
    {
        ConnectionBasedEndpoint *pClient = static_cast<ConnectionBasedEndpoint*>(pEndpoint->accept());
        if(!pClient)
            continue;
        Thread *pThread = new Thread(Processor::information().getCurrentThread()->getParent(), clientThread, pClient);
        pThread->detach();
    }

    return 0;
}

static bool init()
{
    ConnectionBasedEndpoint *pEndpoint = static_cast<ConnectionBasedEndpoint*>(TcpManager::instance().getEndpoint(LISTEN_PORT, RoutingTable::instance().DefaultRoute()));
    if(!pEndpoint)
    {
        WARNING("Status server can't start, couldn't get a TCP endpoint.");
        return false;
    }

    Thread *pThread = new Thread(Processor::information().getCurrentThread()->getParent(), mainThread, pEndpoint);
    pThread->detach();
    return true;
}

static void destroy()
{
}

MODULE_INFO("Status Server", &init, &destroy, "network-stack", "init", "vfs", "config");
