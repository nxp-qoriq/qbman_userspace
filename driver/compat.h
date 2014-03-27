/* Copyright (c) 2008-2011 Freescale Semiconductor, Inc.
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
 *
 * ALTERNATIVELY, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") as published by the Free Software
 * Foundation, either version 2 of that License or (at your option) any
 * later version.
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

#include <sched.h>

/* All <usdpaa/xxx.h> headers include this header, directly or otherwise. This
 * should provide the minimal set of system includes and base-definitions
 * required by these headers, such that C code can include USDPAA headers
 * without pre-requisites. */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <net/ethernet.h>

/* The following definitions are primarily to allow the single-source driver
 * interfaces to be included by arbitrary program code. Ie. for interfaces that
 * are also available in kernel-space, these definitions provide compatibility
 * with certain attributes and types used in those interfaces. */

/* Required compiler attributes */
#define __maybe_unused	__attribute__((unused))
#define __always_unused	__attribute__((unused))
#define __packed	__attribute__((__packed__))
#define __user
#define likely(x)	__builtin_expect(!!(x), 1)
#define unlikely(x)	__builtin_expect(!!(x), 0)
#define ____cacheline_aligned __attribute__((aligned(L1_CACHE_BYTES)))
#define container_of(p, t, f) (t *)((void *)p - offsetof(t, f))
#define __stringify_1(x) #x
#define __stringify(x)	__stringify_1(x)
#define panic(x) \
do { \
	printf("panic: %s", x); \
	abort(); \
} while (0)

#ifdef ARRAY_SIZE
#undef ARRAY_SIZE
#endif
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

/* Required types */
typedef uint8_t		u8;
typedef uint16_t	u16;
typedef uint32_t	u32;
typedef uint64_t	u64;
typedef uint64_t	dma_addr_t;
typedef cpu_set_t	cpumask_t;
#define spinlock_t	pthread_mutex_t
struct rb_node {
	struct rb_node *prev, *next;
};

typedef	u32		compat_uptr_t;
static inline void __user *compat_ptr(compat_uptr_t uptr)
{
	return (void __user *)(unsigned long)uptr;
}

static inline compat_uptr_t ptr_to_compat(void __user *uptr)
{
	return (u32)(unsigned long)uptr;
}

/* SMP stuff */
static inline int cpumask_test_cpu(int cpu, cpumask_t *mask)
{
	return CPU_ISSET(cpu, mask);
}
static inline void cpumask_set_cpu(int cpu, cpumask_t *mask)
{
	CPU_SET(cpu, mask);
}
static inline void cpumask_clear_cpu(int cpu, cpumask_t *mask)
{
	CPU_CLR(cpu, mask);
}
#define DEFINE_PER_CPU(t, x)	__thread t per_cpu__##x
#define per_cpu(x, c)		per_cpu__##x
#define get_cpu_var(x)		per_cpu__##x
#define __get_cpu_var(x)	per_cpu__##x
#define put_cpu_var(x)		do {; } while (0)
#define __PERCPU		__thread
/* to be used as an upper-limit only */
#define NR_CPUS			64

/* Atomic stuff */
typedef struct {
	volatile long v;
} atomic_t;
/* NB: __atomic_*() functions copied and twiddled from lwe_atomic.h */
static inline int atomic_read(const atomic_t *v)
{
	return v->v;
}
static inline void atomic_set(atomic_t *v, int i)
{
	v->v = i;
}
static inline long
__atomic_add(long *ptr, long val)
{
	long ret;

	/* FIXME 64-bit */
	asm volatile("1: lwarx %0, %y1;"
		     "add %0, %0, %2;"
		     "stwcx. %0, %y1;"
		     "bne 1b;" :
		     "=&r" (ret), "+Z" (*ptr) :
		     "r" (val) :
		     "memory", "cc");

	return ret;
}
static inline void atomic_inc(atomic_t *v)
{
	__atomic_add((long *)&v->v, 1);
}
static inline int atomic_dec_and_test(atomic_t *v)
{
	return __atomic_add((long *)&v->v, -1) == 0;
}
static inline void atomic_dec(atomic_t *v)
{
	__atomic_add((long *)&v->v, -1);
}

