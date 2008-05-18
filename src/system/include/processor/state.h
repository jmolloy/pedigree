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

#ifndef KERNEL_PROCESSOR_STATE_H
#define KERNEL_PROCESSOR_STATE_H

#if defined(X86)
  #include <processor/x86/state.h>
  #define PROCESSOR_SPECIFIC_NAME(x) X86##x
#elif defined(X64)
  #include <processor/x64/state.h>
  #define PROCESSOR_SPECIFIC_NAME(x) X64##x
#elif defined(MIPS32)
  #include <processor/mips32/state.h>
  #define PROCESSOR_SPECIFIC_NAME(x) MIPS32##x
#elif defined(MIPS64)
  #include <processor/mips64/state.h>
  #define PROCESSOR_SPECIFIC_NAME(x) MIPS64##x
#elif defined(ARM926E)
  #include <processor/arm_926e/state.h>
  #define PROCESSOR_SPECIFIC_NAME(x) ARM926E##x
#elif defined(PPC32)
  #include <processor/ppc32/state.h>
  #define PROCESSOR_SPECIFIC_NAME(x) PPC32##x
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

/** Lift the processor-specifc InterruptState class into the global namespace */
typedef PROCESSOR_SPECIFIC_NAME(InterruptState) InterruptState;
/** Lift the processor-specifc SyscallState class into the global namespace */
typedef PROCESSOR_SPECIFIC_NAME(SyscallState) SyscallState;
/** Lift the processor-specific ProcessorState class into the global namespace */
typedef PROCESSOR_SPECIFIC_NAME(ProcessorState) ProcessorState;

/** @} */

#undef PROCESSOR_SPECIFIC_NAME

#endif
