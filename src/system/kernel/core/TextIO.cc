/*
 * Copyright (c) 2013 Matthew Iselin
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

#include <machine/TextIO.h>
#include <machine/Machine.h>
#include <machine/Vga.h>
#include <processor/Processor.h>
#include <Log.h>

TextIO::TextIO() :
    m_bInitialised(false), m_bControlSeq(false), m_bBracket(false),
    m_bParams(false), m_CursorX(0), m_CursorY(0), m_SavedCursorX(0),
    m_SavedCursorY(0), m_ScrollStart(0), m_ScrollEnd(0), m_CurrentParam(0),
    m_Params(), m_Fore(TextIO::White), m_Back(TextIO::Black),
    m_pFramebuffer(0), m_pVga(0)
{
}

TextIO::~TextIO()
{
}

bool TextIO::initialise(bool bClear)
{
    // Move into not-initialised mode, reset any held state.
    m_bInitialised = false;
    m_bControlSeq = false;
    m_bBracket = false;
    m_bParams = false;
    m_pFramebuffer = 0;
    m_CurrentParam = 0;
    m_CursorX = m_CursorY = 0;
    m_ScrollStart = m_ScrollEnd = 0;
    m_SavedCursorX = m_SavedCursorY = 0;
    memset(m_Params, 0, sizeof(size_t) * 4);

    m_pVga = Machine::instance().getVga(0);
    if(m_pVga)
    {
        m_pVga->setLargestTextMode();
        m_pFramebuffer = *m_pVga;
        if(m_pFramebuffer != 0)
        {
            if(bClear)
                memset(m_pFramebuffer, 0, m_pVga->getNumRows() * m_pVga->getNumCols() * sizeof(uint16_t));

            m_bInitialised = true;
            m_ScrollStart = 0;
            m_ScrollEnd = m_pVga->getNumRows() - 1;
        }
    }

    return m_bInitialised;
}

void TextIO::write(const char *s, size_t len)
{
    if(!m_bInitialised)
    {
        FATAL("TextIO misused: successfully call initialise() first.");
    }

    if(!s)
    {
        ERROR("TextIO: null string passed in.");
        return;
    }

    while((*s) && (len--))
    {
        uint8_t attributeByte = (m_Back << 4) | (m_Fore & 0x0F);
        uint16_t blank = ' ' | (attributeByte << 8);
        if(m_bControlSeq && m_bBracket)
        {
            switch(*s)
            {
                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9':
                    m_Params[m_CurrentParam] = (m_Params[m_CurrentParam] * 10) + ((*s) - '0');
                    m_bParams = true;
                    break;

                case ';':
                    ++m_CurrentParam;
                    break;

                case 'A':
                    // Cursor up.
                    if(m_CursorY)
                    {
                        if(m_bParams)
                            m_CursorY -= m_Params[0];
                        else
                            --m_CursorY;
                    }
                    m_bControlSeq = false;
                    break;

                case 'B':
                    // Cursor down.
                    if(m_bParams)
                        m_CursorY += m_Params[0];
                    else
                        ++m_CursorY;
                    
                    if(m_CursorY >= m_pVga->getNumRows())
                        m_CursorY = m_pVga->getNumRows() - 1;
                    m_bControlSeq = false;
                    break;

                case 'C':
                    // Cursor right.
                    if(m_bParams)
                        m_CursorX += m_Params[0];
                    else
                        ++m_CursorX;
                    
                    if(m_CursorX >= m_pVga->getNumCols())
                        m_CursorX = m_pVga->getNumCols() - 1;
                    m_bControlSeq = false;
                    break;

                case 'D':
                    // Cursor left.
                    if(m_CursorX)
                    {
                        if(m_bParams)
                            m_CursorX -= m_Params[0];
                        else
                            --m_CursorX;
                    }
                    m_bControlSeq = false;
                    break;

                case 'H':
                case 'f':
                    if(m_bParams)
                    {
                        // Set X/Y
                        m_CursorX = m_Params[1] - 1;
                        m_CursorY = m_Params[0] - 1;
                    }
                    else
                    {
                        // Reset X/Y
                        m_CursorX = m_CursorY = 0;
                    }
                    m_bControlSeq = false;
                    break;

                case 'J':
                    if(!m_bParams)
                    {
                        // Erase from current line to end of screen.
                        wmemset(&m_pFramebuffer[m_CursorY * m_pVga->getNumCols()],
                                blank,
                                (m_pVga->getNumRows() - m_CursorY) * m_pVga->getNumCols());
                    }
                    else if(m_Params[0] == 1)
                    {
                        // Erase from current line to top of screen.
                        wmemset(m_pFramebuffer,
                                blank,
                                m_CursorY * m_pVga->getNumCols());
                    }
                    else if(m_Params[0] == 2)
                    {
                        // Erase entire screen, move to home.
                        wmemset(m_pFramebuffer,
                                blank,
                                m_pVga->getNumRows() * m_pVga->getNumCols());
                        m_CursorX = m_CursorY = 0;
                    }
                    m_bControlSeq = false;
                    break;

                case 'K':
                    if(!m_bParams)
                    {
                        // Erase to end of line.
                        wmemset(&m_pFramebuffer[(m_CursorY * m_pVga->getNumCols()) + m_CursorX],
                                blank,
                                m_pVga->getNumCols() - m_CursorX);
                    }
                    else if(m_Params[0] == 1)
                    {
                        // Erase to start of line.
                        wmemset(&m_pFramebuffer[m_CursorY * m_pVga->getNumCols()],
                                blank,
                                m_CursorX);
                    }
                    else if(m_Params[0] == 2)
                    {
                        // Erase entire line.
                        wmemset(&m_pFramebuffer[m_CursorY * m_pVga->getNumCols()],
                                blank,
                                m_pVga->getNumCols());
                    }
                    m_bControlSeq = false;
                    break;

                case 'm':
                    // Handle \e[0m
                    if(m_bParams && !m_CurrentParam)
                        ++m_CurrentParam;

                    for(size_t i = 0; i < m_CurrentParam; ++i)
                    {
                        switch(m_Params[i])
                        {
                            case 0:
                                // Reset all attributes.
                                m_Fore = White;
                                m_Back = Black;
                                break;

                            /// \todo Hidden? Inverse?

                            case 30:
                            case 31:
                            case 32:
                            case 33:
                            case 34:
                            case 35:
                            case 36:
                            case 37:
                                setColour(&m_Fore, m_Params[i] - 30);
                                break;
                            case 38:
                                if(m_Params[i + 1] == 5)
                                {
                                    setColour(&m_Fore, m_Params[i + 2]);
                                    i += 3;
                                }
                                break;

                            case 40:
                            case 41:
                            case 42:
                            case 43:
                            case 44:
                            case 45:
                            case 46:
                            case 47:
                                setColour(&m_Back, m_Params[i] - 40);
                                break;
                            case 48:
                                if(m_Params[i + 1] == 5)
                                {
                                    setColour(&m_Back, m_Params[i + 2]);
                                    i += 3;
                                }
                                break;

                            case 90:
                            case 91:
                            case 92:
                            case 93:
                            case 94:
                            case 95:
                            case 96:
                            case 97:
                                setColour(&m_Fore, m_Params[i] - 90, true);
                                break;

                            case 100:
                            case 101:
                            case 102:
                            case 103:
                            case 104:
                            case 105:
                            case 106:
                            case 107:
                                setColour(&m_Back, m_Params[i] - 100, true);
                                break;

                            default:
                                WARNING("TextIO: unhandled 'Set Attribute Mode' command " << Dec << m_Params[i] << Hex << ".");
                                break;
                        }
                    }
                    m_bControlSeq = false;
                    break;

                case 'r':
                    if(m_bParams)
                    {
                        m_ScrollStart = m_Params[0] - 1;
                        m_ScrollEnd = m_Params[1] - 1;

                        if(m_ScrollStart >= m_pVga->getNumRows())
                            m_ScrollStart = m_pVga->getNumRows() - 1;
                        if(m_ScrollEnd >= m_pVga->getNumRows())
                            m_ScrollEnd = m_pVga->getNumRows() - 1;
                    }
                    else
                    {
                        m_ScrollStart = 0;
                        m_ScrollEnd = m_pVga->getNumRows() - 1;
                    }

                    if(m_ScrollStart > m_ScrollEnd)
                    {
                        size_t tmp = m_ScrollStart;
                        m_ScrollStart = m_ScrollEnd;
                        m_ScrollEnd = tmp;
                    }

                    m_bControlSeq = false;
                    break;

                case 's':
                    m_SavedCursorX = m_CursorX;
                    m_SavedCursorY = m_CursorY;
                    m_bControlSeq = false;
                    break;

                case 'u':
                    m_CursorX = m_SavedCursorX;
                    m_CursorY = m_SavedCursorY;
                    m_bControlSeq = false;
                    break;

                default:
                    ERROR("TextIO: unknown control sequence character '" << *s << "'!");
                    m_bControlSeq = false;
                    break;
            }
        }
        else if(m_bControlSeq && !m_bBracket)
        {
            switch(*s)
            {
                case '[':
                    m_bBracket = true;
                    break;

                case '7':
                    m_SavedCursorX = m_CursorX;
                    m_SavedCursorY = m_CursorY;
                    m_bControlSeq = false;
                    break;

                case '8':
                    m_CursorX = m_SavedCursorX;
                    m_CursorY = m_SavedCursorY;
                    m_bControlSeq = false;
                    break;

                case 'D':
                    // Scroll everything down one line.
                    memmove(&m_pFramebuffer[m_ScrollEnd * m_pVga->getNumCols()],
                            &m_pFramebuffer[m_ScrollStart * m_pVga->getNumCols()],
                            (m_ScrollEnd - m_ScrollStart) * m_pVga->getNumCols() * sizeof(uint16_t));
                    break;

                case 'M':
                    // Scroll everything up one line.
                    // Scroll everything down one line.
                    memmove(&m_pFramebuffer[m_ScrollStart * m_pVga->getNumCols()],
                            &m_pFramebuffer[m_ScrollEnd * m_pVga->getNumCols()],
                            (m_ScrollEnd - m_ScrollStart) * m_pVga->getNumCols() * sizeof(uint16_t));
                    break;

                default:
                    ERROR("TextIO: unknown escape sequence character '" << *s << "'!");
                    break;
            }
        }
        else
        {
            if(*s == '\e')
            {
                m_bControlSeq = true;
                m_bBracket = false;
                m_bParams = false;
                m_CurrentParam = 0;
                memset(m_Params, 0, sizeof(size_t) * 4);
            }
            else
            {
                switch(*s)
                {
                    case 0x08:
                        if(m_CursorX)
                            --m_CursorX;
                        else
                        {
                            m_CursorX = m_pVga->getNumCols() - 1;
                            if(m_CursorY)
                                --m_CursorY;
                        }
                        break;
                    case 0x09:
                        m_CursorX = (m_CursorX + 8) & ~7;
                        break;
                    case '\r':
                        m_CursorX = 0;
                        break;
                    case '\n':
                        m_CursorX = 0;
                        ++m_CursorY;
                        break;
                    default:
                        if(*s >= ' ')
                        {
                            if(m_CursorX < m_pVga->getNumCols())
                            {
                                m_pFramebuffer[(m_CursorY * m_pVga->getNumCols()) + m_CursorX] = *s | (attributeByte << 8);
                                m_CursorX++;
                            }
                        }
                        break;
                }

                // Handle wrapping, if needed.
                /// \todo Disabling line wrapping is allowed (but does what?)
                if(m_CursorX >= m_pVga->getNumCols())
                {
                    m_CursorX = 0;
                    ++m_CursorY;
                }

                if(m_CursorY > m_ScrollEnd)
                {
                    // By how much have we exceeded the scroll region?
                    size_t numRows = (m_CursorY - m_ScrollEnd);

                    // At what position is the top of the scroll?
                    // ie, to where are we moving the data into place?
                    size_t startOffset = m_ScrollStart * m_pVga->getNumCols();

                    // Where are we pulling data from?
                    size_t fromOffset = (m_ScrollStart + numRows) * m_pVga->getNumCols();

                    // How many rows are we moving? This is the distance from
                    // the 'from' offset to the end of the scroll region.
                    size_t movedRows = ((m_ScrollEnd + 1) * m_pVga->getNumCols()) - fromOffset;

                    // Where do we begin blanking from?
                    size_t blankFrom = (((m_ScrollEnd + 1) - numRows) * m_pVga->getNumCols());

                    // How much blanking do we need to do?
                    size_t blankLength = ((m_ScrollEnd + 1) * m_pVga->getNumCols()) - blankFrom;

                    memmove(&m_pFramebuffer[startOffset],
                            &m_pFramebuffer[fromOffset],
                            movedRows * sizeof(uint16_t));
                    wmemset(&m_pFramebuffer[blankFrom],
                            blank,
                            blankLength);

                    m_CursorY = m_ScrollEnd;
                }
            }
        }

        ++s;
    }

    // Assume we moved the cursor, and update where it is displayed
    // accordingly.
    m_pVga->moveCursor(m_CursorX, m_CursorY);
}

void TextIO::setColour(TextIO::VgaColour *which, size_t param, bool bBright)
{
    switch(param)
    {
        case 0:
            *which = bBright ? DarkGrey : Black;
            break;
        case 1:
            *which = bBright ? LightRed : Red;
            break;
        case 2:
            *which = bBright ? LightGreen : Green;
            break;
        case 3:
            *which = bBright ? Yellow : Orange;
            break;
        case 4:
            *which = bBright ? LightBlue : Blue;
            break;
        case 5:
            *which = bBright ? LightMagenta : Magenta;
            break;
        case 6:
            *which = bBright ? LightCyan : Cyan;
            break;
        case 7:
            *which = bBright ? White : LightGrey;
            break;
        default:
            break;
    }
}