/* new variants not present in LWE */
static inline int atomic_inc_and_test(atomic_t *v)
{
	return __atomic_add((long *)&v->v, 1) == 0;
}

static inline int atomic_inc_return(atomic_t *v)
{
	return	__atomic_add((long *)&v->v, 1);
}

/* Waitqueue stuff */
typedef struct { }		wait_queue_head_t;
#define DECLARE_WAIT_QUEUE_HEAD(x) int dummy_##x __always_unused
#define might_sleep()		do {; } while (0)
#define init_waitqueue_head(x)	do {; } while (0)
#define wake_up(x)		do {; } while (0)
#define wait_event(x, c) \
do { \
	while (!(c)) { \
		bman_poll(); \
		qman_poll(); \
	} \
} while (0)
#define wait_event_interruptible(x, c) \
({ \
	wait_event(x, c); \
	0; \
})

/* I/O operations */
static inline u32 in_be32(volatile void *__p)
{
	volatile u32 *p = __p;
	return *p;
}
static inline void out_be32(volatile void *__p, u32 val)
{
	volatile u32 *p = __p;
	*p = val;
}
#define hwsync __sync_synchronize
#define dcbt_ro(p) __builtin_prefetch(p, 0)
#define dcbt_rw(p) __builtin_prefetch(p, 1)
#define lwsync() \
	do { \
		asm volatile ("lwsync" : : : "memory"); \
	} while (0)
#define dcbf(p) \
	do { \
		asm volatile ("dcbf 0,%0" : : "r" (p)); \
	} while (0)
#define dcbi(p) dcbf(p)
#ifdef CONFIG_PPC_E500MC
#define dcbzl(p) \
	do { \
		__asm__ __volatile__ ("dcbzl 0,%0" : : "r" (p)); \
	} while (0)
#define dcbz_64(p) \
	do { \
		dcbzl(p); \
	} while (0)
#define dcbf_64(p) \
	do { \
		dcbf(p); \
	} while (0)
/* Commonly used combo */
#define dcbit_ro(p) \
	do { \
		dcbi(p); \
		dcbt_ro(p); \
	} while (0)
#else
#define dcbz(p) \
	do { \
		__asm__ __volatile__ ("dcbz 0,%0" : : "r" (p)); \
	} while (0)
#define dcbz_64(p) \
	do { \
		dcbz((u32)p + 32);	\
		dcbz(p);	\
	} while (0)
#define dcbf_64(p) \
	do { \
		dcbf((u32)p + 32); \
		dcbf(p); \
	} while (0)
/* Commonly used combo */
#define dcbit_ro(p) \
	do { \
		dcbi(p); \
		dcbi((u32)p + 32); \
		dcbt_ro(p); \
		dcbt_ro((u32)p + 32); \
	} while (0)
#endif /* CONFIG_PPC_E500MC */
#define barrier() \
	do { \
		asm volatile ("" : : : "memory"); \
	} while(0)
#define cpu_relax barrier

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

#define BUG()	abort()
#ifdef CONFIG_BUGON
#ifdef pr_debug
#undef pr_debug
#endif
#define pr_debug(fmt, args...)	printf(fmt, ##args)
#define BUG_ON(c) \
do { \
	if (c) { \
		pr_crit("BUG: %s:%d\n", __FILE__, __LINE__); \
		abort(); \
	} \
} while(0)
#define might_sleep_if(c)	BUG_ON(c)
#define msleep(x) \
do { \
	pr_crit("BUG: illegal call %s:%d\n", __FILE__, __LINE__); \
	exit(EXIT_FAILURE); \
} while(0)
#else
#ifdef pr_debug
#undef pr_debug
#endif
#define pr_debug(fmt, args...)	do { ; } while(0)
#define BUG_ON(c)		do { ; } while(0)
#define might_sleep_if(c)	do { ; } while(0)
#define msleep(x)		do { ; } while(0)
#endif
#define WARN_ON(c, str) \
do { \
	static int warned_##__LINE__; \
	if ((c) && !warned_##__LINE__) { \
		pr_warning("%s\n", str); \
		pr_warning("(%s:%d)\n", __FILE__, __LINE__); \
		warned_##__LINE__ = 1; \
	} \
} while (0)

