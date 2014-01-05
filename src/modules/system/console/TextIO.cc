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

#include <machine/Machine.h>
#include <machine/Vga.h>
#include <processor/Processor.h>
#include <Log.h>

#include "TextIO.h"

TextIO::TextIO(String str, size_t inode, Filesystem *pParentFS, File *pParent) :
    File(str, 0, 0, 0, inode, pParentFS, 0, pParent),
    m_bInitialised(false), m_bControlSeq(false), m_bBracket(false),
    m_bParenthesis(false), m_bParams(false), m_bQuestionMark(false),
    m_CursorX(0), m_CursorY(0), m_SavedCursorX(0), m_SavedCursorY(0),
    m_ScrollStart(0), m_ScrollEnd(0), m_LeftMargin(0), m_RightMargin(0),
    m_CurrentParam(0), m_Params(), m_Fore(TextIO::LightGrey), m_Back(TextIO::Black),
    m_pFramebuffer(0), m_pBackbuffer(0), m_pVga(0), m_TabStops(),
    m_OutBuffer(TEXTIO_RINGBUFFER_SIZE)
{
    m_pBackbuffer = new uint16_t[BACKBUFFER_STRIDE * BACKBUFFER_ROWS];
}

TextIO::~TextIO()
{
    delete [] m_pBackbuffer;
    m_pBackbuffer = 0;
}

