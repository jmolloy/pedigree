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

#ifndef MACHINE_ARM_VERSATILE_VGA_H
#define MACHINE_ARM_VERSATILE_VGA_H

#include <machine/Vga.h>

/**
 * Vga device abstraction.
 */
class ArmVersatileVga : public Vga
{
  public:
    ArmVersatileVga();
    virtual ~ArmVersatileVga();
  
    char read();
    void write(char c);
  
    /**
      * Changes the mode the VGA device is in.
      * \param nCols The number of columns required.
      * \param nRows The number of rows required.
      * \param bIsText True if the caller requires a text mode, false if graphical.
      * \param nBpp Only applicable for graphics modes - the number of bits per pixel.
      * \return True on success, false on failure.
    */
    bool setMode (size_t nCols, size_t nRows, bool bIsText, size_t nBpp=0) { return false; };
    
    /**
      * Sets the largest possible text mode.
      * \return True on success, false on failure.
    */
    bool setLargestTextMode () { return false; };
    
    /**
      * Tests the current video mode.
      * \return True if the current mode matches the given arguments.
    */
    bool isMode (size_t nCols, size_t nRows, bool bIsText, size_t nBpp=0) { return false; };
    
    /**
      * Tests if the current video mode is the largest text mode.
      * \return True if the current video mode is equal to the largest text mode.
    */
    bool isLargestTextMode () { return false; };
    
    /**
      * \return The number of columns in the current mode.
    */
    size_t getNumCols () { return 80; };
    
    /**
      * \return The number of rows in the current mode.
    */
    size_t getNumRows () { return 25; };
    
    /**
      * Stores the current video mode.
    */
    void rememberMode() {};
    
    /**
      * Restores the saved video mode from a rememberMode() call.
    */
    void restoreMode() {};
    
    /**
      * Copies the given buffer into video memory, replacing the current framebuffer.
      *
      * The buffer is assumed to be in the correct format for directly copying into video memory.
      * This will obviously depend on the current mode (text/graphical) as well as resolution and
      * bits per pixel (graphics mode only).
      * \param A pointer to the buffer to swap into video memory.
      * \param The length of pBuffer.
    */
    void pokeBuffer (uint8_t *pBuffer, size_t nBufLen) {};
    
    /**
      * Copies the current framebuffer into the given buffer.
      *
      * The buffer is assumed to be in the correct format for directly copying from video memory.
      * This will obviously depend on the current mode (text/graphical) as well as resolution and
      * bits per pixel (graphics mode only).
      * \param A pointer to the buffer.
      * \param The length of pBuffer.
    */
    void peekBuffer (uint8_t *pBuffer, size_t nBufLen) {};
    
    /**
      * Moves the cursor to the position specified by the parameters.
      * \param nX The column to move to.
      * \param nY The row to move to.
    */
    void moveCursor (size_t nX, size_t nY) {};
  
  operator uint16_t*() const {return 0;}

};

#endif
