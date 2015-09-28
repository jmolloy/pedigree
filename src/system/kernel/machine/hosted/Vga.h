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

#ifndef MACHINE_HOSTED_VGA_H
#define MACHINE_HOSTED_VGA_H

#include <machine/Vga.h>

/**
 * Vga device abstraction.
 */
class HostedVga : public Vga
{
  public:
    HostedVga();
    virtual ~HostedVga();

  /**
   * Sets the given attribute mode control.
   */
  virtual void setControl(VgaControl which);

  /**
   * Clears the given attribute mode control.
   */
  virtual void clearControl(VgaControl which);

  virtual bool setMode (int mode);

  /**
   * Sets the largest possible text mode.
   * \return True on success, false on failure.
   */
  virtual bool setLargestTextMode ();

  /**
   * Tests the current video mode.
   * \return True if the current mode matches the given arguments.
   */
  virtual bool isMode (size_t nCols, size_t nRows, bool bIsText, size_t nBpp=0);

  /**
   * Tests if the current video mode is the largest text mode.
   * \return True if the current video mode is equal to the largest text mode.
   */
  virtual bool isLargestTextMode ();

  /**
   * \return The number of columns in the current mode.
   */
  virtual size_t getNumCols ()
  {
    return m_nWidth;
  }

  /**
   * \return The number of rows in the current mode.
   */
  virtual size_t getNumRows ()
  {
    return m_nHeight;
  }

  /**
   * Stores the current video mode.
   */
  virtual void rememberMode();

  /**
   * Restores the saved video mode from a rememberMode() call.
   */
  virtual void restoreMode();

  /**
   * Copies the given buffer into video memory, replacing the current framebuffer.
   *
   * The buffer is assumed to be in the correct format for directly copying into video memory.
   * This will obviously depend on the current mode (text/graphical) as well as resolution and
   * bits per pixel (graphics mode only).
   * \param pBuffer A pointer to the buffer to swap into video memory.
   * \param nBufLen The length of pBuffer.
   */
  virtual void pokeBuffer (uint8_t *pBuffer, size_t nBufLen);

  /**
   * Copies the current framebuffer into the given buffer.
   *
   * The buffer is assumed to be in the correct format for directly copying from video memory.
   * This will obviously depend on the current mode (text/graphical) as well as resolution and
   * bits per pixel (graphics mode only).
   * \param pBuffer A pointer to the buffer.
   * \param nBufLen The length of pBuffer.
   */
  virtual void peekBuffer (uint8_t *pBuffer, size_t nBufLen);

  /**
   * Moves the cursor to the position specified by the parameters.
   * \param nX The column to move to.
   * \param nY The row to move to.
   */
  virtual void moveCursor (size_t nX, size_t nY);

  bool initialise();

  operator uint16_t*() const
  {
    return m_pBackbuffer;
  }

private:
  HostedVga(const HostedVga &);
  HostedVga &operator = (const HostedVga &);

  /**
   * Print the given VGA attribute as an ANSI escape sequence.
   */
  static void printAttrAsAnsi(uint8_t attr);

  /**
   * Fix up the given colour into a proper ANSI colour.
   */
  static uint8_t ansiColourFixup(uint8_t colour);

  /**
   * The width of the current mode.
   */
  size_t m_nWidth;

  /**
   * The height of the current mode.
   */
  size_t m_nHeight;

  /**
   * Cursor X.
   */
  size_t m_CursorX;

  /**
   * Cursor Y.
   */
  size_t m_CursorY;

  /**
   * We keep a count of the number of times we've entered the debugger without leaving it - when we enter the debugger,
   * we increment this. When we leave, we decrement. If we leave and it reaches zero, we change out of mode 0x3 into
   * the current video mode.
   */
  int m_ModeStack;

  /**
   * Current video mode.
   */
  int m_nMode;

  /**
   * Most recent VGA controls (as these get reset on mode change).
   */
  uint8_t m_nControls;

  /**
   * Backbuffer for text.
   */
  uint16_t *m_pBackbuffer;
};

#endif