bool TextIO::initialise(bool bClear)
{
    // Move into not-initialised mode, reset any held state.
    m_bInitialised = false;
    m_bControlSeq = false;
    m_bBracket = false;
    m_bParams = false;
    m_bQuestionMark = false;
    m_pFramebuffer = 0;
    m_CurrentParam = 0;
    m_CursorX = m_CursorY = 0;
    m_ScrollStart = m_ScrollEnd = 0;
    m_LeftMargin = m_RightMargin = 0;
    m_SavedCursorX = m_SavedCursorY = 0;
    m_CurrentModes = 0;
    memset(m_Params, 0, sizeof(size_t) * MAX_TEXTIO_PARAMS);
    memset(m_TabStops, 0, BACKBUFFER_STRIDE);

    m_pVga = Machine::instance().getVga(0);
    if(m_pVga)
    {
        m_pVga->setLargestTextMode();
        m_pFramebuffer = *m_pVga;
        if(m_pFramebuffer != 0)
        {
            if(bClear)
            {
                memset(m_pFramebuffer, 0, m_pVga->getNumRows() * m_pVga->getNumCols() * sizeof(uint16_t));
                memset(m_pBackbuffer, 0, BACKBUFFER_STRIDE * BACKBUFFER_ROWS);
            }

            m_bInitialised = true;
            m_ScrollStart = 0;
            m_ScrollEnd = m_pVga->getNumRows() - 1;
            m_LeftMargin = 0;
            m_RightMargin = m_pVga->getNumCols();

            m_CurrentModes = AnsiVt52;

            // Set default tab stops.
            for(size_t i = 0; i < BACKBUFFER_STRIDE; i += 8)
                m_TabStops[i] = '|';
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
        uint8_t attributeByte = 0;
        if(m_CurrentModes & Inverse)
        {
            attributeByte = (m_Fore << 4) | (m_Back & 0x0F);
        }
        else
        {
            attributeByte = (m_Back << 4) | (m_Fore & 0x0F);
        }
        uint16_t blank = ' ' | (attributeByte << 8);

        // NOTICE("Dispatching '" << (*s) << "'");

        if(m_bControlSeq && m_bBracket)
        {
            switch(*s)
            {
                case '"':
                case '$':
                case '!':
                case '>':
                    // Eat unhandled characters.
                    break;

                case 0x08:
                    doBackspace();
                    break;

                case '\n':
                case 0x0B:
                    if(m_CurrentModes & LineFeedNewLine)
                        doCarriageReturn();
                    doLinefeed();
                    break;

                case '\r':
                    doCarriageReturn();
                    break;

                case '?':
                    m_bQuestionMark = true;
                    break;

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
                    if(m_CurrentParam >= MAX_TEXTIO_PARAMS)
                        FATAL("TextIO: too many parameters!");
                    break;

                case 'A':
                    // Cursor up.
                    if(m_CursorY)
                    {
                        if(m_bParams && m_Params[0])
                            m_CursorY -= m_Params[0];
                        else
                            --m_CursorY;
                    }

                    if(m_CursorY < m_ScrollStart)
                        m_CursorY = m_ScrollStart;

                    m_bControlSeq = false;
                    break;

                case 'B':
                    // Cursor down.
                    if(m_bParams && m_Params[0])
                        m_CursorY += m_Params[0];
                    else
                        ++m_CursorY;

                    if(m_CursorY > m_ScrollEnd)
                        m_CursorY = m_ScrollEnd;

                    m_bControlSeq = false;
                    break;

                case 'C':
                    // Cursor right.
                    if(m_bParams && m_Params[0])
                        m_CursorX += m_Params[0];
                    else
                        ++m_CursorX;
                    
                    if(m_CursorX >= m_RightMargin)
                        m_CursorX = m_RightMargin - 1;

                    m_bControlSeq = false;
                    break;

                case 'D':
                    // Cursor left.
                    if(m_CursorX)
                    {
                        if(m_bParams && m_Params[0])
                            m_CursorX -= m_Params[0];
                        else
                            --m_CursorX;
                    }

                    if(m_CursorX < m_LeftMargin)
                        m_CursorX = m_LeftMargin;

                    m_bControlSeq = false;
                    break;

                case 'H':
                case 'f':
                    // CUP/HVP commands
                    if(m_bParams)
                    {
                        size_t xmove = m_Params[1] ? m_Params[1] - 1 : 0;
                        size_t ymove = m_Params[0] ? m_Params[0] - 1 : 0;

                        // Set X/Y
                        if(m_CurrentModes & Origin)
                        {
                            // Line/Column numbers are relative.
                            m_CursorX = m_LeftMargin + xmove;
                            m_CursorY = m_ScrollStart + ymove;
                        }
                        else
                        {
                            m_CursorX = xmove;
                            m_CursorY = ymove;
                        }
                    }
                    else
                    {
                        // Reset X/Y
                        if(m_CurrentModes & Origin)
                        {
                            m_CursorX = m_LeftMargin;
                            m_CursorY = m_ScrollStart;
                        }
                        else
                        {
                            m_CursorX = m_CursorY = 0;
                        }
                    }

                    m_bControlSeq = false;
                    break;

                case 'J':
                    if((!m_bParams) || (!m_Params[0]))
                    {
                        eraseEOS(blank);
                    }
                    else if(m_Params[0] == 1)
                    {
                        // Erase from current position to top of screen.
                        wmemset(m_pBackbuffer,
                                blank,
                                (m_CursorY * BACKBUFFER_STRIDE) + m_CursorX + 1);
                    }
                    else if(m_Params[0] == 2)
                    {
                        // Erase entire screen, move to home.
                        wmemset(m_pBackbuffer,
                                blank,
                                BACKBUFFER_ROWS * BACKBUFFER_STRIDE);
                    }
                    m_bControlSeq = false;
                    break;

                case 'K':
                    if((!m_bParams) || (!m_Params[0]))
                    {
                        eraseEOL(blank);
                    }
                    else if(m_Params[0] == 1)
                    {
                        // Erase to start of line.
                        wmemset(&m_pBackbuffer[m_CursorY * BACKBUFFER_STRIDE],
                                blank,
                                m_CursorX + 1);
                    }
                    else if(m_Params[0] == 2)
                    {
                        // Erase entire line.
                        wmemset(&m_pBackbuffer[m_CursorY * BACKBUFFER_STRIDE],
                                blank,
                                BACKBUFFER_STRIDE);
                    }
                    m_bControlSeq = false;
                    break;

                case 'c':
                    if(m_Params[0])
                    {
                        ERROR("TextIO: Device Attributes command with non-zero parameter.");
                    }
                    else
                    {
                        // We mosly support the 'Advanced Video Option'.
                        // (apart from underline/blink)
                        const char *attribs = "\e[?1;2c";
                        m_OutBuffer.write(const_cast<char *>(attribs), strlen(attribs));
                    }
                    m_bControlSeq = false;
                    break;

                case 'g':
                    if(m_Params[0])
                    {
                        if(m_Params[0] == 3)
                        {
                            memset(m_TabStops, 0, BACKBUFFER_STRIDE);
                        }
                    }
                    else
                    {
                        m_TabStops[m_CursorX] = 0;
                    }
                    m_bControlSeq = false;
                    break;

                case 'h':
                case 'l':
                    {
                    int modesToChange = 0;

                    if(m_bQuestionMark & m_bParams)
                    {
                        for(size_t i = 0; i <= m_CurrentParam; ++i)
                        {
                            switch(m_Params[i])
                            {
                                case 1:
                                    modesToChange |= CursorKey;
                                    break;
                                case 2:
                                    modesToChange |= AnsiVt52;
                                    break;
                                case 3:
                                    modesToChange |= Column;
                                    break;
                                case 4:
                                    modesToChange |= Scrolling;
                                    break;
                                case 5:
                                    modesToChange |= Screen;
                                    break;
                                case 6:
                                    modesToChange |= Origin;
                                    break;
                                case 7:
                                    modesToChange |= AutoWrap;
                                    break;
                                case 8:
                                    modesToChange |= AutoRepeat;
                                    break;
                                case 9:
                                    modesToChange |= Interlace;
                                    break;
                                default:
                                    WARNING("TextIO: unknown 'DEC Private Mode Set' mode '" << m_Params[i] << "'");
                                    break;
                            }
                        }
                    }
                    else if(m_bParams)
                    {
                        for(size_t i = 0; i <= m_CurrentParam; ++i)
                        {
                            switch(m_Params[i])
                            {
                                case 20:
                                    modesToChange |= LineFeedNewLine;
                                    break;
                                default:
                                    WARNING("TextIO: unknown 'Set Mode' mode '" << m_Params[i] << "'");
                                    break;
                            }
                        }
                    }

                    if(*s == 'h')
                    {
                        // Set modes.
                        m_CurrentModes |= modesToChange;

                        // Setting modes
                        if(modesToChange & Origin)
                        {
                            // Reset origin to margins.
                            m_CursorX = m_LeftMargin;
                            m_CursorY = m_ScrollStart;
                        }
                        else if(modesToChange & Column)
                        {
                            m_RightMargin = BACKBUFFER_COLS_WIDE;

                            // Clear screen as a side-effect.
                            wmemset(m_pBackbuffer,
                                    blank,
                                    BACKBUFFER_STRIDE * BACKBUFFER_ROWS);

                            // Reset margins.
                            m_LeftMargin = 0;
                            m_ScrollStart = 0;
                            m_ScrollEnd = BACKBUFFER_ROWS - 1;

                            // Home the cursor.
                            m_CursorX = 0;
                            m_CursorY = 0;
                        }
                    }
                    else
                    {
                        // Reset modes.
                        m_CurrentModes &= ~(modesToChange);

                        // Resetting modes
                        if(modesToChange & Origin)
                        {
                            // Reset origin to top left corner.
                            m_CursorX = 0;
                            m_CursorY = 0;
                        }
                        else if(modesToChange & Column)
                        {
                            m_RightMargin = BACKBUFFER_COLS_NORMAL;

                            // Clear screen as a side-effect.
                            wmemset(m_pBackbuffer,
                                    blank,
                                    BACKBUFFER_STRIDE * BACKBUFFER_ROWS);

                            // Reset margins.
                            m_LeftMargin = 0;
                            m_ScrollStart = 0;
                            m_ScrollEnd = BACKBUFFER_ROWS - 1;

                            // Home the cursor.
                            m_CursorX = 0;
                            m_CursorY = 0;
                        }
                    }

                    if(modesToChange & Screen)
                    {
                        VgaColour defaultBack = Black, defaultFore = LightGrey;

                        if(m_CurrentModes & Screen)
                        {
                            m_Fore = Black;
                            m_Back = LightGrey;
                        }
                        else
                        {
                            m_Fore = LightGrey;
                            m_Back = Black;

                            defaultBack = LightGrey;
                            defaultFore = Black;
                        }

                        // Invert the framebuffer.
                        for(size_t i = 0; i < (BACKBUFFER_STRIDE * BACKBUFFER_ROWS); ++i)
                        {
                            uint8_t c = m_pBackbuffer[i] & 0xFF;
                            uint8_t attrib = (m_pBackbuffer[i] >> 8) & 0xFF;

                            uint8_t back = (attrib >> 4) & 0xF;
                            uint8_t fore = attrib & 0xF;

                            if(back == defaultBack && fore == defaultFore)
                            {
                                attrib = (m_Back << 4) | (m_Fore & 0xF);
                            }

                            /// \todo This breaks characters with the 'negative' attribute.
                            m_pBackbuffer[i] = c | (attrib << 8);
                        }
                    }

                    m_bControlSeq = false;
                    }
                    break;

                case 'm':
                    for(size_t i = 0; i <= m_CurrentParam; ++i)
                    {
                        switch(m_Params[i])
                        {
                            case 0:
                                // Reset all attributes.
                                m_Fore = LightGrey;
                                m_Back = Black;
                                m_CurrentModes &= ~(Inverse | Bright);
                                break;

                            case 1:
                                if(!(m_CurrentModes & Bright))
                                {
                                    m_CurrentModes |= Bright;
                                    m_Fore = adjustColour(m_Fore, true);
                                }
                                break;

                            case 2:
                                if(m_CurrentModes & Bright)
                                {
                                    m_CurrentModes &= ~Bright;
                                    m_Fore = adjustColour(m_Fore, false);
                                }
                                break;

                            case 7:
                                if(!(m_CurrentModes & Inverse))
                                {
                                    m_CurrentModes |= Inverse;
                                }
                                break;

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
                                    setColour(&m_Fore, m_Params[i + 2], m_CurrentModes & Bright);
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
                                    setColour(&m_Back, m_Params[i + 2], m_CurrentModes & Bright);
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

                case 'n':
                    switch(m_Params[0])
                    {
                        case 5:
                            {
                                // Report ready with no malfunctions detected.
                                const char *status = "\e[0n";
                                m_OutBuffer.write(const_cast<char *>(status), strlen(status));
                            }
                            break;
                        case 6:
                            {
                                // Report cursor position.
                                // CPR - \e[ Y ; X R
                                NormalStaticString response("\e[");

                                size_t reportX = m_CursorX + 1;
                                size_t reportY = m_CursorY + 1;

                                if(m_CurrentModes & Origin)
                                {
                                    // Only report relative if the cursor is within the
                                    // margins and scroll region! Otherwise, absolute.
                                    if((reportX > m_LeftMargin) && (reportX <= m_RightMargin))
                                        reportX -= m_LeftMargin;
                                    if((reportY > m_ScrollStart) && (reportY <= m_ScrollEnd))
                                        reportY -= m_ScrollStart;
                                }

                                response.append(reportY);
                                response.append(";");
                                response.append(reportX);
                                response.append("R");
                                m_OutBuffer.write(const_cast<char *>(static_cast<const char *>(response)), response.length());
                            }
                            break;
                        default:
                            NOTICE("TextIO: unknown device status request " << Dec << m_Params[0] << Hex << ".");
                            break;
                    }
                    m_bControlSeq = false;
                    break;

                case 'p':
                    // Depending on parameters and symbols in the sequence, this
                    // could be "Set Conformance Level" (DECSCL),
                    // "Soft Terminal Reset" (DECSTR), etc, etc... so ignore for now.
                    /// \todo Should we handle this?
                    WARNING("TextIO: dropping command after seeing 'p' command sequence terminator.");
                    m_bControlSeq = false;
                    break;

                case 'q':
                    // Load LEDs
                    /// \todo hook in to Keyboard::setLedState!
                    m_bControlSeq = false;
                    break;

                case 'r':
                    if(m_bParams)
                    {
                        m_ScrollStart = m_Params[0] - 1;
                        m_ScrollEnd = m_Params[1] - 1;

                        if(m_ScrollStart >= BACKBUFFER_ROWS)
                            m_ScrollStart = BACKBUFFER_ROWS - 1;
                        if(m_ScrollEnd >= BACKBUFFER_ROWS)
                            m_ScrollEnd = BACKBUFFER_ROWS - 1;
                    }
                    else
                    {
                        m_ScrollStart = 0;
                        m_ScrollEnd = BACKBUFFER_ROWS - 1;
                    }

                    if(m_ScrollStart > m_ScrollEnd)
                    {
                        size_t tmp = m_ScrollStart;
                        m_ScrollStart = m_ScrollEnd;
                        m_ScrollEnd = tmp;
                    }

                    if(m_CurrentModes & Origin)
                    {
                        m_CursorX = m_LeftMargin;
                        m_CursorY = m_ScrollStart;
                    }
                    else
                    {
                        m_CursorX = 0;
                        m_CursorY = 0;
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

                case 'x':
                    // Request Terminal Parameters
                    if(m_Params[0] > 1)
                    {
                        ERROR("TextIO: invalid 'sol' parameter for 'Request Terminal Parameters'");
                    }
                    else
                    {
                        // Send back a parameter report.
                        // Parameters:
                        // * Reporting on request
                        // * No parity
                        // * 8 bits per character
                        // * 19200 bits per second xspeed
                        // * 19200 bits per second rspeed
                        // * 16x bit rate multiplier
                        // * No STP option, so no flags
                        const char *termparms = 0;
                        if(m_Params[0])
                            termparms = "\e[3;1;1;120;120;1;0x";
                        else
                            termparms = "\e[2;1;1;120;120;1;0x";
                        m_OutBuffer.write(const_cast<char *>(termparms), strlen(termparms));
                    }
                    m_bControlSeq = false;
                    break;

                case 'y':
                    // Invoke Confidence Test (no-op)
                    m_bControlSeq = false;
                    break;

                default:
                    ERROR("TextIO: unknown control sequence character '" << *s << "'!");
                    m_bControlSeq = false;
                    break;
            }
        }
        else if(m_bControlSeq && m_bParenthesis)
        {
            switch(*s)
            {
                default:
                    ERROR("TextIO: unknown character set sequence character '" << *s << "'!");
                    m_bControlSeq = false;
                    break;
            }
        }
        else if(m_bControlSeq && (!m_bBracket) && (!m_bParenthesis))
        {
            switch(*s)
            {
                case 0x08:
                    doBackspace();
                    break;

                case 'A':
                    if(m_CursorY > m_ScrollStart)
                        --m_CursorY;
                    m_bControlSeq = false;
                    break;

                case 'B':
                    if(m_CursorY < m_ScrollEnd)
                        ++m_CursorY;
                    m_bControlSeq = false;
                    break;

                case 'C':
                    ++m_CursorX;
                    if(m_CursorX >= m_RightMargin)
                        m_CursorX = m_RightMargin - 1;
                    m_bControlSeq = false;
                    break;

                case 'D':
                    if(m_CurrentModes & AnsiVt52)
                    {
                        // Index - cursor down one line, scroll if necessary.
                        doLinefeed();
                    }
                    else
                    {
                        // Cursor Left
                        if(m_CursorX > m_LeftMargin)
                            --m_CursorX;
                    }
                    m_bControlSeq = false;
                    break;

                case 'E':
                    // Next Line - move to start of next line.
                    doCarriageReturn();
                    doLinefeed();
                    m_bControlSeq = false;
                    break;

                case 'F':
                case 'G':
                    ERROR("TextIO: graphics mode is not implemented.");
                    m_bControlSeq = false;
                    break;

                case 'H':
                    if(m_CurrentModes & AnsiVt52)
                    {
                        // Horizontal tabulation set.
                        m_TabStops[m_CursorX] = '|';
                    }
                    else
                    {
                        // Cursor to Home
                        m_CursorX = 0;
                        m_CursorY = 0;
                    }
                    m_bControlSeq = false;
                    break;

                case 'M':
                case 'I':
                    // Reverse Index - cursor up one line, or scroll up if at top.
                    --m_CursorY;
                    checkScroll();
                    m_bControlSeq = false;
                    break;

                case 'J':
                    eraseEOS(blank);
                    m_bControlSeq = false;
                    break;

                case 'K':
                    eraseEOL(blank);
                    m_bControlSeq = false;
                    break;

                case 'Y':
                    {
                        uint8_t row = (*(++s)) - 0x20;
                        uint8_t col = (*(++s)) - 0x20;

                        /// \todo Sanity check.
                        m_CursorX = col;
                        m_CursorY = row;
                    }
                    m_bControlSeq = false;
                    break;

                case 'Z':
                    {
                        const char *identifier = 0;
                        if(m_CurrentModes & AnsiVt52)
                            identifier = "\e[?1;2c";
                        else
                            identifier = "\e/Z";
                        m_OutBuffer.write(const_cast<char *>(identifier), strlen(identifier));
                    }
                    m_bControlSeq = false;
                    break;

                case '#':
                    // DEC commands
                    ++s;
                    switch(*s)
                    {
                        case '8':
                            {
                            // DEC Screen Alignment Test (DECALN)
                            // Fills screen with 'E' characters.
                            uint16_t set = 'E' | (attributeByte << 8);
                            wmemset(m_pBackbuffer,
                                    set,
                                    BACKBUFFER_STRIDE * BACKBUFFER_ROWS);
                            }
                            break;

                        default:
                            ERROR("TextIO: unknown DEC command '" << *s << "'");
                            break;
                    }
                    m_bControlSeq = false;
                    break;

                case '=':
                    /// \todo implement me!
                    ERROR("TextIO: alternate keypad mode is not implemented.");
                    m_bControlSeq = false;
                    break;

                case '<':
                    m_CurrentModes |= AnsiVt52;
                    m_bControlSeq = false;
                    break;

                case '>':
                    /// \todo implement me!
                    ERROR("TextIO: alternate keypad mode is not implemented.");
                    m_bControlSeq = false;
                    break;

                case '[':
                    m_bBracket = true;
                    break;

                case '(':
                case ')':
                    m_bParenthesis = true;

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

                case 'c':
                    // Power-up reset!
                    initialise(true);
                    m_bControlSeq = false;
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
                m_bParenthesis = false;
                m_bQuestionMark = true;
                m_CurrentParam = 0;
                memset(m_Params, 0, sizeof(size_t) * MAX_TEXTIO_PARAMS);
            }
            else
            {
                switch(*s)
                {
                    case 0x05:
                        {
                            // Reply with our answerback.
                            const char *answerback = "\e[1;2c";
                            m_OutBuffer.write(const_cast<char *>(answerback), strlen(answerback));
                        }
                        break;
                    case 0x08:
                        doBackspace();
                        break;
                    case 0x09:
                        doHorizontalTab();
                        break;
                    case '\r':
                        doCarriageReturn();
                        break;
                    case '\n':
                    case 0x0B:
                        if(m_CurrentModes & LineFeedNewLine)
                            doCarriageReturn();
                        doLinefeed();
                        break;
                    case 0x0E:
                    case 0x0F:
                        NOTICE("TextIO: unhandled SHIFT-IN/SHIFT-OUT");
                        break;
                    default:
                        if(*s >= ' ')
                        {
                            // We must handle wrapping *just before* we write
                            // the next printable, because otherwise things
                            // like BS at the right margin fail to work correctly.
                            checkWrap();

                            if(m_CursorX < BACKBUFFER_STRIDE)
                            {
                                m_pBackbuffer[(m_CursorY * BACKBUFFER_STRIDE) + m_CursorX] = *s | (attributeByte << 8);
                                ++m_CursorX;
                            }
                            else
                            {
                                ERROR("TextIO: X co-ordinate is beyond the end of a backbuffer line: " << m_CursorX << " vs " << BACKBUFFER_STRIDE << "?");
                            }
                        }
                        break;
                }
            }
        }

        if(m_CursorX < m_LeftMargin)
        {
            WARNING("TextIO: X co-ordinate ended up befor the left margin.");
            m_CursorX = m_LeftMargin;
        }

        ++s;
    }

    // Assume we moved the cursor, and update where it is displayed
    // accordingly.
    m_pVga->moveCursor(m_CursorX, m_CursorY);

    // This write is now complete.
    flip();

    // Wake up anything waiting on output from us if needed.
    if(m_OutBuffer.dataReady())
        dataChanged();
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

void TextIO::doBackspace()
{
    // If we are at a position where we would expect to wrap, step back one
    // extra character position so we don't wrap.
    if(m_CursorX == m_RightMargin)
        --m_CursorX;

    // Backspace will not do anything if we are already on the left margin.
    if(m_CursorX > m_LeftMargin)
        --m_CursorX;
}

void TextIO::doLinefeed()
{
    ++m_CursorY;
    checkScroll();
}

void TextIO::doCarriageReturn()
{
    m_CursorX = m_LeftMargin;
}

void TextIO::doHorizontalTab()
{
    bool tabStopFound = false;

    // Move to the next tab stop from the current position.
    for(size_t x = (m_CursorX + 1); x < m_RightMargin; ++x)
    {
        if(m_TabStops[x] != 0)
        {
            m_CursorX = x;
            tabStopFound = true;
            break;
        }
    }

    if(!tabStopFound)
    {
        // Tab to the right margin, if no tab stop was found at all.
        m_CursorX = m_RightMargin - 1;
    }
    else if(m_CursorX >= m_RightMargin)
        m_CursorX = m_RightMargin - 1;
}

void TextIO::checkScroll()
{
    uint8_t attributeByte = (m_Back << 4) | (m_Fore & 0x0F);
    uint16_t blank = ' ' | (attributeByte << 8);

    // Handle scrolling, which can take place due to linefeeds and
    // other such cursor movements.
    if(m_CursorY < m_ScrollStart)
    {
        // By how much have we exceeded the scroll region?
        size_t numRows = (m_ScrollStart - m_CursorY);

        // Top of the scrolling area
        size_t sourceRow = m_ScrollStart;
        size_t destRow = m_ScrollStart + numRows;

        // Bottom of the scrolling area
        size_t sourceEnd = m_ScrollEnd + 1 - numRows;
        size_t destEnd = m_ScrollEnd + 1;

        // Move data.
        memmove(&m_pBackbuffer[destRow * BACKBUFFER_STRIDE],
                &m_pBackbuffer[sourceRow * BACKBUFFER_STRIDE],
                (sourceEnd - sourceRow) * BACKBUFFER_STRIDE * sizeof(uint16_t));

        // Clear out the start of the region now.
        wmemset(&m_pBackbuffer[sourceRow * BACKBUFFER_STRIDE],
                blank,
                (destRow - sourceRow) * BACKBUFFER_STRIDE);

        m_CursorY = m_ScrollStart;
    }
    else if(m_CursorY > m_ScrollEnd)
    {
        // By how much have we exceeded the scroll region?
        size_t numRows = (m_CursorY - m_ScrollEnd);

        // At what position is the top of the scroll?
        // ie, to where are we moving the data into place?
        size_t startOffset = m_ScrollStart * BACKBUFFER_STRIDE;

        // Where are we pulling data from?
        size_t fromOffset = (m_ScrollStart + numRows) * BACKBUFFER_STRIDE;

        // How many rows are we moving? This is the distance from
        // the 'from' offset to the end of the scroll region.
        size_t movedRows = ((m_ScrollEnd + 1) * BACKBUFFER_STRIDE) - fromOffset;

        // Where do we begin blanking from?
        size_t blankFrom = (((m_ScrollEnd + 1) - numRows) * BACKBUFFER_STRIDE);

        // How much blanking do we need to do?
        size_t blankLength = ((m_ScrollEnd + 1) * BACKBUFFER_STRIDE) - blankFrom;

        memmove(&m_pBackbuffer[startOffset],
                &m_pBackbuffer[fromOffset],
                movedRows * sizeof(uint16_t));
        wmemset(&m_pBackbuffer[blankFrom],
                blank,
                blankLength);

        m_CursorY = m_ScrollEnd;
    }
}

void TextIO::checkWrap()
{
    if(m_CursorX >= m_RightMargin)
    {
        // Default autowrap mode is off - new characters at
        // the right margin replace any that are already there.
        if(m_CurrentModes & AutoWrap)
        {
            m_CursorX = m_LeftMargin;
            ++m_CursorY;

            checkScroll();
        }
        else
        {
            m_CursorX = m_RightMargin - 1;
        }
    }
}

void TextIO::eraseEOS(uint16_t blank)
{
    // Erase from current position to end of screen.
    wmemset(&m_pBackbuffer[(m_CursorY * BACKBUFFER_STRIDE) + m_CursorX],
            blank,
            ((BACKBUFFER_ROWS - m_CursorY) * BACKBUFFER_STRIDE) - m_CursorX);
}

void TextIO::eraseEOL(uint16_t blank)
{
    // Erase to end of line.
    wmemset(&m_pBackbuffer[(m_CursorY * BACKBUFFER_STRIDE) + m_CursorX],
            blank,
            BACKBUFFER_STRIDE - m_CursorX - 1);
}

void TextIO::flip()
{
    for(size_t y = 0; y < m_pVga->getNumRows(); ++y)
    {
        memcpy(&m_pFramebuffer[y * m_pVga->getNumCols()],
                &m_pBackbuffer[y * BACKBUFFER_STRIDE],
                m_pVga->getNumCols() * sizeof(uint16_t));
    }
}

uint64_t TextIO::read(uint64_t location, uint64_t size, uintptr_t buffer, bool bCanBlock)
{
    if(bCanBlock)
    {
        if(!m_OutBuffer.waitFor(RingBufferWait::Reading))
            return 0; // Interrupted.
    }
    else if(!m_OutBuffer.dataReady())
    {
        return 0;
    }

    uintptr_t originalBuffer = buffer;
    while(m_OutBuffer.dataReady() && size)
    {
        *reinterpret_cast<char*>(buffer) = m_OutBuffer.read();
        ++buffer;
        --size;
    }

    return buffer - originalBuffer;
}

uint64_t TextIO::write(uint64_t location, uint64_t size, uintptr_t buffer, bool bCanBlock)
{
    write(reinterpret_cast<const char *>(buffer), size);
    return size;
}

int TextIO::select(bool bWriting, int timeout)
{
    if(bWriting)
        return 1;

    if(timeout)
    {
        while(!m_OutBuffer.waitFor(RingBufferWait::Reading));
        return 1;
    }
    else
    {
        return m_OutBuffer.dataReady();
    }
}
