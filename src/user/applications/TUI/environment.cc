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

#include "environment.h"
#include <syscall.h>

#include <stdio.h>
extern void log();
void *operator new (size_t size) throw()
{
  void *ret = malloc(size);
  return ret;
}
void *operator new[] (size_t size) throw()
{
  return malloc(size);
}
void *operator new (size_t size, void* memory) throw()
{
  return memory;
}
void *operator new[] (size_t size, void* memory) throw()
{
  return memory;
}
void operator delete (void * p)
{
  free(p);
}
void operator delete[] (void * p)
{
  free(p);
}

size_t Syscall::nextRequest(size_t responseToLast, char *buffer, size_t *sz, size_t buffersz, size_t *terminalId)
{
    size_t ret = syscall5(TUI_NEXT_REQUEST, responseToLast, reinterpret_cast<size_t>(buffer), reinterpret_cast<size_t>(sz), buffersz, reinterpret_cast<size_t>(terminalId));
    // Memory barrier, "sz" will have changed. Reload.
    asm volatile ("" : : : "memory");
    return ret;
}

size_t Syscall::getFb(Display::ScreenMode *pMode)
{
    return 0;
    
    return syscall1(TUI_GETFB, reinterpret_cast<size_t>(pMode));
    // Memory barrier, "sz" will have changed. Reload.
    asm volatile ("" : : : "memory");
}

void Syscall::requestPending()
{
    syscall0(TUI_REQUEST_PENDING);
}

void Syscall::respondToPending(size_t response, char *buffer, size_t sz)
{
    syscall3(TUI_RESPOND_TO_PENDING, response, reinterpret_cast<size_t>(buffer), sz);
}

void Syscall::createConsole(size_t tabId, char *pName)
{
    syscall2(TUI_CREATE_CONSOLE, tabId, reinterpret_cast<size_t>(pName));
}

void Syscall::setCtty(char *pName)
{
    syscall1(TUI_SET_CTTY, reinterpret_cast<size_t>(pName));
}

void Syscall::setCurrentConsole(size_t tabId)
{
    syscall1(TUI_SET_CURRENT_CONSOLE, tabId);
}

rgb_t *Syscall::newBuffer()
{
    return 0; // return reinterpret_cast<rgb_t*> (syscall0(TUI_VID_NEW_BUFFER));
}

void Syscall::setCurrentBuffer(rgb_t *pBuffer)
{
    return; // syscall1(TUI_VID_SET_BUFFER, reinterpret_cast<uintptr_t>(pBuffer));
}

void Syscall::updateBuffer(rgb_t *pBuffer, DirtyRectangle &rect)
{
    return;
    
    vid_req_t req;

    req.buffer = pBuffer;
    req.x = rect.getX();
    req.y = rect.getY();
    req.x2 = rect.getX2()-1;
    req.y2 = rect.getY2()-1;

    // Sanity check - this happens if no points were given!
    if (req.x  == ~0UL && req.y  == ~0UL &&
        req.x2 == ~0UL && req.y2 == ~0UL)
        return;

    asm volatile("" ::: "memory");

    syscall1(TUI_VID_UPDATE_BUFFER, reinterpret_cast<uintptr_t>(&req));
}

void Syscall::killBuffer(rgb_t *pBuffer)
{
    return;
    
    syscall1(TUI_VID_KILL_BUFFER, reinterpret_cast<uintptr_t>(pBuffer));
}

void Syscall::bitBlit(rgb_t *pBuffer, size_t x, size_t y, size_t x2, size_t y2,
                      size_t w, size_t h)
{
    return;
    
    vid_req_t req;
    req.buffer = pBuffer;
    req.x = x;
    req.y = y;
    req.x2 = x2;
    req.y2 = y2;
    req.w = w;
    req.h = h;

    asm volatile("" ::: "memory");

    syscall1(TUI_VID_BIT_BLIT, reinterpret_cast<uintptr_t>(&req));
}

void Syscall::fillRect(rgb_t *pBuffer, size_t x, size_t y, size_t w, size_t h, rgb_t c)
{
    return;
    
    vid_req_t req;
    req.buffer = pBuffer;
    req.x = x;
    req.y = y;
    req.w = w;
    req.h = h;
    req.c = &c;

    asm volatile("" ::: "memory");

    syscall1(TUI_VID_FILL_RECT, reinterpret_cast<uintptr_t>(&req));
}

DirtyRectangle::DirtyRectangle() :
    m_X(~0), m_Y(~0), m_X2(0), m_Y2(0)
{
}

DirtyRectangle::~DirtyRectangle()
{
}

void DirtyRectangle::point(size_t x, size_t y)
{
    if (x < m_X)
        m_X = x;
    if (x > m_X2)
        m_X2 = x;

    if (y < m_Y)
        m_Y = y;
    if (y > m_Y2)
        m_Y2 = y;
}

rgb_t interpolateColour(rgb_t col1, rgb_t col2, uint16_t a)
{
    rgb_t ret;
    ret.r = (col1.r*a + col2.r*(256-a)) / 256;
    ret.g = (col1.g*a + col2.g*(256-a)) / 256;
    ret.b = (col1.b*a + col2.b*(256-a)) / 256;

    return ret;
}