#define ALIGN(x, a) (((x) + ((typeof(x))(a) - 1)) & ~((typeof(x))(a) - 1))

/****************/
/* Linked-lists */
/****************/

struct list_head {
	struct list_head *prev;
	struct list_head *next;
};

#define LIST_HEAD(n) \
struct list_head n = { \
	.prev = &n, \
	.next = &n \
}
#define INIT_LIST_HEAD(p) \
do { \
	struct list_head *__p298 = (p); \
	__p298->prev = __p298->next =__p298; \
} while(0)
#define list_entry(node, type, member) \
	(type *)((void *)node - offsetof(type, member))
#define list_empty(p) \
({ \
	const struct list_head *__p298 = (p); \
	((__p298->next == __p298) && (__p298->prev == __p298)); \
})
#define list_add(p,l) \
do { \
	struct list_head *__p298 = (p); \
	struct list_head *__l298 = (l); \
	__p298->next = __l298->next; \
	__p298->prev = __l298; \
	__l298->next->prev = __p298; \
	__l298->next = __p298; \
} while(0)
#define list_add_tail(p,l) \
do { \
	struct list_head *__p298 = (p); \
	struct list_head *__l298 = (l); \
	__p298->prev = __l298->prev; \
	__p298->next = __l298; \
	__l298->prev->next = __p298; \
	__l298->prev = __p298; \
} while(0)
#define list_for_each(i, l)				\
	for (i = (l)->next; i != (l); i = i->next)
#define list_for_each_safe(i, j, l)			\
	for (i = (l)->next, j = i->next; i != (l);	\
	     i = j, j = i->next)
#define list_for_each_entry(i, l, name) \
	for (i = list_entry((l)->next, typeof(*i), name); &i->name != (l); \
		i = list_entry(i->name.next, typeof(*i), name))
#define list_for_each_entry_safe(i, j, l, name) \
	for (i = list_entry((l)->next, typeof(*i), name), \
		j = list_entry(i->name.next, typeof(*j), name); \
		&i->name != (l); \
		i = j, j = list_entry(j->name.next, typeof(*j), name))
#define list_del(i) \
do { \
	(i)->next->prev = (i)->prev; \
	(i)->prev->next = (i)->next; \
} while(0)

/* Other miscellaneous interfaces our APIs depend on; */

/* Qman/Bman API inlines and macros; */
#define lower_32_bits(x) ((u32)(x))
#define upper_32_bits(x) ((u32)(((x) >> 16) >> 16))

/* PPAC inlines require cpu_spin(); */
/* Alternate Time Base */
#define SPR_ATBL	526
#define SPR_ATBU	527
#define SPR_TBL		268
#define SPR_TBU		269
#define mfspr(reg) \
({ \
	register_t ret; \
	asm volatile("mfspr %0, %1" : "=r" (ret) : "i" (reg) : "memory"); \
	ret; \
})
static inline uint64_t mfatb(void)
{
	uint32_t hi, lo, chk;
	do {
		hi = mfspr(SPR_ATBU);
		lo = mfspr(SPR_ATBL);
		chk = mfspr(SPR_ATBU);
	} while (unlikely(hi != chk));
	return (uint64_t) hi << 32 | (uint64_t) lo;
}

static inline uint64_t mftb(void)
{
	uint32_t hi, lo, chk;
	do {
		hi = mfspr(SPR_TBU);
		lo = mfspr(SPR_TBL);
		chk = mfspr(SPR_TBU);
	} while (unlikely(hi != chk));
	return (uint64_t) hi << 32 | (uint64_t) lo;
}

/* Spin for a few cycles without bothering the bus */
static inline void cpu_spin(int cycles)
{
	uint64_t now = mfatb();
	while (mfatb() < (now + cycles))
		;
}

/* <usdpaa/compat.h> already includes system headers and definitions required
 * via the APIs, so these includes and definitions should only supply whatever
 * additions are required to compile the implementations. */
