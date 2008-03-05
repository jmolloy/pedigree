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
#ifndef KERNEL_MACHINE_TYPES_H
#define KERNEL_MACHINE_TYPES_H

#ifdef X86_COMMON
  #include <machine/x86_common/types.h>
  #define MACHINE_SPECIFIC_NAME(x) X86Common##x
#endif
#ifdef MIPS_COMMON
  #include <machine/mips_common/types.h>
  #define MACHINE_SPECIFIC_NAME(x) MIPSCommon##x
#endif


// NOTE: This throws a compile-time error if this header is not adapted for
//       the selected machine architecture
#ifndef MACHINE_SPECIFIC_NAME
  #error Unknown machine architecture
#endif

/** @addtogroup kernelmachine machine-specifc kernel
 * machine-specific kernel interface
 *  @ingroup kernel
 * @{ */

// NOTE: If a newly added machine architecture does not supply all the
//       needed types, you will get an error here

/** Define a type for IRQ identifications */
typedef MACHINE_SPECIFIC_NAME(irq_id_t) irq_id_t;

/** @} */

#undef MACHINE_SPECIFIC_NAME

#endif
