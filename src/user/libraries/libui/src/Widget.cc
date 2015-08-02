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

#include <Widget.h>

#include <ipc/Ipc.h>

/// \todo GTFO libc!
#include <unistd.h>

/// \todo GTFO libc!
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <syslog.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/un.h>

#ifdef TARGET_LINUX
#include <bsd/string.h>  // strlcpy
#endif

#include <map>
#include <queue>

#include "protocol.h"

using namespace LibUiProtocol;

#define PEDIGREE_WINMAN_ENDPOINT "pedigree-winman"

/// Default event handler for new widgets.
static bool defaultEventHandler(WidgetMessages message, size_t dataSize, const void *dataBuffer)
{
    return false;
}

std::map<uint64_t, Widget *> Widget::s_KnownWidgets;
std::queue<char *> Widget::s_PendingMessages;
size_t Widget::s_NumWidgets;

Widget::Widget() :
    m_bConstructed(false), m_pFramebuffer(0), m_Handle(0),
    m_EventCallback(defaultEventHandler),
    m_Socket(-1), m_SharedFramebuffer(0)
{
    ++s_NumWidgets;
}

Widget::~Widget()
{
    destroy();

    --s_NumWidgets;

    // Wipe out any pending messages that might be left behind.
    if (!s_NumWidgets)
    {
        while (!Widget::s_PendingMessages.empty())
        {
            char *p = Widget::s_PendingMessages.front();
            delete [] p;
            Widget::s_PendingMessages.pop();
        }
    }
}

bool Widget::construct(const char *endpoint, const char *title, widgetCallback_t cb, PedigreeGraphics::Rect &dimensions)
{
    if(m_Handle)
    {
        // Already constructed!
        return false;
    }

    /// \todo Maybe we can get a decent way of having a default handler?
    if(!cb)
        return false;

    struct sockaddr_un meaddr;
    memset(&meaddr, 0, sizeof(meaddr));
    meaddr.sun_family = AF_UNIX;
    sprintf(meaddr.sun_path, CLIENT_SOCKET_BASE, endpoint);

    struct sockaddr_un saddr;
    memset(&saddr, 0, sizeof(saddr));
    saddr.sun_family = AF_UNIX;
    strcpy(saddr.sun_path, WINMAN_SOCKET_PATH);

    // Create socket for window manager communication.
    m_Socket = socket(AF_UNIX, SOCK_DGRAM, 0);
    if(m_Socket < 0)
        return false;

    // Connect to window manager.
    if(bind(m_Socket, (struct sockaddr *) &meaddr, sizeof(meaddr)) != 0)
    {
        syslog(LOG_ALERT, "widget bind failed: %s", strerror(errno));
        close(m_Socket);
        m_Socket = -1;
        return false;
    }
    if(connect(m_Socket, (struct sockaddr *) &saddr, sizeof(saddr)) != 0)
    {
        syslog(LOG_ALERT, "widget connection failed: %s", strerror(errno));
        close(m_Socket);
        m_Socket = -1;
        return false;
    }

    // Construct the handle first.
    uint64_t pid = getpid();
    uintptr_t widgetPointer = reinterpret_cast<uintptr_t>(this);
#ifdef BITS_64
    m_Handle = (pid << 32) | (((widgetPointer >> 32) | (widgetPointer & 0xFFFFFFFF)) & 0xFFFFFFFF);
#else
    m_Handle = (pid << 32) | (widgetPointer);
#endif

    // Prepare a message to send.
    size_t totalSize = sizeof(WindowManagerMessage) + sizeof(CreateMessage);
    char *messageData = new char[totalSize];
    memset(messageData, 0, totalSize);
    WindowManagerMessage *pWinMan = reinterpret_cast<WindowManagerMessage*>(messageData);
    CreateMessage *pCreate = reinterpret_cast<CreateMessage*>(messageData + sizeof(WindowManagerMessage));

    // Fill.
    pWinMan->messageCode = Create;
    pWinMan->messageSize = sizeof(CreateMessage);
    pWinMan->widgetHandle = m_Handle;
    pWinMan->isResponse = false;
    strlcpy(pCreate->endpoint, endpoint, 256);
    strlcpy(pCreate->title, title, 256);
    pCreate->minWidth = dimensions.getW();
    pCreate->minHeight = dimensions.getH();
    pCreate->rigid = true;

    // Send the message off to the window manager and wait for a response. The
    // response will contain a shared memory region we can use as our window
    // framebuffer.
    send(m_Socket, messageData, totalSize, 0);
    delete [] messageData;

    s_KnownWidgets.insert(std::make_pair(m_Handle, this));

    // Wait for the ACK.
    Widget::checkForMessage(Create, true);

    m_EventCallback = cb;

    // Wait for the initial Reposition message to come in, as it is needed to
    // properly allocate a window buffer.
    Widget::checkForMessage(LibUiProtocol::Reposition, false);

    // Handle any remaining messages before we consider ourselves constructed.
    Widget::checkForEvents(true);

    return true;
}