#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>
#include <malloc.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <limits.h>
#include <assert.h>
#include <dirent.h>
#include <inttypes.h>
#include <error.h>

/* NB: these compatibility shims are in this exported header because they're
 * required by interfaces shared with linux drivers (ie. for "single-source"
 * purposes).
 */

/* Compiler/type stuff */
typedef unsigned int	gfp_t;
typedef uint32_t	phandle;

#define noinline	__attribute__((noinline))
#define __iomem
#define EINTR		4
#define ENODEV		19
#define MODULE_AUTHOR(s)
#define MODULE_LICENSE(s)
#define MODULE_DESCRIPTION(s)
#define MODULE_PARM_DESC(x, y)
#define EXPORT_SYMBOL(x)
#define module_init(fn) int m_##fn(void) { return fn(); }
#define module_exit(fn) void m_##fn(void) { fn(); }
#define module_param(x, y, z)
#define module_param_string(w, x, y, z)
#define GFP_KERNEL	0
#define __KERNEL__
#define __init
#define __raw_readb(p)	*(const volatile unsigned char *)(p)
#define __raw_readl(p)	*(const volatile unsigned int *)(p)
#define __raw_writel(v, p) \
do { \
	*(volatile unsigned int *)(p) = (v); \
} while (0)

#if defined(__powerpc64__)
#define CONFIG_PPC64
#endif

/* printk() stuff */
#define printk(fmt, args...)	do_not_use_printk
#define nada(fmt, args...)	do { ; } while(0)

/* Debug stuff */
#ifdef CONFIG_FSL_BMAN_CHECKING
#define BM_ASSERT(x) \
	do { \
		if (!(x)) { \
			pr_crit("ASSERT: (%s:%d) %s\n", __FILE__, __LINE__, \
				__stringify_1(x)); \
			exit(EXIT_FAILURE); \
		} \
	} while(0)
#else
#define BM_ASSERT(x)		do { ; } while(0)
#endif
#ifdef CONFIG_FSL_QMAN_CHECKING
#define QM_ASSERT(x) \
	do { \
		if (!(x)) { \
			pr_crit("ASSERT: (%s:%d) %s\n", __FILE__, __LINE__, \
				__stringify_1(x)); \
			exit(EXIT_FAILURE); \
		} \
	} while(0)
#else
#define QM_ASSERT(x)		do { ; } while(0)
#endif
static inline void __hexdump(unsigned long start, unsigned long end,
			unsigned long p, size_t sz, const unsigned char *c)
{
	while (start < end) {
		unsigned int pos = 0;
		char buf[64];
		int nl = 0;
		pos += sprintf(buf + pos, "%08lx: ", start);
		do {
			if ((start < p) || (start >= (p + sz)))
				pos += sprintf(buf + pos, "..");
			else
				pos += sprintf(buf + pos, "%02x", *(c++));
			if (!(++start & 15)) {
				buf[pos++] = '\n';
				nl = 1;
			} else {
				nl = 0;
				if(!(start & 1))
					buf[pos++] = ' ';
				if(!(start & 3))
					buf[pos++] = ' ';
			}
		} while (start & 15);
		if (!nl)
			buf[pos++] = '\n';
		buf[pos] = '\0';
		pr_info("%s", buf);
	}
}
static inline void hexdump(const void *ptr, size_t sz)
{
	unsigned long p = (unsigned long)ptr;
	unsigned long start = p & ~(unsigned long)15;
	unsigned long end = (p + sz + 15) & ~(unsigned long)15;
	const unsigned char *c = ptr;
	__hexdump(start, end, p, sz, c);
}


/* Interrupt stuff */
typedef uint32_t	irqreturn_t;
#define IRQ_HANDLED	0
#ifdef CONFIG_FSL_DPA_IRQ_SAFETY
#error "Won't work"
#endif
#define local_irq_disable()	do { ; } while(0)
#define local_irq_enable()	do { ; } while(0)
#define local_irq_save(v)	do { ; } while(0)
#define local_irq_restore(v)	do { ; } while(0)
#define request_irq(irq, isr, args, devname, portal) \
	qbman_request_irq(irq, isr, args, devname, portal)
