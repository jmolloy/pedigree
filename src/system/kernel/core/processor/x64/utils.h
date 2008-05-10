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

#ifndef KERNEL_PROCESSOR_X64_UTILS_H
#define KERNEL_PROCESSOR_X64_UTILS_H

#include <processor/types.h>

/** @addtogroup kernelprocessorx64
 * @{ */

/** Get the virtual address from the physical address. This is possible on x64
 *  because the whole physical memory is mapped into the virtual address space
 *\param[in] address the physical address
 *\return the virtual address */
inline uintptr_t physicalAddress(physical_uintptr_t address)
{
  return static_cast<uintptr_t>(address + 0xFFFF800000000000);
}

/** Get the virtual address from the physical address. This is possible on x64
 *  because the whole physical memory is mapped into the virtual address space
 *\param[in] pAddress the physical address
 *\return the virtual address */
template<typename T>
inline T *physicalAddress(T *pAddress)
{
  return reinterpret_cast<T*>(reinterpret_cast<uint64_t>(pAddress) + 0xFFFF800000000000);
}

/** @} */

#endif
