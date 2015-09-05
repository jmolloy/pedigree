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

#include <machine/Machine.h>
#include <machine/HidInputManager.h>
#include <machine/KeymapManager.h>
#include <machine/Device.h>
#include "Keyboard.h"

#ifdef DEBUGGER
  #include <Debugger.h>

  #ifdef TRACK_PAGE_ALLOCATIONS
    #include <AllocationCommand.h>
  #endif
#endif

#ifdef DEBUGGER
#include <SlamCommand.h>
#endif

#include <SlamAllocator.h>

namespace __pedigree_hosted {};
using namespace __pedigree_hosted;

#include <unistd.h>
#include <fcntl.h>
#include <termios.h>

HostedKeyboard::HostedKeyboard() : m_bDebugState(false)
{
    // Eat any pending input.
    blocking(false);
    char buf[64] = {0};
    while(read(0, buf, 64) == 64);
    blocking(true);
}

HostedKeyboard::~HostedKeyboard()
{
}

void HostedKeyboard::initialise()
{
    // Don't echo characters.
    struct termios t;
    tcgetattr(0, &t);
    t.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(0, TCSAFLUSH, &t);
}

char HostedKeyboard::getChar()
{
    if(!getDebugState())
        return 0;

    blocking(true);

    char buf[2] = {0};
    ssize_t n = read(0, buf, 1);
    if (n != 1)
        return 0;
    return buf[0];
}

char HostedKeyboard::getCharNonBlock()
{
    if(!getDebugState())
        return 0;

    blocking(false);

    char buf[2] = {0};
    ssize_t n = read(0, buf, 1);
    if(n != 1)
        return 0;
    return buf[0];
}

void HostedKeyboard::setDebugState(bool enableDebugState)
{
    m_bDebugState = enableDebugState;
}

bool HostedKeyboard::getDebugState()
{
    return m_bDebugState;
}

char HostedKeyboard::getLedState()
{
    return 0;
}

void HostedKeyboard::setLedState(char state)
{
}

void HostedKeyboard::blocking(bool enable)
{
    int flags = fcntl(0, F_GETFL);
    if(flags < 0)
    {
        return;
    }
    if(enable)
        flags &= ~O_NONBLOCK;
    else
        flags |= O_NONBLOCK;
    fcntl(0, F_SETFL, flags);
}
