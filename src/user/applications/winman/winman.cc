/*
 * Copyright (c) 2010 James Molloy, Matthew Iselin
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
#include <stdio.h>
#include <unistd.h>
#include <syslog.h>
#include <string.h>
#include <sched.h>

#include <map>
#include <list>

#include <graphics/Graphics.h>
#include <input/Input.h>
#include <ipc/Ipc.h>

#include <protocol.h>

PedigreeGraphics::Framebuffer *g_pTopLevelFramebuffer = 0;

/** \addtogroup PedigreeGUI
 *  @{
 */

/**
 * WObject: base class for a window manager object.
 */
class WObject
{
    public:
        enum Type
        {
            Container,
            Window,
        };

        WObject() : m_Dimensions(0, 0, 0, 0)
        {
        }

        virtual ~WObject()
        {
        }

        virtual Type getType() const = 0;

        virtual void resize(ssize_t horizDistance, ssize_t vertDistance, WObject *pChild = 0) = 0;

        void reposition(size_t x = ~0UL, size_t y = ~0UL, size_t w = ~0UL, size_t h = ~0UL)
        {
            if(x == ~0UL)
                x = m_Dimensions.getX();
            if(y == ~0UL)
                y = m_Dimensions.getY();
            if(w == ~0UL)
                w = m_Dimensions.getW();
            if(h == ~0UL)
                h = m_Dimensions.getH();
            //m_Dimensions.update(x, y, m_Dimensions.getW(), m_Dimensions.getH());

            syslog(LOG_INFO, "reposition: %d, %d %dx%d", x, y, w, h);
            m_Dimensions.update(x, y, w, h);
            syslog(LOG_INFO, "actual reposition: %d, %d %dx%d", m_Dimensions.getX(), m_Dimensions.getY(), m_Dimensions.getW(), m_Dimensions.getH());

            ssize_t horizDistance = w - static_cast<ssize_t>(m_Dimensions.getW());
            ssize_t vertDistance = h - static_cast<ssize_t>(m_Dimensions.getH());
            //resize(horizDistance, vertDistance, this);
        }

        virtual void resized()
        {
        }

    protected:
        void setDimensions(PedigreeGraphics::Rect &rt)
        {
            m_Dimensions = rt;
        }

        PedigreeGraphics::Rect &getDimensions()
        {
            return m_Dimensions;
        }

    private:
        PedigreeGraphics::Rect m_Dimensions;
};

/**
 * Window: an actual window in the system, with actual rendering logic and
 * event handling.
 */
class Window : public WObject
{
    public:
        Window(PedigreeGraphics::Framebuffer *pBaseFramebuffer) :
            m_pBaseFramebuffer(pBaseFramebuffer)
        {
        }
        Window();

        virtual ~Window()
        {
        }

        virtual Type getType() const
        {
            return WObject::Window;
        }

        virtual void render()
        {
            PedigreeGraphics::Rect &me = getDimensions();
            size_t x = me.getX() + 5;
            size_t y = me.getY() + 5;
            size_t w = me.getW() - 10;
            size_t h = me.getH() - 10;
            //syslog(LOG_INFO, "winman: Window::render(%d, %d, %d, %d)", x, y, w, h);
            m_pBaseFramebuffer->rect(x, y, w, h, PedigreeGraphics::createRgb(255, 0, 0), PedigreeGraphics::Bits32_Rgb);
            m_pBaseFramebuffer->redraw(x, y, w, h, true);
        }

        virtual void resize(ssize_t horizDistance, ssize_t vertDistance, WObject *pChild = 0)
        {
        }

    private:
        PedigreeGraphics::Framebuffer *m_pBaseFramebuffer;
};

/**
 * Container: contains windows (and other containers) and defines layout
 * semantics, including management of the sizing of its own children.
 */
class Container : public WObject
{
    public:
        enum Layout
        {
            SideBySide, // Subwindows are tiled side-by-side
            Stacked, // Subwindows are tiled each above the other
        };

        Container(WObject *pParent) :
            m_Children(), m_pParent(pParent), m_Layout(SideBySide)
        {
        }

        virtual ~Container()
        {
        }

        virtual Type getType() const
        {
            return WObject::Container;
        }

        Layout getLayout() const
        {
            return m_Layout;
        }

        void setLayout(Layout newLayout)
        {
            m_Layout = newLayout;
            retile();
        }

        /**
         * Add a new child.
         */
        void addChild(WObject *pChild)
        {
            m_Children.push_back(pChild);
            retile();
        }

