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

#ifndef KERNEL_PROCESSOR_PROCESSORINFORMATION_H
#define KERNEL_PROCESSOR_PROCESSORINFORMATION_H

/** @addtogroup kernelprocessor
 * @{ */

/** Identifier of a processor */
typedef size_t ProcessorId;

/** @} */

#if defined(X86_COMMON)
  #include <processor/x86_common/ProcessorInformation.h>
  #define PROCESSOR_SPECIFIC_NAME(x) X86Common##x
#elif defined(MIPS_COMMON)
  #include <processor/mips_common/ProcessorInformation.h>
  #define PROCESSOR_SPECIFIC_NAME(x) MIPSCommon##x
#elif defined(ARM_COMMON)
  #include <processor/arm_common/ProcessorInformation.h>
  #define PROCESSOR_SPECIFIC_NAME(x) ARM##x
#elif defined(PPC_COMMON)
  #include <processor/ppc_common/ProcessorInformation.h>
  #define PROCESSOR_SPECIFIC_NAME(x) PPCCommon##x
#endif

// NOTE: This throws a compile-time error if this header is not adapted for
//       the selected processor architecture
#if !defined(PROCESSOR_SPECIFIC_NAME)
  #error Unknown processor architecture
#endif

/** @addtogroup kernelprocessor
 * @{ */

// NOTE: If a newly added processor architecture does not supply all the
//       needed types, you will get an error here

/** Define ProcessorInformation */
typedef PROCESSOR_SPECIFIC_NAME(ProcessorInformation) ProcessorInformation;

/** @} */

#undef PROCESSOR_SPECIFIC_NAME

#endif