#define free_irq(irq, portal) \
	qbman_free_irq(irq, portal)
#define irq_can_set_affinity(x)	0
#define irq_set_affinity(x,y)	0

/* memcpy() stuff - when you know alignments in advance */
#ifdef CONFIG_TRY_BETTER_MEMCPY
static inline void copy_words(void *dest, const void *src, size_t sz)
{
	u32 *__dest = dest;
	const u32 *__src = src;
	size_t __sz = sz >> 2;
	BUG_ON((unsigned long)dest & 0x3);
	BUG_ON((unsigned long)src & 0x3);
	BUG_ON(sz & 0x3);
	while (__sz--)
		*(__dest++) = *(__src++);
}
static inline void copy_shorts(void *dest, const void *src, size_t sz)
{
	u16 *__dest = dest;
	const u16 *__src = src;
	size_t __sz = sz >> 1;
	BUG_ON((unsigned long)dest & 0x1);
	BUG_ON((unsigned long)src & 0x1);
	BUG_ON(sz & 0x1);
	while (__sz--)
		*(__dest++) = *(__src++);
}
static inline void copy_bytes(void *dest, const void *src, size_t sz)
{
	u8 *__dest = dest;
	const u8 *__src = src;
	while (sz--)
		*(__dest++) = *(__src++);
}
#else
#define copy_words memcpy
#define copy_shorts memcpy
#define copy_bytes memcpy
#endif

/* Spinlock stuff */
#define spinlock_t		pthread_mutex_t
#define __SPIN_LOCK_UNLOCKED(x)	PTHREAD_ADAPTIVE_MUTEX_INITIALIZER_NP
#define DEFINE_SPINLOCK(x)	spinlock_t x = __SPIN_LOCK_UNLOCKED(x)
#define spin_lock_init(x) \
	do { \
		__maybe_unused int __foo;	\
		pthread_mutexattr_t __foo_attr;	\
		__foo = pthread_mutexattr_init(&__foo_attr);	\
		BUG_ON(__foo);	\
		__foo = pthread_mutexattr_settype(&__foo_attr,	\
						  PTHREAD_MUTEX_ADAPTIVE_NP); \
		BUG_ON(__foo);	\
		__foo = pthread_mutex_init(x, &__foo_attr); \
		BUG_ON(__foo); \
	} while (0)
#define spin_lock(x) \
	do { \
		__maybe_unused int __foo = pthread_mutex_lock(x); \
		BUG_ON(__foo); \
	} while (0)
#define spin_unlock(x) \
	do { \
		__maybe_unused int __foo = pthread_mutex_unlock(x); \
		BUG_ON(__foo); \
	} while (0)
#define spin_lock_irq(x)	do {				\
					local_irq_disable();	\
					spin_lock(x);		\
				} while (0)
#define spin_unlock_irq(x)	do {				\
					spin_unlock(x);		\
					local_irq_enable();	\
				} while (0)
#define spin_lock_irqsave(x, f)	do { spin_lock_irq(x); } while (0)
#define spin_unlock_irqrestore(x, f) do { spin_unlock_irq(x); } while (0)

#define raw_spinlock_t				spinlock_t
#define raw_spin_lock_init(x)			spin_lock_init(x)
#define raw_spin_lock_irqsave(x, f)		spin_lock(x)
#define raw_spin_unlock_irqrestore(x, f)	spin_unlock(x)

/* Completion stuff */
#define DECLARE_COMPLETION(n) int n = 0;
#define complete(n) \
do { \
	*n = 1; \
} while(0)
#define wait_for_completion(n) \
do { \
	while (!*n) { \
		bman_poll(); \
		qman_poll(); \
	} \
	*n = 0; \
} while(0)