        /**
         * Taking our dimensions and layout into account, retile our children.
         * This will resize and reposition children, which may cause them to
         * retile also.
         */
        void retile()
        {
            syslog(LOG_INFO, "winman: retile");

            if(m_Children.size() == 0)
            {
                syslog(LOG_INFO, "winman: no children in container, no retile");
                return;
            }

            PedigreeGraphics::Rect &me = getDimensions();
            size_t currentWidth = me.getW();
            size_t currentHeight = me.getH();

            if(m_Layout == SideBySide)
            {
                size_t dividedW = currentWidth / m_Children.size();
                size_t newX = me.getX();
                size_t newY = me.getY();

                WObjectList_t::iterator it = m_Children.begin();
                for(; it != m_Children.end(); ++it)
                {
                    syslog(LOG_INFO, "tile now at x=%d, w=%d", newX, dividedW);
                    (*it)->reposition(newX, newY, dividedW, currentHeight);
                    newX += dividedW;
                }
            }
            else if(m_Layout == Stacked)
            {
                size_t dividedH = currentHeight / m_Children.size();
                size_t newX = me.getX();
                size_t newY = me.getY();

                WObjectList_t::iterator it = m_Children.begin();
                for(; it != m_Children.end(); ++it)
                {
                    syslog(LOG_INFO, "tile now at y=%d, h=%d", newY, dividedH);
                    (*it)->reposition(newX, newY, currentWidth, dividedH);
                    newY += dividedH;
                }
            }

            // Fun logic to tile children goes here.
            // May need to call resized() on child windows.
        }

        /**
         * Resize the entire container.
         * A child might ask us to do this if it is resized horizontally and
         * we are tiling Stacked, as it no longer fits inside our container.
         */
        virtual void resize(ssize_t horizDistance, ssize_t vertDistance, WObject *pChild = 0)
        {
            PedigreeGraphics::Rect &me = getDimensions();
            size_t currentWidth = me.getW();
            size_t currentHeight = me.getH();

            reposition(
                me.getX(), me.getY(),
                currentWidth + horizDistance, currentHeight + vertDistance);

            if(m_pParent->getType() == WObject::Container)
            {
                Container *pContainer = static_cast<Container*>(m_pParent);
                pContainer->resize(horizDistance, vertDistance, this);
            }

            retile();
        }

        /**
         * Render children
         */
        void render()
        {
            WObjectList_t::iterator it = m_Children.begin();
            for(; it != m_Children.end(); ++it)
            {
                if((*it)->getType() == WObject::Container)
                {
                    Container *pContainer = static_cast<Container*>(*it);
                    pContainer->render();
                }
                else if((*it)->getType() == WObject::Window)
                {
                    ::Window *pWindow = static_cast< ::Window*>(*it);
                    pWindow->render();
                }
            }
        }

    protected:
        typedef std::list<WObject*> WObjectList_t;
        std::list<WObject*> m_Children;

        Container()
        {
            Container(0);
        }

    private:
        WObject *m_pParent;

        Layout m_Layout;
};

/**
 * Root container. Can't be resized, holds the first level of children.
 */
class RootContainer : public Container
{
    public:
        RootContainer(size_t w, size_t h) :
            Container()
        {
            reposition(0, 0, w, h);
        }

        virtual void resize(ssize_t horizDistance, ssize_t vertDistance, WObject *pChild = 0)
        {
        }
};





/** @} */

class WindowMetadata {
    public:
        PedigreeGraphics::Framebuffer *pFramebuffer;
        char endpoint[256];
};

std::map<uint64_t, WindowMetadata*> *g_Windows;

void handleMessage(char *messageData)
{
    LibUiProtocol::WindowManagerMessage *pWinMan =
        reinterpret_cast<LibUiProtocol::WindowManagerMessage*>(messageData);

    syslog(LOG_INFO, "winman: incoming message at %x", messageData);
    syslog(LOG_INFO, "winman: incoming message with code %d", pWinMan->messageCode);

    size_t totalSize = sizeof(LibUiProtocol::WindowManagerMessage);

    PedigreeIpc::IpcMessage *pIpcResponse = new PedigreeIpc::IpcMessage();
    bool bSuccess = pIpcResponse->initialise();
    syslog(LOG_INFO, "winman: creating response: %s", bSuccess ? "good" : "bad");

    if(pWinMan->messageCode == LibUiProtocol::Create)
    {
        LibUiProtocol::CreateMessage *pCreate =
        reinterpret_cast<LibUiProtocol::CreateMessage*>(messageData + sizeof(LibUiProtocol::WindowManagerMessage));
        totalSize += sizeof(LibUiProtocol::CreateMessageResponse);
        char *responseData = (char *) pIpcResponse->getBuffer();

        WindowMetadata *pMetadata = new WindowMetadata;
        strcpy(pMetadata->endpoint, pCreate->endpoint);

        /// \todo Create a new window PROPERLY.
        syslog(LOG_INFO, "winman: create %dx%d", pCreate->minWidth, pCreate->minHeight);
        pMetadata->pFramebuffer = g_pTopLevelFramebuffer->createChild(0, 0, pCreate->minWidth, pCreate->minHeight);

        LibUiProtocol::WindowManagerMessage *pHeader =
            reinterpret_cast<LibUiProtocol::WindowManagerMessage*>(responseData);
        pHeader->messageCode = LibUiProtocol::Create;
        pHeader->widgetHandle = pWinMan->widgetHandle;
        pHeader->messageSize = sizeof(LibUiProtocol::CreateMessageResponse);
        pHeader->isResponse = true;

        LibUiProtocol::CreateMessageResponse *pCreateResp =
            reinterpret_cast<LibUiProtocol::CreateMessageResponse*>(responseData + sizeof(LibUiProtocol::WindowManagerMessage));
        pCreateResp->provider = pMetadata->pFramebuffer->getProvider();

        syslog(LOG_INFO, "winman: create message!");
        g_Windows->insert(std::make_pair(pWinMan->widgetHandle, pMetadata));
        syslog(LOG_INFO, "winman: metadata entered");

        syslog(LOG_INFO, "winman: getting endpoint for IPC");
        PedigreeIpc::IpcEndpoint *pEndpoint = PedigreeIpc::getEndpoint(pMetadata->endpoint);
        syslog(LOG_INFO, "winman: endpoint retrieved [%s] %p", pMetadata->endpoint, pEndpoint);

        syslog(LOG_INFO, "winman: transmitting response");
        PedigreeIpc::send(pEndpoint, pIpcResponse, true);

        syslog(LOG_INFO, "winman: create message response transmitted");
    }
    else
    {
        syslog(LOG_INFO, "winman: unhandled message type");
    }

    // delete pIpcResponse;
}

