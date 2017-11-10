/*-
 *   BSD LICENSE
 *
 * Copyright (c) 2008-2016 Freescale Semiconductor, Inc.
 * Copyright 2017 NXP
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *	 notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *	 notice, this list of conditions and the following disclaimer in the
 *	 documentation and/or other materials provided with the distribution.
 *     * Neither the name of Freescale Semiconductor nor the
 *	 names of its contributors may be used to endorse or promote products
 *	 derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY Freescale Semiconductor ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL Freescale Semiconductor BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef HEADER_COMPAT_H
#define HEADER_COMPAT_H

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif


#include <stdint.h>

#include <stdlib.h>
#include <stddef.h>
#include <pthread.h>
#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>
#include <assert.h>

/* The following definitions are primarily to allow the single-source driver
 * interfaces to be included by arbitrary program code. Ie. for interfaces that
 * are also available in kernel-space, these definitions provide compatibility
 * with certain attributes and types used in those interfaces.
 */

#define likely(x)	__builtin_expect(!!(x), 1)
#define unlikely(x)	__builtin_expect(!!(x), 0)

/* Debugging */
#define prflush(fmt, args...) \
	do { \
		printf(fmt, ##args); \
		fflush(stdout); \
	} while (0)
#define pr_crit(fmt, args...)	 prflush("CRIT:" fmt, ##args)
#define pr_err(fmt, args...)	 prflush("ERR:" fmt, ##args)
#define pr_warning(fmt, args...) prflush("WARN:" fmt, ##args)
#define pr_info(fmt, args...)	 prflush(fmt, ##args)

#ifdef CONFIG_BUGON
#ifdef pr_debug
#undef pr_debug
#endif
#define pr_debug(fmt, args...)	printf(fmt, ##args)
#define QBMAN_BUG_ON(c) \
do { \
	if (c) { \
		pr_crit("BUG: %s:%d\n", __FILE__, __LINE__); \
		abort(); \
	} \
} while(0)
#else
#ifdef pr_debug
#undef pr_debug
#endif
#define pr_debug(fmt, args...) {}
#define QBMAN_BUG_ON(c)		do { ; } while(0)
#endif

/* Other miscellaneous interfaces our APIs depend on; */


/* Atomic stuff */
#define smp_mb() { asm volatile("dmb ish": : : "memory"); }

#define dmb(opt) { asm volatile("dmb " #opt : : : "memory"); }
#define smp_mb() dmb(ish)
#define dma_wmb() dmb(ish)
typedef struct {
	int counter;
} atomic_t;

#define atomic_read(v)  (*(volatile int *)&(v)->counter)
#define atomic_set(v, i) (((v)->counter) = (i))
static inline void atomic_add(int i, atomic_t *v)
{
	unsigned long tmp;
	int result;

	asm volatile("// atomic_add\n"
	"1:	ldxr    %w0, %2\n"
	"	add     %w0, %w0, %w3\n"
	"	stxr    %w1, %w0, %2\n"
	"	cbnz    %w1, 1b"
	: "=&r" (result), "=&r" (tmp), "+Q" (v->counter)
	: "Ir" (i));
}

static inline int atomic_add_return(int i, atomic_t *v)
{
	unsigned long tmp;
	int result;

	asm volatile("// atomic_add_return\n"
	"1:	ldxr    %w0, %2\n"
	"	add     %w0, %w0, %w3\n"
	"	stlxr   %w1, %w0, %2\n"
	"	cbnz    %w1, 1b"
	: "=&r" (result), "=&r" (tmp), "+Q" (v->counter)
	: "Ir" (i)
	: "memory");

	smp_mb();
	return result;
}

static inline void atomic_sub(int i, atomic_t *v)
{
	unsigned long tmp;
	int result;

	asm volatile("// atomic_sub\n"
	"1:	ldxr    %w0, %2\n"
	"	sub     %w0, %w0, %w3\n"
	"	stxr    %w1, %w0, %2\n"
	"	cbnz    %w1, 1b"
	: "=&r" (result), "=&r" (tmp), "+Q" (v->counter)
	: "Ir" (i));
}

static inline int atomic_sub_return(int i, atomic_t *v)
{
	unsigned long tmp;
	int result;

	asm volatile("// atomic_sub_return\n"
	"1:	ldxr    %w0, %2\n"
	"	sub     %w0, %w0, %w3\n"
	"	stlxr   %w1, %w0, %2\n"
	"	cbnz    %w1, 1b"
	: "=&r" (result), "=&r" (tmp), "+Q" (v->counter)
	: "Ir" (i)
	: "memory");

	smp_mb();
	return result;
}

#define atomic_inc(v)           atomic_add(1, v)
#define atomic_dec(v)           atomic_sub(1, v)

#define atomic_inc_and_test(v)  (atomic_add_return(1, v) == 0)
#define atomic_dec_and_test(v)  (atomic_sub_return(1, v) == 0)
#define atomic_inc_return(v)    (atomic_add_return(1, v))
#define atomic_dec_return(v)    (atomic_sub_return(1, v))
#define atomic_sub_and_test(i, v) (atomic_sub_return(i, v) == 0)

#endif /* HEADER_COMPAT_H */
