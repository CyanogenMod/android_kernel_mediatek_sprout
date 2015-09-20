#ifndef __ASM_GENERIC_SIGNAL_H
#define __ASM_GENERIC_SIGNAL_H

#include <uapi/asm-generic/signal.h>

#ifndef __ASSEMBLY__
<<<<<<< HEAD
typedef struct {
	unsigned long sig[_NSIG_WORDS];
} sigset_t;

/* not actually used, but required for linux/syscalls.h */
typedef unsigned long old_sigset_t;

#include <asm-generic/signal-defs.h>

#ifdef SA_RESTORER
#define __ARCH_HAS_SA_RESTORER
#endif

struct sigaction {
	__sighandler_t sa_handler;
	unsigned long sa_flags;
=======
>>>>>>> v3.10.88
#ifdef SA_RESTORER
#endif

#include <asm/sigcontext.h>
#undef __HAVE_ARCH_SIG_BITOPS

#endif /* __ASSEMBLY__ */
#endif /* _ASM_GENERIC_SIGNAL_H */
