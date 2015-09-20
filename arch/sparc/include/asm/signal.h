#ifndef __SPARC_SIGNAL_H
#define __SPARC_SIGNAL_H

#ifndef __ASSEMBLY__
#include <linux/personality.h>
#include <linux/types.h>
#endif
#include <uapi/asm/signal.h>

#ifndef __ASSEMBLY__
/*
 * DJHR
 * SA_STATIC_ALLOC is used for the sparc32 system to indicate that this
 * interrupt handler's irq structure should be statically allocated
 * by the request_irq routine.
 * The alternative is that arch/sparc/kernel/irq.c has carnal knowledge
 * of interrupt usage and that sucks. Also without a flag like this
 * it may be possible for the free_irq routine to attempt to free
 * statically allocated data.. which is NOT GOOD.
 *
 */
#define SA_STATIC_ALLOC         0x8000
<<<<<<< HEAD
#endif

#include <asm-generic/signal-defs.h>

struct __new_sigaction {
	__sighandler_t		sa_handler;
	unsigned long		sa_flags;
	__sigrestore_t		sa_restorer;  /* not used by Linux/SPARC yet */
	__new_sigset_t		sa_mask;
};

struct __old_sigaction {
	__sighandler_t		sa_handler;
	__old_sigset_t		sa_mask;
	unsigned long		sa_flags;
	void			(*sa_restorer)(void);  /* not used by Linux/SPARC yet */
};
#define __ARCH_HAS_SA_RESTORER
=======
>>>>>>> v3.10.88

#define __ARCH_HAS_KA_RESTORER
#define __ARCH_HAS_SA_RESTORER

#endif /* !(__ASSEMBLY__) */
#endif /* !(__SPARC_SIGNAL_H) */
