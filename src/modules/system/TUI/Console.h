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
#ifndef SUBSYS_CONSOLE_H
#define SUBSYS_CONSOLE_H

#include <console/Console.h>
#include <utilities/RequestQueue.h>

#define TUI_MODE_CHANGED 98
#define TUI_CHAR_RECV 99

class TuiRequestEvent : public Event
{
    public:
        TuiRequestEvent(size_t req, uintptr_t buff, size_t maxSz, size_t reqId, size_t tabId, uintptr_t handlerAddress) :
            Event(handlerAddress, true, req == CONSOLE_READ ? ~0 : 0), m_Req(req), m_Buff(buff),
            m_MaxSz(maxSz), m_ReqId(reqId), m_TabId(tabId)
        {};
        virtual ~TuiRequestEvent()
        {};

        virtual inline size_t getNumber()
        {
            return EventNumbers::TuiEvent;
        }

        virtual size_t serialize(uint8_t *pBuffer)
        {
            size_t offset = 0;

            memcpy(&pBuffer[offset], &m_Req, sizeof(size_t));
            offset += sizeof(size_t);

            memcpy(&pBuffer[offset], &m_Buff, sizeof(uintptr_t));
            offset += sizeof(size_t);

            memcpy(&pBuffer[offset], &m_MaxSz, sizeof(size_t));
            offset += sizeof(size_t);

            memcpy(&pBuffer[offset], &m_ReqId, sizeof(size_t));
            offset += sizeof(size_t);

            memcpy(&pBuffer[offset], &m_TabId, sizeof(size_t));
            offset += sizeof(size_t);

            return offset;
        }

        static bool unserialize(uint8_t *pBuffer, Event &event)
        {
            return true;
        }
    private:

        size_t m_Req;
        uintptr_t m_Buff;
        size_t m_MaxSz;
        size_t m_ReqId;
        size_t m_TabId;
};

class UserConsole : public RequestQueue
{
public:
    UserConsole();
    ~UserConsole();

    /** Overrides addRequest in order to send the event directly to userspace */
    virtual uint64_t addRequest(size_t priority, uint64_t p1=0, uint64_t p2=0, uint64_t p3=0, uint64_t p4=0, uint64_t p5=0,
                        uint64_t p6=0, uint64_t p7=0, uint64_t p8=0);

    /** Asynchronous requests should not be used for the TUI */
    virtual uint64_t addAsyncRequest(size_t priority, uint64_t p1=0, uint64_t p2=0, uint64_t p3=0, uint64_t p4=0, uint64_t p5=0,
                             uint64_t p6=0, uint64_t p7=0, uint64_t p8=0)
    {
        FATAL("Asynchronous request attempted in the TUI");
        return 0;
    }

    /** Block and wait for a request. */
    size_t nextRequest(size_t responseToLast, char *buffer, size_t *sz, size_t buffsz, size_t *tabId);

    /** Set the currently executing request as "pending" - its value will be
        set later. */
    void requestPending();

    /** Respond to the previously set pending request. */
    void respondToPending(size_t response, char *buffer, size_t sz);

    /** Stops the current block for a request, if necessary */
    void stopCurrentBlock();

    /** Sets up the event handlers for incoming requests */
    void setupHandlers(uintptr_t readHandler, uintptr_t writeHandler, uintptr_t miscHandler)
    {
        m_TuiReadHandler = readHandler;
        m_TuiWriteHandler = writeHandler;
        m_TuiMiscHandler = miscHandler;
        m_pTuiThread = Processor::information().getCurrentThread();
    }

    /** Sets the return value for a given request */
    void setReturnValue(size_t reqId, size_t returnVal)
    {
        TuiRequest *pReq = m_ActiveRequests.lookup(reqId);
        if(pReq)
        {
            pReq->returnValue = returnVal;
            pReq->mutex.release();
        }
    }

private:
    virtual uint64_t executeRequest(uint64_t p1, uint64_t p2, uint64_t p3, uint64_t p4, uint64_t p5, uint64_t p6, uint64_t p7, uint64_t p8)
    {return 0;}

    UserConsole(const UserConsole&);
    const UserConsole& operator = (const UserConsole&);

    Request *m_pReq;
    Request *m_pPendingReq;

    uintptr_t m_TuiReadHandler;
    uintptr_t m_TuiWriteHandler;
    uintptr_t m_TuiMiscHandler;
    Thread *m_pTuiThread;

    struct TuiRequest
    {
        TuiRequest() : id(0), reqType(0), buffer(0), bufferSize(0), returnValue(0), mutex(true) {};
        virtual ~TuiRequest() {};

        size_t id;
        size_t reqType;
        uintptr_t buffer;
        size_t bufferSize;
        size_t returnValue; // Set when returning from userspace
        Mutex mutex;
    };

    size_t m_NextRequestID;
    Tree<size_t, TuiRequest*> m_ActiveRequests;
};

#endif
