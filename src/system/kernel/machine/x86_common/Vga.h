/*
 * Copyright (c) 2008 James Molloy, James Pritchett, Jörg Pfähler, Matthew Iselin
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
#ifndef MACHINE_X86_VGA_H
#define MACHINE_X86_VGA_H

#include <machine/Vga.h>
#include <processor/IoPort.h>

#define VGA_BASE                0x3C0
#define VGA_AC_INDEX            0x0
#define VGA_AC_WRITE            0x0
#define VGA_AC_READ             0x1
#define VGA_MISC_WRITE          0x2
#define VGA_SEQ_INDEX           0x4
#define VGA_SEQ_DATA            0x5
#define VGA_DAC_READ_INDEX      0x7
#define VGA_DAC_WRITE_INDEX     0x8
#define VGA_DAC_DATA            0x9
#define VGA_MISC_READ           0xC
#define VGA_GC_INDEX            0xE
#define VGA_GC_DATA             0xF
/*                              COLOR emulation  MONO emulation */
#define VGA_CRTC_INDEX          0x14             /* 0x3B4 */
#define VGA_CRTC_DATA           0x15             /* 0x3B5 */
#define VGA_INSTAT_READ         0x1A

#define VGA_NUM_SEQ_REGS        5
#define VGA_NUM_CRTC_REGS       25
#define VGA_NUM_GC_REGS         9
#define VGA_NUM_AC_REGS         21
#define VGA_NUM_REGS            (1 + VGA_NUM_SEQ_REGS + VGA_NUM_CRTC_REGS + \
                                VGA_NUM_GC_REGS + VGA_NUM_AC_REGS)

/**
 * Vga device abstraction.
 */
class X86Vga : public Vga
{
  public:
    X86Vga(uint32_t nRegisterBase, uint32_t nFramebufferBase);
    virtual ~X86Vga();
  
    /**
   * Changes the mode the VGA device is in.
   * \param nCols The number of columns required.
   * \param nRows The number of rows required.
   * \param bIsText True if the caller requires a text mode, false if graphical.
   * \param nBpp Only applicable for graphics modes - the number of bits per pixel.
   * \return True on success, false on failure.
   */
  virtual bool setMode (size_t nCols, size_t nRows, bool bIsText, size_t nBpp=0);
  
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

private:
  X86Vga(const X86Vga &);
  X86Vga &operator = (const X86Vga &);

  /**
   * The IoPort to access control registers.
   */
  IoPort m_RegisterPort;
  
  /**
   * The framebuffer.
   */
  uint8_t *m_pFramebuffer;
  
  /**
   * The width of the current mode.
   */
  size_t m_nWidth;
  
  /**
   * The height of the current mode.
   */
  size_t m_nHeight;
  
  /**
   * Sets a specific vga mode.
   */
  bool setMode(int nMode);
  
  /**
   * Tries to identify the current vga mode.
   */
  int getMode();
  
  uint8_t m_pStoredMode[61];
  
  int m_nMode;
};

#endif
