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
#ifndef MACHINE_ENDPOINT_H
#define MACHINE_ENDPOINT_H

#include <utilities/String.h>
#include <utilities/Vector.h>
#include <processor/types.h>
#include <machine/Network.h>
class Socket;

// Forward declaration, because Endpoint is used by ProtocolManager
class ProtocolManager;

/**
 * The Pedigree network stack - Protocol Endpoint
 *
 * Some features are global across Endpoints; they are provided here. Other
 * features are specific to connectionless and connection-based Endpoints,
 * which are handled by separate object definitions.
 */
class Endpoint
{
private:
    Endpoint(const Endpoint &e);
    const Endpoint& operator = (const Endpoint& e);
public:

    /** Type of this endpoint */
    enum EndpointType
    {
        /** Connectionless - UDP, RAW */
        Connectionless = 0,

        /** Connection-based - TCP */
        ConnectionBased
    };

    /** shutdown() parameter */
    enum ShutdownType
    {
        /** Disable sending on the Endpoint. For TCP this entails a FIN. */
        ShutSending,

        /** Disable receiving on the Endpoint. */
        ShutReceiving,

        /** Disable both sides of the Endpoint. */
        ShutBoth
    };

    /** Constructors and destructors */
    Endpoint() :
            m_Sockets(), m_LocalPort(0), m_RemotePort(0), m_LocalIp(), m_RemoteIp(),
            m_Manager(0), m_bConnection(false)
    {};
    Endpoint(uint16_t local, uint16_t remote) :
            m_Sockets(), m_LocalPort(local), m_RemotePort(remote), m_LocalIp(), m_RemoteIp(),
            m_Manager(0), m_bConnection(false)
    {};
    Endpoint(IpAddress remoteIp, uint16_t local = 0, uint16_t remote = 0) :
            m_Sockets(), m_LocalPort(local), m_RemotePort(remote), m_LocalIp(), m_RemoteIp(remoteIp),
            m_Manager(0), m_bConnection(false)
    {};
    virtual ~Endpoint() {};

    /** Endpoint type (implemented by the concrete child classes) */
    virtual EndpointType getType() = 0;

    /** Access to internal information */
    virtual uint16_t getLocalPort()
    {
        return m_LocalPort;
    }
    virtual uint16_t getRemotePort()
    {
        return m_RemotePort;
    }
    virtual IpAddress getLocalIp()
    {
        return m_LocalIp;
    }
    virtual IpAddress getRemoteIp()
    {
        return m_RemoteIp;
    }

    virtual void setLocalPort(uint16_t port)
    {
        m_LocalPort = port;
    }
    virtual void setRemotePort(uint16_t port)
    {
        m_RemotePort = port;
    }
    virtual void setLocalIp(IpAddress local)
    {
        m_LocalIp = local;
    }
    virtual void setRemoteIp(IpAddress remote)
    {
        m_RemoteIp = remote;
    }

    /**
     * Special address type, like stationInfo but with information about
     * the remote host's port for communications.
     */
    struct RemoteEndpoint
    {
        RemoteEndpoint() :
                ip(), remotePort(0)
        {};

        /// IpAddress will handle IPv6 and IPv4
        IpAddress ip;

        /// Remote host's port
        uint16_t remotePort;
    };

    /** Is data ready to receive? */
    virtual bool dataReady(bool block = false, uint32_t timeout = 30)
    {
        return false;
    };

    /** <Protocol>Manager functionality */
    virtual size_t depositPayload(size_t nBytes, uintptr_t payload, RemoteEndpoint remoteHost)
    {
        return 0;
    }

    /** All endpoint types must provide a shutdown() method that shuts part of the socket */
    virtual bool shutdown(ShutdownType what) = 0;

    /**
     * This would allow an Endpoint to switch cards to utilise the best route
     * for a return packet. I'm not sure if it's used or not.
     */
    virtual void setCard(Network* pCard)
    {
    };

    /** Protocol management */
    ProtocolManager *getManager()
    {
        return m_Manager;
    }

    void setManager(ProtocolManager *man)
    {
        m_Manager = man;
    }

    /** Connection type */
    inline bool isConnectionless()
    {
        return !m_bConnection;
    }
    
    /** Adds a socket to the internal list */
    void AddSocket(Socket *s)
    {
        m_Sockets.pushBack(s);
    }

    /** Removes a socket from the internal list */
    void RemoveSocket(Socket *s)
    {
        for(List<Socket*>::Iterator it = m_Sockets.begin(); it != m_Sockets.end(); ++it)
        {
            if(*it == s)
            {
                m_Sockets.erase(it);
                return;
            }
        }
    }

protected:

    /** List of sockets linked to this Endpoint */
    List<Socket *> m_Sockets;

private:

    /** Our local port (sourcePort in the UDP header) */
    uint16_t m_LocalPort;

    /** Our destination port */
    uint16_t m_RemotePort;

    /** Local IP. Zero means bound to all. */
    IpAddress m_LocalIp;

    /** Remote IP */
    IpAddress m_RemoteIp;

    /** Protocol manager */
    ProtocolManager *m_Manager;

protected:

    /** Connection-based?
      * Because the initialisation for any recv/send action on either type
      * of Endpoint (connected or connectionless) is the same across protocols
      * this can reduce the amount of repeated code.
      */
    bool m_bConnection;
};

#endif