bool Widget::setProperty(std::string propName, void *propVal, size_t maxSize)
{
    // Constructed yet?
    if(!m_Handle)
        return false;

    // Are the arguments sane?
    if(!propVal)
        return false;
    if(maxSize > 0x1000)
        return false;

    // Allocate the message.
    /// \todo Sanitise maxSize once large messages can be sent.
    ssize_t headerSize = sizeof(WindowManagerMessage) + sizeof(SetPropertyMessage);
    ssize_t totalSize = headerSize + maxSize;
    char *messageData = new char[totalSize];
    WindowManagerMessage *pWinMan = reinterpret_cast<WindowManagerMessage*>(messageData);
    SetPropertyMessage *pMessage = reinterpret_cast<SetPropertyMessage*>(messageData + sizeof(WindowManagerMessage));

    // Fill the message.
    pWinMan->messageCode = SetProperty;
    pWinMan->messageSize = sizeof(SetPropertyMessage) + maxSize;
    pWinMan->widgetHandle = m_Handle;
    pWinMan->isResponse = false;

    propName.copy(pMessage->propertyName, sizeof pMessage->propertyName);
    pMessage->valueLength = maxSize;

    // Copy in the property data.
    memcpy(messageData + headerSize, propVal, maxSize);

    // Transmit.
    bool result = send(m_Socket, messageData, totalSize, 0) == totalSize;

    // Clean up.
    delete [] messageData;

    return result;
}

bool Widget::getProperty(std::string propName, char **buffer, size_t maxSize)
{
    return false;
}

bool Widget::setTitle(const std::string &newTitle)
{
    // Constructed yet?
    if(!m_Handle)
        return false;

    size_t titleLength = newTitle.length();
    if(titleLength > 511)
        titleLength = 511;

    // Allocate the message.
    ssize_t totalSize = sizeof(WindowManagerMessage) + sizeof(SetTitleMessage);
    char *messageData = new char[totalSize];
    memset(messageData, 0, totalSize);
    WindowManagerMessage *pWinMan = reinterpret_cast<WindowManagerMessage*>(messageData);
    SetTitleMessage *pMessage = reinterpret_cast<SetTitleMessage*>(messageData + sizeof(WindowManagerMessage));

    // Fill the message.
    pWinMan->messageCode = SetTitle;
    pWinMan->messageSize = titleLength;
    pWinMan->widgetHandle = m_Handle;
    pWinMan->isResponse = false;

    newTitle.copy(pMessage->newTitle, sizeof pMessage->newTitle);

    // Transmit.
    bool result = send(m_Socket, messageData, totalSize, 0) == totalSize;

    // Clean up.
    delete [] messageData;

    return result;
}

void Widget::setParent(Widget *pWidget)
{
}

Widget *Widget::getParent()
{
    return 0;
}

bool Widget::redraw(PedigreeGraphics::Rect &rt)
{
    // Constructed yet?
    if(!m_Handle)
        return false;

    ssize_t totalSize = sizeof(WindowManagerMessage) + sizeof(RequestRedrawMessage);
    char *messageData = new char[totalSize];
    memset(messageData, 0, totalSize);
    WindowManagerMessage *pWinMan = reinterpret_cast<WindowManagerMessage*>(messageData);
    RequestRedrawMessage *pMessage = reinterpret_cast<RequestRedrawMessage*>(messageData + sizeof(WindowManagerMessage));

    // Fill the message.
    pWinMan->messageCode = RequestRedraw;
    pWinMan->messageSize = sizeof(RequestRedrawMessage);
    pWinMan->widgetHandle = m_Handle;
    pWinMan->isResponse = false;
    pMessage->x = rt.getX();
    pMessage->y = rt.getY();
    pMessage->width = rt.getW();
    pMessage->height = rt.getH();

    // Transmit.
    bool bRet = false;
    bRet = send(m_Socket, messageData, totalSize, 0) == totalSize;
    delete [] messageData;

    // Wait for an ACK message.
    Widget::checkForMessage(LibUiProtocol::RequestRedraw, true);

    // Now that we have completed the redraw, handle any pending messages.
    // For example, the reason we are waiting for the redraw could be because
    // the window manager is handling keyboard events, and each of those events
    // is causing a packet to be sent. If we don't handle that, a select() on
    // our socket will not reflect the pending messages, and input will appear
    // to get eaten.
    handlePendingMessages();

    return bRet;
}