void checkForMessages(PedigreeIpc::IpcEndpoint *pEndpoint)
{
    if(!pEndpoint)
        return;

    PedigreeIpc::IpcMessage *pRecv = 0;
    if(PedigreeIpc::recv(pEndpoint, &pRecv, false))
    {
        handleMessage(static_cast<char *>(pRecv->getBuffer()));

        delete pRecv;
    }
}

int main(int argc, char *argv[])
{
    // Create the window manager IPC endpoint for libui.
    PedigreeIpc::createEndpoint("pedigree-winman");
    PedigreeIpc::IpcEndpoint *pEndpoint = PedigreeIpc::getEndpoint("pedigree-winman");

    if(!pEndpoint)
    {
        syslog(LOG_CRIT, "error: couldn't create the pedigree-winman IPC endpoint!");
        return 0;
    }

    /*
    // Make a program to test window creation etc...
    if(fork() == 0)
    {
        syslog(LOG_INFO, "winman: child alive %d", getpid());
        char *const new_argv[] = {"/applications/gears"};
        execv(new_argv[0], new_argv);
        return 0;
    }
    */

    // Use the root framebuffer.
    g_pTopLevelFramebuffer = new PedigreeGraphics::Framebuffer();

    if(!g_pTopLevelFramebuffer->getRawBuffer())
    {
        syslog(LOG_CRIT, "error: no top-level framebuffer could be created.");
        return -1;
    }

    size_t g_nWidth = g_pTopLevelFramebuffer->getWidth();
    size_t g_nHeight = g_pTopLevelFramebuffer->getHeight();

    g_pTopLevelFramebuffer->rect(0, 0, g_nWidth, g_nHeight, PedigreeGraphics::createRgb(0, 0, 255), PedigreeGraphics::Bits32_Rgb);
    g_pTopLevelFramebuffer->redraw(0, 0, g_nWidth, g_nHeight, true);

    syslog(LOG_INFO, "winman: creating tile root");

    RootContainer *pRootContainer = new RootContainer(g_nWidth, g_nHeight);

    Window *pLeft = new Window(g_pTopLevelFramebuffer);
    Window *pRight = new Window(g_pTopLevelFramebuffer);

    Container *pMiddle = new Container(pRootContainer);
    pMiddle->setLayout(Container::Stacked);

    Window *pTop = new Window(g_pTopLevelFramebuffer);
    Window *pMiddleWindow = new Window(g_pTopLevelFramebuffer);
    Window *pBottom = new Window(g_pTopLevelFramebuffer);

    // Should be evenly split down the screen after retile()
    pRootContainer->addChild(pLeft);
    pRootContainer->addChild(pMiddle);
    pRootContainer->addChild(pRight);

    // Should be evenly split in the middle tile after retile()
    pMiddle->addChild(pTop);
    pMiddle->addChild(pMiddleWindow);
    pMiddle->addChild(pBottom);

    syslog(LOG_INFO, "winman: entering main loop %d", getpid());

    g_Windows = new std::map<uint64_t, WindowMetadata*>();

    // Main loop: logic & message handling goes here!
    while(true)
    {
        //checkForMessages(pEndpoint);
        pRootContainer->render();
        g_pTopLevelFramebuffer->redraw();

        sched_yield();
    }

    return 0;
}
