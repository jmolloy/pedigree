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
#ifndef KERNEL_MACHINE_INITIALISEMACHINE_H
#define KERNEL_MACHINE_INITIALISEMACHINE_H

/** This is the first stage in the machine-dependent initialisation procedure.
 * This function should be called before any other machine-dependant class
 * methods or global functions. After this function only the memory-management
 * related functions and class methods may be called.
 * \note This is called from main()
 */
void initialiseMachine1();

/** This is the second stage in the machine-dependent initialisation procedure.
 * This function must be called after the memory-management has been initialised and
 * before any other machine-dependant class methods or global functions (except
 * those mentioned to be legal in initMachine1) is used.
 * After this function returns, any machine-dependant class methods and global
 * functions may be called.
 * \note This is called from main()
 */
void initialiseMachine2();

#endif
