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

#ifndef MACHINE_PPC_VGA_H
#define MACHINE_PPC_VGA_H

#include <processor/types.h>
#include <machine/Vga.h>
#include <processor/MemoryRegion.h>
//include <machine/openfirmware/OpenFirmware.h>
//include <machine/openfirmware/Device.h>

// We make an assumption about the device size.
#define MAX_WIDTH 1024
#define MAX_HEIGHT 800

#define FONT_WIDTH 8
#define FONT_HEIGHT 16

// Routines for taking a 0-255 set of RGB values and converting them to a 8,16 or 32 bpp specific form.
#ifdef BIG_ENDIAN
#define RGB_8(r,g,b) ( ((r&0x3)<<6) | ((g&0x7)<<3) | (b&0x7) )
#define RGB_16(r,g,b)  ( ((r&0x1f)<<11) | ((g&0x1f)<<6) | ((b&0x1f)<<1) )
#define RGB_32(r,g,b) ( 0 | r<<24 | g<<16 | b<<8 )
#else
#error Little endian not implemented here.
#endif

/**
 * Vga device abstraction.
 */
class PPCVga : public Vga
{
public:
  PPCVga();
  virtual ~PPCVga();

  void initialise();

  /**
   * Changes the mode the VGA device is in.
   * \param nCols The number of columns required.
   * \param nRows The number of rows required.
   * \param bIsText True if the caller requires a text mode, false if graphical.
   * \param nBpp Only applicable for graphics modes - the number of bits per pixel.
   * \return True on success, false on failure.
   */
  virtual bool setMode (int mode) {return false;}

  /**
   * Sets the largest possible text mode.
   * \return True on success, false on failure.
   */
  virtual bool setLargestTextMode () {return false;}

  /**
   * Tests the current video mode.
   * \return True if the current mode matches the given arguments.
   */
  virtual bool isMode (size_t nCols, size_t nRows, bool bIsText, size_t nBpp=0) {return false;}

  /**
   * Tests if the current video mode is the largest text mode.
   * \return True if the current video mode is equal to the largest text mode.
   */
  virtual bool isLargestTextMode () {return true;}

  /**
   * \return The number of columns in the current mode.
   */
  virtual size_t getNumCols () {return m_Width/FONT_WIDTH;}

  /**
   * \return The number of rows in the current mode.
   */
  virtual size_t getNumRows () {return m_Height/FONT_HEIGHT;}

  /**
   * Stores the current video mode.
   */
  virtual void rememberMode() {}

  /**
   * Restores the saved video mode from a rememberMode() call.
   */
  virtual void restoreMode() {}

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
   * \param A pointer to the buffer.
   * \param The length of pBuffer.
   */
  virtual void peekBuffer (uint8_t *pBuffer, size_t nBufLen);

  /**
   * Moves the cursor to the position specified by the parameters.
   * \param nX The column to move to.
   * \param nY The row to move to.
   */
  virtual void moveCursor (size_t nX, size_t nY) {}

  operator uint16_t*() const {return const_cast<uint16_t*> (m_pTextBuffer);}

private:
  PPCVga(const PPCVga &);
  PPCVga &operator = (const PPCVga &);

  void putChar(char c, int x, int y, unsigned int f, unsigned int b);

  /** We use a 16-bit-per-character text mode buffer to simplify things. BootIO is a bit of a
   *  simpleton, really. */
  uint16_t m_pTextBuffer[(MAX_WIDTH/FONT_WIDTH)*(MAX_HEIGHT/FONT_HEIGHT)];

  MemoryRegion m_Framebuffer;

  /** Our real graphics framebuffer
      \todo: This should be a MemoryMappedIO - needs VirtualAddressSpace though. */
  uint8_t *m_pFramebuffer;

  /** Screen width */
  uint32_t m_Width;
  /** Screen height */
  uint32_t m_Height;
  /** Screen depth - bits per pixel */
  uint32_t m_Depth;
  /** Stride - number of bytes per row */
  uint32_t m_Stride;
  /** Colour index table */
  uint32_t m_pColours[16];
};

#endif
