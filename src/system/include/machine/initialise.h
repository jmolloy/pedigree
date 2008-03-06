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
#ifndef KERNEL_MACHINE_INITIALISE_H
#define KERNEL_MACHINE_INITIALISE_H

/** @ingroup kernelmachine
 * @{ */

/** This is the first stage in the machine-dependent initialisation procedure.
 * This function should be called after the processor-specific interface has
 * been fully initialised
 *\note only called by main() */
void initialiseMachine();

/** Has the machine-specific interface already been initialised?
 *\return true, if initialiseMachine() was already called and completed successfully, false otherwise */
bool isMachineInitialised();

/** @} */

#endif
