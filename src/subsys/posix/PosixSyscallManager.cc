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

#include <processor/SyscallManager.h>
#include <processor/Processor.h>
#include <process/Scheduler.h>
#include <Log.h>

#include "PosixSyscallManager.h"
#include "syscallNumbers.h"
#include "file-syscalls.h"
#include "system-syscalls.h"
#include "console-syscalls.h"
#include "net-syscalls.h"
#include "pipe-syscalls.h"
#include "signal-syscalls.h"
#include "sem-syscalls.h"
#include "pthread-syscalls.h"
#include "pedigree-syscalls.h"

PosixSyscallManager::PosixSyscallManager()
{
}

PosixSyscallManager::~PosixSyscallManager()
{
}

void PosixSyscallManager::initialise()
{
    SyscallManager::instance().registerSyscallHandler(posix, this);
    pedigree_config_init();
}

uintptr_t PosixSyscallManager::call(uintptr_t function, uintptr_t p1, uintptr_t p2, uintptr_t p3, uintptr_t p4, uintptr_t p5)
{
    if (function >= serviceEnd)
    {
        ERROR("PosixSyscallManager: invalid function called: " << Dec << static_cast<int>(function));
        return 0;
    }
    return SyscallManager::instance().syscall(posix, function, p1, p2, p3, p4, p5);
}

