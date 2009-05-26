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

#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include <syscall.h>

#include "Xterm.h"
#include "FreetypeXterm.h"

#define CONSOLE_READ    1
#define CONSOLE_WRITE   2
#define CONSOLE_GETROWS 3
#define CONSOLE_GETCOLS 4
#define CONSOLE_DATA_AVAILABLE 5

void *operator new (size_t size) throw()
{
  return malloc(size);
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

void log(char *c)
{
    syscall1(TUI_LOG, reinterpret_cast<size_t>(c));
}

size_t nextRequest(size_t responseToLast, char *buffer, size_t *sz, size_t buffersz)
{
    size_t ret = syscall4(TUI_NEXT_REQUEST, responseToLast, reinterpret_cast<size_t>(buffer), reinterpret_cast<size_t>(sz), buffersz);
    // Memory barrier, "sz" will have changed. Reload.
    asm volatile ("" : : : "memory");
    return ret;
}

size_t getFb(Display::ScreenMode *pMode)
{
    return syscall1(TUI_GETFB, reinterpret_cast<size_t>(pMode));
}

uint32_t getCharNonBlock()
{
    return syscall0(TUI_GETCHAR_NONBLOCK);
}

char queue[256];
size_t len = 0;

char getFromQueue()
{
    if (len > 0)
    {
        char c = queue[0];
        for (size_t i = 0; i < len-1; i++)
            queue[i] = queue[i+1];
        len--;
        return c;
    }
    else
        return 0;
}

bool addXtermCommandToQueue(uint32_t utf32)
{
    return false;
}

void addUtf32ToQueue(uint32_t utf32)
{
    queue[len++] = utf32&0x7F;
}

Xterm *g_pXterm;
size_t sz;
int main (int argc, char **argv)
{
    log ("Started up.");

    Display::ScreenMode mode;
    size_t fb = getFb(&mode);

    g_pXterm = new FreetypeXterm(mode, reinterpret_cast<void*>(fb));

    char *buffer = new char[2048];
    char str[64];
    size_t lastResponse = 0;
    while (true)
    {
        size_t cmd = nextRequest(lastResponse, buffer, &sz, 2047);
        sprintf(str, "Command %d received.", cmd);
        log(str);
        switch(cmd)
        {
            case CONSOLE_WRITE:
                buffer[sz] = '\0';
                g_pXterm->write(buffer);
                lastResponse = sz;
                break;
            case CONSOLE_READ:
            {
                size_t i = 0;
                while (i < sz)
                {
                    char c = getFromQueue();
                    if (c)
                    {
                        buffer[i++] = c;
                        continue;
                    }

                    uint32_t utf32 = getCharNonBlock();
                    if (utf32 == 0)
                    {
                        if (i == 0)
                            continue; ///  \todo Block here.
                        else
                            break;
                    }

                    // ENTER (\n) is actually \r on input.
                    if (utf32 == 0x000A) utf32 = 0x000D;

                    if (!addXtermCommandToQueue(utf32))
                        addUtf32ToQueue(utf32);
                }
                lastResponse = i;
                break;
            }
        }
    }

    return 0;
}

