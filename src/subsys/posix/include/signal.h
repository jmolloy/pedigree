#ifndef _SIGNAL_H_
#define _SIGNAL_H_

#include "_ansi.h"
#include <sys/signal.h>
#include <sys/types.h>

_BEGIN_STD_C

#ifndef sig_atomic_t
typedef int	sig_atomic_t;		/* Atomic entity type (ANSI) */
#endif

typedef _sig_func_ptr sig_t;

#define SIG_DFL ((_sig_func_ptr)0)	/* Default action */
#define SIG_IGN ((_sig_func_ptr)1)	/* Ignore action */
#define SIG_ERR ((_sig_func_ptr)-1)	/* Error return */

struct _reent;

_sig_func_ptr _EXFUN(_signal_r, (struct _reent *, int, _sig_func_ptr));
int	_EXFUN(_raise_r, (struct _reent *, int));
int _EXFUN(kill, (pid_t, int));
int _EXFUN(sigaction, (int sig, const struct sigaction *act, struct sigaction *oact));

#ifndef _REENT_ONLY
_sig_func_ptr _EXFUN(signal, (int, _sig_func_ptr));
int           _EXFUN(raise, (int));
int           _EXFUN(kill, (int, int));
#endif

_END_STD_C

#endif /* _SIGNAL_H_ */