/* Platform device stuff */
struct platform_device { void *dev; };
static inline struct
platform_device *platform_device_alloc(const char *name __always_unused,
					int id __always_unused)
{
	struct platform_device *ret = malloc(sizeof(*ret));
	if (ret)
		ret->dev = NULL;
	return ret;
}
#define platform_device_add(pdev)	0
#define platform_device_del(pdev)	do { ; } while(0)
static inline void platform_device_put(struct platform_device *pdev)
{
	free(pdev);
}
struct resource {
	int unused;
};

/* Allocator stuff */
#define kmalloc(sz, t)	malloc(sz)
#define vmalloc(sz)	malloc(sz)
#define kfree(p)	do { if (p) free(p); } while (0)
static inline void *kzalloc(size_t sz, gfp_t __foo __always_unused)
{
	void *ptr = malloc(sz);
	if (ptr)
		memset(ptr, 0, sz);
	return ptr;
}
static inline unsigned long get_zeroed_page(gfp_t __foo __always_unused)
{
	void *p;
	if (posix_memalign(&p, 4096, 4096))
		return 0;
	memset(p, 0, 4096);
	return (unsigned long)p;
}
struct kmem_cache {
	size_t sz;
	size_t align;
};
#define SLAB_HWCACHE_ALIGN	0
static inline struct kmem_cache *kmem_cache_create(const char *n __always_unused,
		 size_t sz, size_t align, unsigned long flags __always_unused,
			void (*c)(void *) __always_unused)
{
	struct kmem_cache *ret = malloc(sizeof(*ret));
	if (ret) {
		ret->sz = sz;
		ret->align = align;
	}
	return ret;
}
static inline void kmem_cache_destroy(struct kmem_cache *c)
{
	free(c);
}
static inline void *kmem_cache_alloc(struct kmem_cache *c, gfp_t f __always_unused)
{
	void *p;
	if (posix_memalign(&p, c->align, c->sz))
		return NULL;
	return p;
}
static inline void kmem_cache_free(struct kmem_cache *c __always_unused, void *p)
{
	free(p);
}
static inline void *kmem_cache_zalloc(struct kmem_cache *c, gfp_t f)
{
	void *ret = kmem_cache_alloc(c, f);
	if (ret)
		memset(ret, 0, c->sz);
	return ret;
}

/* Bitfield stuff. */
#define BITS_PER_ULONG	(sizeof(unsigned long) << 3)
#define SHIFT_PER_ULONG	(((1 << 5) == BITS_PER_ULONG) ? 5 : 6)
#define BITS_MASK(idx)	((unsigned long)1 << ((idx) & (BITS_PER_ULONG - 1)))
#define BITS_IDX(idx)	((idx) >> SHIFT_PER_ULONG)
static inline unsigned long test_bits(unsigned long mask,
				volatile unsigned long *p)
{
	return *p & mask;
}
static inline int test_bit(int idx, volatile unsigned long *bits)
{
	return test_bits(BITS_MASK(idx), bits + BITS_IDX(idx));
}
static inline void set_bits(unsigned long mask, volatile unsigned long *p)
{
	*p |= mask;
}
static inline void set_bit(int idx, volatile unsigned long *bits)
{
	set_bits(BITS_MASK(idx), bits + BITS_IDX(idx));
}
static inline void clear_bits(unsigned long mask, volatile unsigned long *p)
{
	*p &= ~mask;
}
static inline void clear_bit(int idx, volatile unsigned long *bits)
{
	clear_bits(BITS_MASK(idx), bits + BITS_IDX(idx));
}
static inline unsigned long test_and_set_bits(unsigned long mask,
					volatile unsigned long *p)
{
	unsigned long ret = test_bits(mask, p);
	set_bits(mask, p);
	return ret;
}
static inline int test_and_set_bit(int idx, volatile unsigned long *bits)
{
	int ret = test_bit(idx, bits);
	set_bit(idx, bits);
	return ret;
}
static inline int test_and_clear_bit(int idx, volatile unsigned long *bits)
{
	int ret = test_bit(idx, bits);
	clear_bit(idx, bits);
	return ret;
}
static inline int find_next_zero_bit(unsigned long *bits, int limit, int idx)
{
	while ((++idx < limit) && test_bit(idx, bits))
		;
	return idx;
}
static inline int find_first_zero_bit(unsigned long *bits, int limit)
{
	int idx = 0;
	while (test_bit(idx, bits) && (++idx < limit))
		;
	return idx;
}

