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

/*
	setjmp.h
	stubs for future use.
*/

#ifndef _SETJMP_H_
#define _SETJMP_H_

#include "_ansi.h"
#include <machine/setjmp.h>

_BEGIN_STD_C

// gettext uses this for handling SIGFPE, which isn't yet forwarded to
// applications.
///todo Proper code for sigjmp
typedef jmp_buf sigjmp_buf;

void	_EXFUN(longjmp,(jmp_buf __jmpb, int __retval)) _ATTRIBUTE((noreturn));
int	    _EXFUN(setjmp,(jmp_buf __jmpb));

int     _EXFUN(sigsetjmp, (sigjmp_buf env, int savemask));
void    _EXFUN(siglongjmp, (sigjmp_buf env, int val)) _ATTRIBUTE((noreturn));

// Legacy longjmp/setjmp that does not modify the signal mask.
int     _EXFUN(_longjmp,(jmp_buf __jmpb, int __retval)) _ATTRIBUTE((noreturn));
int     _EXFUN(_setjmp,(jmp_buf __jmpb));

_END_STD_C

#endif /* _SETJMP_H_ */

