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

#include "Vga.h"
#include <BootIO.h>

#include <stdio.h>

HostedVga::HostedVga() :
  m_nWidth(80),
  m_nHeight(25),
  m_CursorX(0),
  m_CursorY(0),
  m_ModeStack(0),
  m_nMode(3),
  m_nControls(0),
  m_pBackbuffer(0)
{
}

HostedVga::~HostedVga()
{
    delete [] m_pBackbuffer;
}

void HostedVga::setControl(Vga::VgaControl which)
{
    m_nControls |= 1 << static_cast<uint8_t>(which);
}

void HostedVga::clearControl(Vga::VgaControl which)
{
    m_nControls &= ~(1 << static_cast<uint8_t>(which));
}

bool HostedVga::setMode (int mode)
{
    m_nMode = mode;
    return true;
}

bool HostedVga::setLargestTextMode ()
{
    return true;
}

bool HostedVga::isMode (size_t nCols, size_t nRows, bool bIsText, size_t nBpp)
{
    if(bIsText)
        return true;
    return false;
}

bool HostedVga::isLargestTextMode ()
{
    return true;
}

void HostedVga::rememberMode()
{
}

void HostedVga::restoreMode()
{
}

void HostedVga::pokeBuffer (uint8_t *pBuffer, size_t nBufLen)
{
    if (!pBuffer)
        return;
    if (!m_pBackbuffer)
        return;

    size_t thisLen = (m_nWidth * m_nWidth * 2);
    if (nBufLen > thisLen)
        nBufLen = thisLen;

    MemoryCopy(m_pBackbuffer, pBuffer, nBufLen);

    size_t savedCursorX = m_CursorX;
    size_t savedCursorY = m_CursorY;

    moveCursor(0, 0);
    for (size_t y = 0; y < m_nHeight; ++y)
    {
        for (size_t x = 0; x < m_nWidth; ++x)
        {
            size_t offset = (y * m_nWidth) + x;
            char c = static_cast<char>(m_pBackbuffer[offset] & 0xFF);
            uint8_t attr = (m_pBackbuffer[offset] >> 8) & 0xFF;
            HostedVga::printAttrAsAnsi(attr);
            putchar(c);
        }

        putchar('\n');
    }

    moveCursor(savedCursorX, savedCursorY);
}

void HostedVga::peekBuffer (uint8_t *pBuffer, size_t nBufLen)
{
    if (!m_pBackbuffer)
        return;

    size_t thisLen = (m_nWidth * m_nWidth * 2);
    if (nBufLen > thisLen)
        nBufLen = thisLen;
    MemoryCopy(pBuffer, m_pBackbuffer, nBufLen);
}

void HostedVga::moveCursor (size_t nX, size_t nY)
{
    if (!m_pBackbuffer)
        return;

    m_CursorX = nX;
    m_CursorY = nY;
    printf("\033[%zu;%zuH", nY, nX);
}

bool HostedVga::initialise()
{
    if (m_pBackbuffer)
    {
        // Already initialised.
        return true;
    }

    m_pBackbuffer = new uint16_t[m_nHeight * m_nWidth];
    m_nControls = 0;

    return true;
}

uint8_t HostedVga::ansiColourFixup(uint8_t colour)
{
    switch(colour)
    {
        case BootIO::Black:
        case BootIO::DarkGrey:
            return 0;
        case BootIO::Blue:
        case BootIO::LightBlue:
            return 4;
        case BootIO::Green:
        case BootIO::LightGreen:
            return 2;
        case BootIO::Cyan:
        case BootIO::LightCyan:
            return 6;
        case BootIO::Red:
        case BootIO::LightRed:
            return 1;
        case BootIO::Magenta:
        case BootIO::LightMagenta:
            return 5;
        case BootIO::Orange:
        case BootIO::Yellow:
            return 3;
        case BootIO::LightGrey:
        case BootIO::White:
            return 7;
        default:
            // Default colour.
            return 9;
    }
}

void HostedVga::printAttrAsAnsi(uint8_t attr)
{
    uint8_t fore = ansiColourFixup(attr & 0xF);
    uint8_t back = ansiColourFixup((attr >> 4) & 0xF);

    uint32_t fore_param = fore > 7 ? 90 + fore : 30 + fore;
    uint32_t back_param = back > 7 ? 100 + back : 40 + back;
    printf("\033[%d;%dm", fore_param, back_param);
}