bool Widget::visibility(bool vis)
{
    // Constructed yet?
    if(!m_Handle)
        return false;

    // Allocate the message.
    ssize_t totalSize = sizeof(WindowManagerMessage) + sizeof(SetVisibilityMessage);
    char *messageData = new char[totalSize];
    memset(messageData, 0, totalSize);
    WindowManagerMessage *pWinMan = reinterpret_cast<WindowManagerMessage*>(messageData);
    SetVisibilityMessage *pMessage = reinterpret_cast<SetVisibilityMessage*>(messageData + sizeof(WindowManagerMessage));

    // Fill the message.
    pWinMan->messageCode = SetVisibility;
    pWinMan->messageSize = sizeof(SetVisibilityMessage);
    pWinMan->widgetHandle = m_Handle;
    pWinMan->isResponse = false;
    pMessage->bVisible = vis;

    // Transmit.
    bool result = send(m_Socket, messageData, totalSize, 0) == totalSize;

    // Clean up.
    delete [] messageData;

    return result;
}

void Widget::destroy()
{
    // Constructed yet (or already destroyed)?
    if(!m_Handle)
        return;

    // Allocate the message.
    ssize_t totalSize = sizeof(WindowManagerMessage) + sizeof(DestroyMessage);
    char *messageData = new char[totalSize];
    memset(messageData, 0, totalSize);
    WindowManagerMessage *pWinMan = reinterpret_cast<WindowManagerMessage*>(messageData);
    DestroyMessage *pMessage = reinterpret_cast<DestroyMessage*>(messageData + sizeof(WindowManagerMessage));

    // Fill the message.
    pWinMan->messageCode = Destroy;
    pWinMan->messageSize = sizeof(DestroyMessage);
    pWinMan->widgetHandle = m_Handle;
    pWinMan->isResponse = false;

    // Transmit.
    bool bRet = send(m_Socket, messageData, totalSize, 0) == totalSize;

    // Clean up.
    delete [] messageData;

    // Wait for an ACK message, before we return.
    // At this point, we will be completely without a framebuffer.
    Widget::checkForMessage(LibUiProtocol::Destroy, true);

    // Invalidate this widget now.
    delete m_pFramebuffer;
    delete m_SharedFramebuffer;
    m_pFramebuffer = 0;
    m_SharedFramebuffer = 0;
    m_Handle = 0;
}

PedigreeGraphics::Framebuffer *Widget::getFramebuffer()
{
    return m_pFramebuffer;
}

void Widget::checkForEvents(bool bAsync)
{
    bool bContinue = true;

    int max_fd = 0;
    fd_set fds;
    FD_ZERO(&fds);

    while(bContinue)
    {
        // Check for pending messages that we could handle easily.
        if (Widget::handlePendingMessages())
            return;  // Messages were handled, we're done here.

        char *buffer = new char[4096];

        for (std::map<uint64_t, Widget *>::iterator it = s_KnownWidgets.begin();
             it != s_KnownWidgets.end();
             ++it)
        {
            Widget *pWidget = it->second;
            max_fd = std::max(max_fd, pWidget->getSocket());
            FD_SET(pWidget->getSocket(), &fds);
        }

        struct timeval tv;
        tv.tv_sec = tv.tv_usec = 0;

        // Async - check and don't do anything if no message found.
        int nready = select(max_fd + 1, &fds, 0, 0, bAsync ? &tv : 0);
        if(nready)
        {
            for (int i = 0; i <= max_fd; ++i)
            {
                if (FD_ISSET(i, &fds))
                {
                    recv(i, buffer, 4096, 0);
                    bContinue = false;
                    break;
                }
            }
        }
        else if(bAsync)
        {
            delete [] buffer;
            break;
        }

        Widget::handleMessage(buffer);
        delete [] buffer;
    }
}

