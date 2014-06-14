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

#ifndef TEXTIO_H
#define TEXTIO_H

#include <processor/types.h>
#include <utilities/RingBuffer.h>

#include <machine/TimerHandler.h>

#include <vfs/File.h>

class Vga;

#define MAX_TEXTIO_PARAMS 16
#define TEXTIO_RINGBUFFER_SIZE 1024

// Blink periods in milliseconds.
// We show text for 2x as long as the text is hidden.
#define BLINK_ON_PERIOD 1000
#define BLINK_OFF_PERIOD (BLINK_ON_PERIOD / 2)

/**
 * Provides exceptionally simple VT100 emulation to the Vga class, if
 * one exists. Note that this is NOT xterm emulation.
 */
class TextIO : public File, public TimerHandler
{
private:
    static const int COLOUR_BRIGHT_ADDEND = 8;

    enum VgaColour
    {
        Black       =0,
        Blue        =1,
        Green       =2,
        Cyan        =3,
        Red         =4,
        Magenta     =5,
        Orange      =6,
        LightGrey   =7,
        DarkGrey    =8,
        LightBlue   =9,
        LightGreen  =10,
        LightCyan   =11,
        LightRed    =12,
        LightMagenta=13,
        Yellow      =14,
        White       =15
    };

    VgaColour adjustColour(int colour, bool up)
    {
        if(up)
        {
            return static_cast<VgaColour>(colour + COLOUR_BRIGHT_ADDEND);
        }
        else
        {
            return static_cast<VgaColour>(colour - COLOUR_BRIGHT_ADDEND);
        }
    }

    enum TerminalModes
    {
        LineFeedNewLine = 0x1,
        CursorKey       = 0x2,
        AnsiVt52        = 0x4,
        Column          = 0x8,
        Scrolling       = 0x10,
        Screen          = 0x20,
        Origin          = 0x40,
        AutoWrap        = 0x80,
        AutoRepeat      = 0x100,
        Interlace       = 0x200,

        /// AKA: 'Negative'
        Inverse         = 0x100000,

        /// AKA: 'Bold'
        Bright          = 0x200000,

        Blink           = 0x400000,

        // Character sets.
        CharacterSetG0  = 0x1000000,
        CharacterSetG1  = 0x2000000,
    };

public:
    TextIO(String str, size_t inode, Filesystem *pParentFS, File *pParent);
    virtual ~TextIO();

    /**
     * Initialise and prepare for rendering.
     * Clears the screen as a side-effect, unless false is passed in
     * for bClear.
     * \return True if we can now write, false otherwise.
     */
    bool initialise(bool bClear = true);
  
    /**
     * Write a string to the screen, handling any VT100 control sequences
     * embedded in the string along the way.
     * \param str The string to write.
     */
    void write(const char *s, size_t len);

    /**
     * VFS File interface.
     */
    virtual uint64_t read(uint64_t location, uint64_t size, uintptr_t buffer, bool bCanBlock = true);
    virtual uint64_t write(uint64_t location, uint64_t size, uintptr_t buffer, bool bCanBlock = true);
    virtual int select(bool bWriting = false, int timeout = 0);

    virtual void timer(uint64_t delta, InterruptState &state);

private:
    static const ssize_t BACKBUFFER_COLS_WIDE = 132;
    static const ssize_t BACKBUFFER_COLS_NORMAL = 80;
    static const ssize_t BACKBUFFER_ROWS = 25;

    static const ssize_t BACKBUFFER_STRIDE = BACKBUFFER_COLS_WIDE;

    void setColour(VgaColour *which, size_t param, bool bBright = false);

    void doBackspace();
    void doLinefeed();
    void doCarriageReturn();
    void doHorizontalTab();

    void checkScroll();
    void checkWrap();

    void eraseSOS();
    void eraseEOS();
    void eraseEOL();
    void eraseSOL();
    void eraseLine();

    void eraseScreen(uint8_t character);

    /**
     * Present backbuffer to the VGA instance.
     */
    void flip(bool timer = false, bool hideState = false);

    /** Translate given UTF32 codepoint to an ASCII character for display. */
    uint8_t translate(uint32_t codepoint);

    typedef struct
    {
        uint8_t character;
        VgaColour fore;
        VgaColour back;
        size_t flags;

        /** Used for blink and maybe privacy mode? Renders an empty cell. */
        bool hidden;
    } VgaCell;

    bool m_bInitialised;
    bool m_bControlSeq;
    bool m_bBracket;
    bool m_bParenthesis;
    bool m_bParams;
    bool m_bQuestionMark;
    ssize_t m_CursorX, m_CursorY;
    ssize_t m_SavedCursorX, m_SavedCursorY;
    ssize_t m_ScrollStart, m_ScrollEnd;
    ssize_t m_LeftMargin, m_RightMargin;
    size_t m_CurrentParam;
    size_t m_Params[MAX_TEXTIO_PARAMS];
    int m_CurrentModes;

    VgaColour m_Fore, m_Back;

    uint16_t *m_pFramebuffer;
    VgaCell *m_pBackbuffer;
    Vga *m_pVga;

    char m_TabStops[BACKBUFFER_STRIDE];

    /**
     * Output buffer - data that we as the emulator want to write back to
     * the application (eg, cursor position reports)
     */
    RingBuffer<char> m_OutBuffer;

    /**
     * Designated character sets - simply store the character which is used
     * to designate a character set (eg, '1', 'A').
     */
    uint8_t m_G0, m_G1;

    /** Timer interface: number of nanoseconds counted so far in the timer handler. */
    uint64_t m_Nanoseconds;

    /** Next interval to wait for (milliseconds). */
    uint64_t m_NextInterval;

    /** Are we dealing with a UTF-8 codepoint currently? */
    bool m_bUtf8;

    /** Current codepoint for handling. */
    uint32_t m_nCharacter;

    /** Number of Utf8 bits handled so far in this sequence. */
    size_t m_nUtf8Handled;
};

#endif