uintptr_t PosixSyscallManager::syscall(SyscallState &state)
{
    uintptr_t p1 = state.getSyscallParameter(0);
    uintptr_t p2 = state.getSyscallParameter(1);
    uintptr_t p3 = state.getSyscallParameter(2);
    uintptr_t p4 = state.getSyscallParameter(3);
    uintptr_t p5 = state.getSyscallParameter(4);

    //NOTICE("[" << Processor::information().getCurrentThread()->getParent()->getId() << "] : " << Dec << state.getSyscallNumber() << Hex);

    // We're interruptible.
    Processor::setInterrupts(true);

    switch (state.getSyscallNumber())
    {
        // POSIX system calls
        case POSIX_OPEN:
            return posix_open(reinterpret_cast<const char*>(p1), p2, p3);
        case POSIX_WRITE:
            return posix_write(p1, reinterpret_cast<char*>(p2), p3);
        case POSIX_READ:
            return posix_read(p1, reinterpret_cast<char*>(p2), p3);
        case POSIX_CLOSE:
            return posix_close(p1);
        case POSIX_SBRK:
            return posix_sbrk(p1);
        case POSIX_FORK:
            return posix_fork(state);
        case POSIX_EXECVE:
            return posix_execve(reinterpret_cast<const char*>(p1), reinterpret_cast<const char**>(p2), reinterpret_cast<const char**>(p3), state);
        case POSIX_WAITPID:
            return posix_waitpid(p1, reinterpret_cast<int*> (p2), p3);
        case POSIX_EXIT:
            return posix_exit(p1);
        case POSIX_OPENDIR:
            return posix_opendir(reinterpret_cast<const char*>(p1), reinterpret_cast<dirent*>(p2));
        case POSIX_READDIR:
            return posix_readdir(p1, reinterpret_cast<dirent*>(p2));
        case POSIX_REWINDDIR:
            posix_rewinddir(p1, reinterpret_cast<dirent*>(p2));
            return 0;
        case POSIX_CLOSEDIR:
            return posix_closedir(p1);
        case POSIX_TCGETATTR:
            return posix_tcgetattr(p1, reinterpret_cast<struct termios*>(p2));
        case POSIX_TCSETATTR:
            return posix_tcsetattr(p1, p2, reinterpret_cast<struct termios*>(p3));
        case POSIX_IOCTL:
            return posix_ioctl(p1, p2, reinterpret_cast<void*>(p3));
        case POSIX_STAT:
            return posix_stat(reinterpret_cast<const char*>(p1), reinterpret_cast<struct stat*>(p2));
        case POSIX_FSTAT:
            return posix_fstat(p1, reinterpret_cast<struct stat*>(p2));
        case POSIX_GETPID:
            return posix_getpid();
        case POSIX_CHDIR:
            return posix_chdir(reinterpret_cast<const char*>(p1));
        case POSIX_SELECT:
            return posix_select(static_cast<int>(p1), reinterpret_cast<struct fd_set*>(p2), reinterpret_cast<struct fd_set*>(p3),
                reinterpret_cast<struct fd_set*>(p4), reinterpret_cast<struct timeval*>(p5));
        case POSIX_LSEEK:
            return posix_lseek(static_cast<int>(p1), static_cast<off_t>(p2), static_cast<int>(p3));
        case POSIX_SOCKET:
            return posix_socket(static_cast<int>(p1), static_cast<int>(p2), static_cast<int>(p3));
        case POSIX_CONNECT:
            return posix_connect(static_cast<int>(p1), reinterpret_cast<sockaddr*>(p2), static_cast<size_t>(p3));
        case POSIX_SEND:
            return posix_send(static_cast<int>(p1), reinterpret_cast<void*>(p2), static_cast<size_t>(p3), static_cast<int>(p4));
        case POSIX_RECV:
            return posix_recv(static_cast<int>(p1), reinterpret_cast<void*>(p2), static_cast<size_t>(p3), static_cast<int>(p4));
        case POSIX_BIND:
            return posix_bind(static_cast<int>(p1), reinterpret_cast<sockaddr*>(p2), static_cast<size_t>(p3));
        case POSIX_LISTEN:
            return posix_listen(static_cast<int>(p1), static_cast<int>(p2));
        case POSIX_ACCEPT:
            return posix_accept(static_cast<int>(p1), reinterpret_cast<sockaddr*>(p2), reinterpret_cast<size_t*>(p3));
        case POSIX_RECVFROM:
            return posix_recvfrom(reinterpret_cast<void*>(p1));
        case POSIX_SENDTO:
            return posix_sendto(reinterpret_cast<void*>(p1));
        case POSIX_GETTIMEOFDAY:
            return posix_gettimeofday(reinterpret_cast<struct timeval *>(p1), reinterpret_cast<struct timezone *>(p2));
        case POSIX_DUP:
            return posix_dup(static_cast<int>(p1));
        case POSIX_DUP2:
            return posix_dup2(static_cast<int>(p1), static_cast<int>(p2));
        case POSIX_LSTAT:
            return posix_lstat(reinterpret_cast<char*>(p1), reinterpret_cast<struct stat*>(p2));
        case POSIX_UNLINK:
            return posix_unlink(reinterpret_cast<char*>(p1));
        case POSIX_SYMLINK:
            return posix_symlink(reinterpret_cast<char*>(p1), reinterpret_cast<char*>(p2));
        case POSIX_GETHOSTBYNAME:
            return posix_gethostbyname(reinterpret_cast<const char*>(p1), reinterpret_cast<void*>(p2), static_cast<int>(p3));
        case POSIX_GETHOSTBYADDR:
            return posix_gethostbyaddr(reinterpret_cast<const void*>(p1), static_cast<unsigned long>(p2), static_cast<int>(p3), reinterpret_cast<void*>(p4));
        case POSIX_FCNTL:
            return posix_fcntl(static_cast<int>(p1), static_cast<int>(p2), static_cast<int>(p3), reinterpret_cast<int*>(p4));
        case POSIX_PIPE:
            return posix_pipe(reinterpret_cast<int*>(p1));
        case POSIX_MKDIR:
            return posix_mkdir(reinterpret_cast<const char*>(p1), static_cast<int>(p2));
        case POSIX_GETPWENT:
            return posix_getpwent(reinterpret_cast<passwd*>(p1), static_cast<int>(p2), reinterpret_cast<char *>(p3));
        case POSIX_GETPWNAM:
            return posix_getpwnam(reinterpret_cast<passwd*>(p1), reinterpret_cast<const char*>(p2), reinterpret_cast<char *>(p3));
        case POSIX_GETUID:
            return posix_getuid();
        case POSIX_GETGID:
            return posix_getgid();
        case POSIX_SIGACTION:
            return posix_sigaction(static_cast<int>(p1), reinterpret_cast<const struct sigaction*>(p2), reinterpret_cast<struct sigaction*>(p3));
        case POSIX_SIGNAL:
            return posix_signal(static_cast<int>(p1), reinterpret_cast<void*>(p2));
        case POSIX_RAISE:
            return posix_raise(static_cast<int>(p1), state);
        case POSIX_KILL:
            return posix_kill(static_cast<int>(p1), static_cast<int>(p2));
        case POSIX_SIGPROCMASK:
            return posix_sigprocmask(static_cast<int>(p1), reinterpret_cast<const uint32_t*>(p2), reinterpret_cast<uint32_t*>(p3));
        case POSIX_ALARM:
            return posix_alarm(static_cast<uint32_t>(p1));
        case POSIX_SLEEP:
            return posix_sleep(static_cast<uint32_t>(p1));
        case POSIX_DLOPEN:
            return posix_dlopen(reinterpret_cast<const char*>(p1), static_cast<int>(p2), reinterpret_cast<void*>(p3));
        case POSIX_DLSYM:
            return posix_dlsym(reinterpret_cast<void*>(p1), reinterpret_cast<const char*>(p2));
        case POSIX_DLCLOSE:
            return posix_dlclose(reinterpret_cast<void*>(p1));
        case POSIX_POLL:
            return posix_poll(reinterpret_cast<pollfd*>(p1), static_cast<unsigned int>(p2), static_cast<int>(p3));
        case POSIX_RENAME:
            return posix_rename(reinterpret_cast<const char*>(p1), reinterpret_cast<const char*>(p2));
        case POSIX_GETCWD:
            return reinterpret_cast<uintptr_t>(posix_getcwd(reinterpret_cast<char*>(p1), static_cast<size_t>(p2)));
        case POSIX_READLINK:
            return posix_readlink(reinterpret_cast<const char*>(p1), reinterpret_cast<char*>(p2), static_cast<unsigned int>(p3));
        case POSIX_LINK:
            return posix_link(reinterpret_cast<char*>(p1), reinterpret_cast<char*>(p2));
        case POSIX_ISATTY:
            return posix_isatty(static_cast<int>(p1));
        case POSIX_MMAP:
            return reinterpret_cast<uintptr_t>(posix_mmap(reinterpret_cast<void*>(p1)));
        case POSIX_MUNMAP:
            return posix_munmap(reinterpret_cast<void*>(p1), static_cast<size_t>(p2));
        case POSIX_SHUTDOWN:
            return posix_shutdown(static_cast<int>(p1), static_cast<int>(p2));
        case POSIX_ACCESS:
            return posix_access(reinterpret_cast<const char*>(p1), static_cast<int>(p2));
        case POSIX_SETSID:
            return posix_setsid();
        case POSIX_SETPGID:
            return posix_setpgid(static_cast<int>(p1), static_cast<int>(p2));
        case POSIX_GETPGRP:
            return posix_getpgrp();
        case POSIX_SIGALTSTACK:
            return posix_sigaltstack(reinterpret_cast<const stack_t *>(p1), reinterpret_cast<stack_t *>(p2));

        case POSIX_SEM_CLOSE:
            return posix_sem_close(reinterpret_cast<sem_t*>(p1));
        case POSIX_SEM_DESTROY:
            return posix_sem_destroy(reinterpret_cast<sem_t*>(p1));
        case POSIX_SEM_GETVALUE:
            return posix_sem_getvalue(reinterpret_cast<sem_t*>(p1), reinterpret_cast<int*>(p2));
        case POSIX_SEM_INIT:
            return posix_sem_init(reinterpret_cast<sem_t*>(p1), static_cast<int>(p2), static_cast<unsigned>(p3));
        case POSIX_SEM_POST:
            return posix_sem_post(reinterpret_cast<sem_t*>(p1));
        case POSIX_SEM_TIMEWAIT:
            return posix_sem_timedwait(reinterpret_cast<sem_t*>(p1), reinterpret_cast<timespec*>(p2));
        case POSIX_SEM_TRYWAIT:
            return posix_sem_trywait(reinterpret_cast<sem_t*>(p1));
        case POSIX_SEM_WAIT:
            return posix_sem_wait(reinterpret_cast<sem_t*>(p1));

        /// \todo Implement me! :(
        // case POSIX_SEM_OPEN:
        //     return posix_sem_open(......
        // case POSIX_SEM_UNLINK:
        //     return posix_sem_unlink(.....

        case POSIX_PTHREAD_RETURN:
            posix_pthread_exit(reinterpret_cast<void*>(p1));
            return 0;
        case POSIX_PTHREAD_ENTER:
            return posix_pthread_enter(p1);

        case POSIX_PTHREAD_CREATE:
            return posix_pthread_create(reinterpret_cast<pthread_t*>(p1), reinterpret_cast<const pthread_attr_t*>(p2), reinterpret_cast<pthreadfn>(p3), reinterpret_cast<void*>(p4));
        case POSIX_PTHREAD_JOIN:
            return posix_pthread_join(static_cast<pthread_t>(p1), reinterpret_cast<void **>(p2));
        case POSIX_PTHREAD_DETACH:
            return posix_pthread_detach(static_cast<pthread_t>(p1));
        case POSIX_PTHREAD_SELF:
            return posix_pthread_self();
        case POSIX_PTHREAD_KILL:
            return posix_pthread_kill(static_cast<pthread_t>(p1), static_cast<int>(p2));
        case POSIX_PTHREAD_SIGMASK:
            return posix_pthread_sigmask(static_cast<int>(p1), reinterpret_cast<const uint32_t*>(p2), reinterpret_cast<uint32_t*>(p3));
        case POSIX_PTHREAD_MUTEX_INIT:
            return posix_pthread_mutex_init(reinterpret_cast<pthread_mutex_t*>(p1), reinterpret_cast<const pthread_mutexattr_t*>(p2));
        case POSIX_PTHREAD_MUTEX_DESTROY:
            return posix_pthread_mutex_destroy(reinterpret_cast<pthread_mutex_t*>(p1));
        case POSIX_PTHREAD_MUTEX_LOCK:
            return posix_pthread_mutex_lock(reinterpret_cast<pthread_mutex_t*>(p1));
        case POSIX_PTHREAD_MUTEX_TRYLOCK:
            return posix_pthread_mutex_trylock(reinterpret_cast<pthread_mutex_t*>(p1));
        case POSIX_PTHREAD_MUTEX_UNLOCK:
            return posix_pthread_mutex_unlock(reinterpret_cast<pthread_mutex_t*>(p1));
        case POSIX_PTHREAD_KEY_CREATE:
            return posix_pthread_key_create(reinterpret_cast<pthread_key_t*>(p1), reinterpret_cast<key_destructor>(p2));
        case POSIX_PTHREAD_KEY_DELETE:
            return posix_pthread_key_delete(static_cast<pthread_key_t>(p1));
        case POSIX_PTHREAD_GETSPECIFIC:
            return reinterpret_cast<uintptr_t>(posix_pthread_getspecific(static_cast<pthread_key_t>(p1)));
        case POSIX_PTHREAD_SETSPECIFIC:
            return posix_pthread_setspecific(static_cast<pthread_key_t>(p1), reinterpret_cast<const void*>(p2));
        case POSIX_PTHREAD_KEY_DESTRUCTOR:
            return reinterpret_cast<uintptr_t>(posix_pthread_key_destructor(static_cast<pthread_key_t>(p1)));

        case POSIX_SYSLOG:
            return posix_syslog(reinterpret_cast<const char*>(p1), static_cast<int>(p2));
		
		case POSIX_FTRUNCATE:
			return posix_ftruncate(static_cast<int>(p1), static_cast<off_t>(p2));

        // Stub warning
        case POSIX_STUBBED:
            // This is the solution to a bug - if the address in p1 traps (because of demand loading),
            // it MUST trap before we get the log spinlock, else other things will
            // want to write to it and deadlock.
            static char buf[128];
            strncpy(buf, reinterpret_cast<char*>(p1), 128);
            WARNING("Using stubbed function '" << buf << "'");
            return 0;

            // Pedigree system calls, called from POSIX applications
        case PEDIGREE_LOGIN:
            return pedigree_login(static_cast<int>(p1), reinterpret_cast<const char *>(p2));
        case PEDIGREE_SIGRET:
            return pedigree_sigret();
        case PEDIGREE_INIT_SIGRET:
            pedigree_init_sigret();
            return 0;
        case PEDIGREE_LOAD_KEYMAP:
            return pedigree_load_keymap(reinterpret_cast<char*>(p1), static_cast<size_t>(p2));
        case PEDIGREE_GET_MOUNT:
            return pedigree_get_mount(reinterpret_cast<char*>(p1), reinterpret_cast<char*>(p2), static_cast<size_t>(p3));
        case PEDIGREE_CONFIG_GETCOLNAME:
            pedigree_config_getcolname(p1, p2, reinterpret_cast<char*>(p3), p4);
            return 0;
        case PEDIGREE_CONFIG_GETSTR_N:
            pedigree_config_getstr(p1, p2, reinterpret_cast<char*>(p3), p4);
            return 0;
        case PEDIGREE_CONFIG_GETSTR_S:
            pedigree_config_getstr(p1, reinterpret_cast<const char*>(p2), reinterpret_cast<char*>(p3), p4);
            return 0;
        case PEDIGREE_CONFIG_GETNUM_N:
            return pedigree_config_getnum(p1, p2);
        case PEDIGREE_CONFIG_GETNUM_S:
            return pedigree_config_getnum(p1, reinterpret_cast<const char*>(p2));
        case PEDIGREE_CONFIG_GETBOOL_N:
            return pedigree_config_getbool(p1, p2);
        case PEDIGREE_CONFIG_GETBOOL_S:
            return pedigree_config_getbool(p1, reinterpret_cast<const char*>(p2));
        case PEDIGREE_CONFIG_QUERY:
            return pedigree_config_query(reinterpret_cast<const char*>(p1));
        case PEDIGREE_CONFIG_FREERESULT:
            pedigree_config_freeresult(p1);
            return 0;
        case PEDIGREE_CONFIG_NUMCOLS:
            return pedigree_config_numcols(p1);
        case PEDIGREE_CONFIG_NUMROWS:
            return pedigree_config_numrows(p1);
        case PEDIGREE_CONFIG_NEXTROW:
            return pedigree_config_nextrow(p1);
        case PEDIGREE_CONFIG_WAS_SUCCESSFUL:
            return pedigree_config_was_successful(p1);
        case PEDIGREE_CONFIG_GET_ERROR_MESSAGE:
            pedigree_config_get_error_message(p1, reinterpret_cast<char*>(p2), p3);
            return 0;
        default: ERROR ("PosixSyscallManager: invalid syscall received: " << Dec << state.getSyscallNumber()); return 0;
    }
}
