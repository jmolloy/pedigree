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

#ifndef FILELOADER_H
#define FILELOADER_H

/** @addtogroup kernellinker
 * @{ */

class FileLoader
{
public:
  /**
   * Constructor. Doesn't do much.
   */
  FileLoader() {}
  virtual ~FileLoader() {}

  /**
   * This explicit load function allows the use of global concrete objects.
   * \param pFile Pointer to the contents of the file, in accessible memory. The memory it
   *              is in is considered temporary and may be destroyed by the caller when this
   *              function completes.
   * \param nBufferLength Length of the buffer pointed to by pFile.
   */
  virtual bool load(uint8_t *pFile, unsigned int nBufferLength) =0;
  
  /**
   * Looks up the symbol at location 'nAddress'. Returns NULL if none found.
   * \param[in] nAddress The address to look up.
   * \param[out] pSymbolStart The start location of the retrieved symbol (Optional).
   */
  virtual const char *lookupSymbol(uintptr_t nAddress, uintptr_t *pSymbolStart) =0;
  
  virtual uintptr_t debugFrameTable() = 0;
  virtual uintptr_t debugFrameTableLength() = 0;
};

/** @} */

#endif
