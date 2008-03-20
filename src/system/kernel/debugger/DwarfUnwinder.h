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
#ifndef DWARFUNWINDER_H
#define DWARFUNWINDER_H

#include <compiler.h>
#include <processor/state.h>
#include <processor/types.h>

class DwarfUnwinder
{
  public:
    /**
     * Creates a DwarfUnwinder object defined by frame definitions at nData,
     * which has a maximum size of nLength.
     */
    DwarfUnwinder(uintptr_t nData, size_t nLength);
    ~DwarfUnwinder();
    
    /**
     * Populates outState with the result of unwinding one stack frame from inState.
     * \param[in] inState The starting state.
     * \param[out] outState The state after one stack frame has been unwound.
     * \return False if the frame could not be unwound, true otherwise.
     */
    bool unwind(const ProcessorState &inState, ProcessorState &outState);
    
    /**
     * Decodes a ULEB128 encoded number.
     * \param[in] pBase A pointer, to which nOffset is added to find the ULEB string.
     * \param[in,out] nOffset An offset to add to pBase to find the ULEB string. This is incremented
     *                        by the function, and the function ends with it pointing to the byte after
     *                        the ULEB number.
     * \return An integer representation of the ULEB128 number.
     * \see DWARF specification v3.0, Appendix C.
     */
    static uint32_t decodeUleb128(uint8_t *pBase, uint32_t &nOffset);

    /**
     * Decodes a SLEB128 encoded number.
     * \param[in] pBase A pointer, to which nOffset is added to find the SLEB string.
     * \param[in,out] nOffset An offset to add to pBase to find the SLEB string. This is incremented
     *                        by the function, and the function ends with it pointing to the byte after
     *                        the SLEB number.
     * \return An integer representation of the SLEB128 number.
     * \see DWARF specification v3.0, Appendix C.
     */
    static int32_t decodeSleb128(uint8_t *pBase, uint32_t &nOffset);
    
  private:
    /**
     * Address of our frame data.
     */
    uintptr_t m_nData;
    
    /**
     * Length of our frame data..
     */
    size_t m_nLength;
};

#endif