/************/
/* RB-trees */
/************/

/* Linux has a good RB-tree implementation, that we can't use (GPL). It also has
 * a flat/hooked-in interface that virtually requires license-contamination in
 * order to write a caller-compatible implementation. Instead, I've created an
 * RB-tree encapsulation on top of linux's primitives (it does some of the work
 * the client logic would normally do), and this gives us something we can
 * reimplement on LWE. Unfortunately there's no good+free RB-tree
 * implementations out there that are license-compatible and "flat" (ie. no
 * dynamic allocation). I did find a malloc-based one that I could convert, but
 * that will be a task for later on. For now, LWE's RB-tree is implemented using
 * an ordered linked-list.
 *
 * Note, the only linux-esque type is "struct rb_node", because it's used
 * statically in the exported header, so it can't be opaque. Our version doesn't
 * include a "rb_parent_color" field because we're doing linked-list instead of
 * a true rb-tree.
 */

#if 0 /* declared in <usdpaa/compat.h>, required by <usdpaa/fsl_qman.h> */
struct rb_node {
	struct rb_node *prev, *next;
};
#endif

struct dpa_rbtree {
	struct rb_node *head, *tail;
};

#define DPA_RBTREE { NULL, NULL }
static inline void dpa_rbtree_init(struct dpa_rbtree *tree)
{
	tree->head = tree->tail = NULL;
}

#define QMAN_NODE2OBJ(ptr, type, node_field) \
	(type *)((char *)ptr - offsetof(type, node_field))

#define IMPLEMENT_DPA_RBTREE(name, type, node_field, val_field) \
static inline int name##_push(struct dpa_rbtree *tree, type *obj) \
{ \
	struct rb_node *node = tree->head; \
	if (!node) { \
		tree->head = tree->tail = &obj->node_field; \
		obj->node_field.prev = obj->node_field.next = NULL; \
		return 0; \
	} \
	while (node) { \
		type *item = QMAN_NODE2OBJ(node, type, node_field); \
		if (obj->val_field == item->val_field) \
			return -EBUSY; \
		if (obj->val_field < item->val_field) { \
			if (tree->head == node) \
				tree->head = &obj->node_field; \
			else \
				node->prev->next = &obj->node_field; \
			obj->node_field.prev = node->prev; \
			obj->node_field.next = node; \
			node->prev = &obj->node_field; \
			return 0; \
		} \
		node = node->next; \
	} \
	obj->node_field.prev = tree->tail; \
	obj->node_field.next = NULL; \
	tree->tail->next = &obj->node_field; \
	tree->tail = &obj->node_field; \
	return 0; \
} \
static inline void name##_del(struct dpa_rbtree *tree, type *obj) \
{ \
	if (tree->head == &obj->node_field) { \
		if (tree->tail == &obj->node_field) \
			/* Only item in the list */ \
			tree->head = tree->tail = NULL; \
		else { \
			/* Is the head, next != NULL */ \
			tree->head = tree->head->next; \
			tree->head->prev = NULL; \
		} \
	} else { \
		if (tree->tail == &obj->node_field) { \
			/* Is the tail, prev != NULL */ \
			tree->tail = tree->tail->prev; \
			tree->tail->next = NULL; \
		} else { \
			/* Is neither the head nor the tail */ \
			obj->node_field.prev->next = obj->node_field.next; \
			obj->node_field.next->prev = obj->node_field.prev; \
		} \
	} \
} \
static inline type *name##_find(struct dpa_rbtree *tree, u32 val) \
{ \
	struct rb_node *node = tree->head; \
	while (node) { \
		type *item = QMAN_NODE2OBJ(node, type, node_field); \
		if (val == item->val_field) \
			return item; \
		if (val < item->val_field) \
			return NULL; \
		node = node->next; \
	} \
	return NULL; \
}

static inline u64 div64_u64(u64 n, u64 d)
{
	return n / d;
}

#endif /* HEADER_COMPAT_H */
