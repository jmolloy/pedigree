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
typedef int sigjmp_buf;

void	_EXFUN(longjmp,(jmp_buf __jmpb, int __retval));
int	    _EXFUN(setjmp,(jmp_buf __jmpb));

int     _EXFUN(sigsetjmp, (sigjmp_buf env, int savemask));
void    _EXFUN(siglongjmp, (sigjmp_buf env, int val));

// Legacy longjmp/setjmp that does not modify the signal mask.
int     _EXFUN(_longjmp,(jmp_buf __jmpb, int __retval));
int     _EXFUN(_setjmp,(jmp_buf __jmpb));

_END_STD_C

#endif /* _SETJMP_H_ */