void Widget::checkForMessage(size_t which, bool bResponse)
{
    bool bContinue = true;

    // Check for one that might be hiding in the pending messages queue.
    size_t messagesToCheck = s_PendingMessages.size();
    for (; messagesToCheck > 0; --messagesToCheck)
    {
        char *buffer = s_PendingMessages.front();
        s_PendingMessages.pop();

        LibUiProtocol::WindowManagerMessage *pHeader =
            reinterpret_cast<LibUiProtocol::WindowManagerMessage*>(buffer);
        if (pHeader->messageCode == which && pHeader->isResponse == bResponse)
        {
            handleMessage(buffer);
            delete [] buffer;
            return;
        }

        s_PendingMessages.push(buffer);
    }

    int max_fd = 0;
    fd_set fds;
    FD_ZERO(&fds);

    while (bContinue)
    {
        char *buffer = new char[4096];

        for (std::map<uint64_t, Widget *>::iterator it = s_KnownWidgets.begin();
             it != s_KnownWidgets.end();
             ++it)
        {
            Widget *pWidget = it->second;
            max_fd = std::max(max_fd, pWidget->getSocket());
            FD_SET(pWidget->getSocket(), &fds);
        }

        struct timeval tv;
        tv.tv_sec = tv.tv_usec = 0;

        int nready = select(max_fd + 1, &fds, 0, 0, 0);
        if(nready)
        {
            bool gotMessage = false;
            for (int i = 0; i <= max_fd; ++i)
            {
                if (FD_ISSET(i, &fds))
                {
                    recv(i, buffer, 4096, 0);
                    gotMessage = true;
                    break;
                }
            }

            if (!gotMessage)
            {
                delete [] buffer;
                continue;
            }
        }
        else
        {
            delete [] buffer;
            continue;
        }

        LibUiProtocol::WindowManagerMessage *pHeader =
            reinterpret_cast<LibUiProtocol::WindowManagerMessage*>(buffer);

        if (pHeader->messageCode == which && pHeader->isResponse == bResponse)
        {
            handleMessage(buffer);
            bContinue = false;
            delete [] buffer;
        }
        else
        {
            // Not what we wanted, mark the message pending and try again.
            s_PendingMessages.push(buffer);
        }
    }
}

void Widget::handleMessage(const char *pMessageBuffer)
{
    const LibUiProtocol::WindowManagerMessage *pMessage =
        reinterpret_cast<const LibUiProtocol::WindowManagerMessage*>(pMessageBuffer);

    std::map<uint64_t, Widget *>::iterator it;
    if ((it = s_KnownWidgets.find(pMessage->widgetHandle)) == s_KnownWidgets.end())
    {
        syslog(LOG_ALERT, "no widget known for handle %lx", pMessage->widgetHandle);
        return;
    }

    Widget *pWidget = it->second;
    widgetCallback_t cb = pWidget->getCallback();
    if (!cb)
    {
        return;
    }

    switch(pMessage->messageCode)
    {
        case LibUiProtocol::Reposition:
            {
                const LibUiProtocol::RepositionMessage *pReposition =
                    reinterpret_cast<const LibUiProtocol::RepositionMessage*>(pMessageBuffer + sizeof(LibUiProtocol::WindowManagerMessage));
                delete pWidget->m_SharedFramebuffer;
#ifdef TARGET_LINUX
                pWidget->m_SharedFramebuffer = new SharedBuffer(pReposition->shmem_size, pReposition->shmem_handle);
#else
                pWidget->m_SharedFramebuffer =
                    new PedigreeIpc::SharedIpcMessage(pReposition->shmem_size, pReposition->shmem_handle);
                pWidget->m_SharedFramebuffer->initialise();
#endif

                // Run the callback now that the framebuffer is re-created.
                cb(::Reposition, sizeof(pReposition->rt), &pReposition->rt);
                break;
            }
        case LibUiProtocol::KeyEvent:
            {
                const LibUiProtocol::KeyEventMessage *pKeyEvent =
                    reinterpret_cast<const LibUiProtocol::KeyEventMessage*>(pMessageBuffer + sizeof(LibUiProtocol::WindowManagerMessage));

                cb(::KeyUp, sizeof(pKeyEvent->key), &pKeyEvent->key);
                break;
            }
        case LibUiProtocol::RawKeyEvent:
            {
                const LibUiProtocol::RawKeyEventMessage *pKeyEvent =
                    reinterpret_cast<const LibUiProtocol::RawKeyEventMessage*>(pMessageBuffer + sizeof(LibUiProtocol::WindowManagerMessage));

                WidgetMessages msg = ::RawKeyUp;
                if(pKeyEvent->state == LibUiProtocol::Down)
                    msg = ::RawKeyDown;

                cb(msg, sizeof(pKeyEvent->scancode), &pKeyEvent->scancode);
            }
            break;
        case LibUiProtocol::Focus:
            cb(::Focus, 0, 0);
            break;
        case LibUiProtocol::NoFocus:
            cb(::NoFocus, 0, 0);
            break;
        case LibUiProtocol::RequestRedraw:
            // Spurious redraw ACK.
            break;
        case LibUiProtocol::Destroy:
            cb(::Terminate, 0, 0);
            break;
        default:
            syslog(LOG_INFO, "** unknown event %d", pMessage->messageCode);
    }
}

bool Widget::handlePendingMessages()
{
    bool bHandled = !s_PendingMessages.empty();
    while(!s_PendingMessages.empty())
    {
        char *buffer = s_PendingMessages.front();
        s_PendingMessages.pop();

        Widget::handleMessage(buffer);
        delete [] buffer;
    }
    return bHandled;
}
