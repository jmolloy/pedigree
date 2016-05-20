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

#ifndef KERNEL_MACHINE_HOSTED_TYPES_H
#define KERNEL_MACHINE_HOSTED_TYPES_H

#include <processor/types.h>

// Forward to the system's machine/types.h rather than replacing it altogether.
// We only do this for targets which already pull in things like stdlib.h
// (typically tools to run on the build system, not in Pedigree).
#if defined(__APPLE__) && defined(PEDIGREE_EXTERNAL_SOURCE)
#include_next <machine/types.h>
#endif

/** @addtogroup kernelmachinehostedcommon
 * @{ */

/** Define a type for IRQ identifications */
typedef uint8_t HostedCommonirq_id_t;

/** @} */

#endif
