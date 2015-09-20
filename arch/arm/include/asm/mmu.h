#ifndef __ARM_MMU_H
#define __ARM_MMU_H

#ifdef CONFIG_MMU

/*
 * apply kernel patch: b5466f8728527a05a493cc4abe9e6f034a1bbaab
 */
typedef struct {
#ifdef CONFIG_CPU_HAS_ASID
<<<<<<< HEAD
	u64 id;
=======
	atomic64_t	id;
#else
	int		switch_pending;
>>>>>>> v3.10.88
#endif
	unsigned int	vmalloc_seq;
	unsigned long	sigpage;
} mm_context_t;

/*
 * apply kernel patch: b5466f8728527a05a493cc4abe9e6f034a1bbaab
 */
#ifdef CONFIG_CPU_HAS_ASID
#define ASID_BITS	8
#define ASID_MASK	((~0ULL) << ASID_BITS)
<<<<<<< HEAD
#define ASID(mm)	((mm)->context.id & ~ASID_MASK)
=======
#define ASID(mm)	((mm)->context.id.counter & ~ASID_MASK)
>>>>>>> v3.10.88
#else
#define ASID(mm)	(0)
#endif

#else

/*
 * From nommu.h:
 *  Copyright (C) 2002, David McCullough <davidm@snapgear.com>
 *  modified for 2.6 by Hyok S. Choi <hyok.choi@samsung.com>
 */
typedef struct {
	unsigned long	end_brk;
} mm_context_t;

#endif

<<<<<<< HEAD
/*
 * switch_mm() may do a full cache flush over the context switch,
 * so enable interrupts over the context switch to avoid high
 * latency.
 */
#ifndef CONFIG_CPU_HAS_ASID
#define __ARCH_WANT_INTERRUPTS_ON_CTXSW
#endif

=======
>>>>>>> v3.10.88
#endif
