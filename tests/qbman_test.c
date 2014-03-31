/* Copyright (C) 2014 Freescale Semiconductor, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Freescale Semiconductor nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
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

#include <drivers/fsl_qbman_base.h>
#include <drivers/fsl_qbman_portal.h>
#include <compat.h>
#include <sys/ioctl.h>

#define QBMAN_TEST_MAGIC 'q'
struct qbman_test_swp_ioctl {
	unsigned long portal1_cinh;
	unsigned long portal1_cena;
};
struct qbman_test_dma_ioctl {
	unsigned long *ptr;
	uint64_t phys_addr;
};

#define QBMAN_TEST_SWP_MAP \
	_IOR(QBMAN_TEST_MAGIC, 0x01, struct qbman_test_swp_ioctl)
#define QBMAN_TEST_SWP_UNMAP \
        _IOR(QBMAN_TEST_MAGIC, 0x02, struct qbman_test_swp_ioctl)
#define QBMAN_TEST_DMA_MAP \
        _IOR(QBMAN_TEST_MAGIC, 0x03, struct qbman_test_dma_ioctl)
#define QBMAN_TEST_DMA_UNMAP \
        _IOR(QBMAN_TEST_MAGIC, 0x04, struct qbman_test_dma_ioctl)

static int filep = -1;
static struct qbman_test_swp_ioctl portal_map;
static struct qbman_test_dma_ioctl mem_map;

int qbman_swp_mmap(struct qbman_test_swp_ioctl *params)
{
	int ret;
	filep = open("/dev/qbman-test", O_RDWR);
	if (filep == -1)
		return -ENODEV;
		
        ret = ioctl(filep, QBMAN_TEST_SWP_MAP, params);
        if (ret) {
                perror("ioctl(QBMAN_TEST_SWP_MAP)");
                return ret;
        }
        return 0;
}

int qbman_swp_munmap(struct qbman_test_swp_ioctl *params)
{
	int ret;
	filep = open("/dev/qbman-test", O_RDWR);
	if (filep == -1)
		return -ENODEV;
		
        ret = ioctl(filep, QBMAN_TEST_SWP_UNMAP, params);
        if (ret) {
                perror("ioctl(QBMAN_TEST_SWP_UNMAP)");
                return ret;
        }
        return 0;
}

int qbman_dma_mmap(struct qbman_test_dma_ioctl *params)
{
	int ret;
	filep = open("/dev/qbman-test", O_RDWR);
	if (filep == -1)
		return -ENODEV;
		
        ret = ioctl(filep, QBMAN_TEST_DMA_MAP, params);
        if (ret) {
                perror("ioctl(QBMAN_TEST_DMA_UNMAP)");
                return ret;
        }
        return 0;
}

int qbman_dma_munmap(struct qbman_test_dma_ioctl *params)
{
	int ret;
	filep = open("/dev/qbman-test", O_RDWR);
	if (filep == -1)
		return -ENODEV;
		
        ret = ioctl(filep, QBMAN_TEST_DMA_UNMAP, params);
        if (ret) {
                perror("ioctl(QBMAN_TEST_DMA_UNMAP)");
                return ret;
        }
        return 0;
}

#define QBMAN_PORTAL_IDX 3
#define QBMAN_TEST_FQID 29
#define QBMAN_TEST_BPID 33
#define QBMAN_USE_QD
#ifdef QBMAN_USE_QD
#define QBMAN_TEST_QDID 5
#endif
#define QBMAN_TEST_LFQID 0xf00020

#define NUM_EQ_FRAME 10
#define NUM_DQ_FRAME 10
#define NUM_DQ_IN_DQRR 10
#define NUM_DQ_IN_MEM   (NUM_DQ_FRAME - NUM_DQ_IN_DQRR)

static struct qbman_swp *swp;
static struct qbman_eq_desc eqdesc;
static struct qbman_pull_desc pulldesc;
static struct qbman_release_desc releasedesc;

/* FQ ctx attribute values for the test code. */
#define FQCTX_HI 0xabbaf00d
#define FQCTX_LO 0x98765432
#define FQ_VFQID 0x12345

/* Sample frame descriptor */
static struct qbman_fd_simple fd = {
	.addr_lo = 0xbaba0000,
	.addr_hi = 0x00000123,
	.len = 0x7777,
	.frc = 0xdeadbeef,
	.flc_lo = 0x5a5a5a5a,
	.flc_hi = 0x6b6b6b6b
};

static void fd_inc(struct qbman_fd_simple *_fd)
{
	_fd->addr_lo += _fd->len;
	_fd->flc_lo += 0x100;
	_fd->frc += 0x10;
}

static int fd_cmp(struct qbman_fd *fda, struct qbman_fd *fdb)
{
	int i;
	for (i = 0; i < 8; i++)
		if (fda->words[i] - fdb->words[i])
			return 1;
	return 0;
}

struct qbman_fd fd_eq[NUM_EQ_FRAME];
struct qbman_fd fd_dq[NUM_DQ_FRAME];

/* "Buffers" to be released (and storage for buffers to be acquired) */
static uint64_t rbufs[] = { 0xf00dabba01234567ull, 0x9988776655443322ull };
static uint64_t abufs[2];

static void do_enqueue(struct qbman_swp *p)
{
	int i, j, ret;
	struct qbman_eq_response *eq_storage;
	dma_addr_t eq_storage_phys;

	eq_storage = (struct qbman_eq_response *)mem_map.ptr;
	eq_storage_phys = mem_map.phys_addr;

#ifdef QBMAN_USE_QD
	pr_info("*****QBMan_test: Enqueue %d frames to QD %d\n",
					NUM_EQ_FRAME, QBMAN_TEST_QDID);
#else
	pr_info("*****QBMan_test: Enqueue %d frames to FQ %d\n",
					NUM_EQ_FRAME, QBMAN_TEST_FQID);
#endif
	for (i = 0; i < NUM_EQ_FRAME; i++) {
		/*********************************/
		/* Prepare a enqueue descriptor */
		/*********************************/
		qbman_eq_desc_clear(&eqdesc);
		qbman_eq_desc_set_no_orp(&eqdesc, 1);
		qbman_eq_desc_set_response(&eqdesc, eq_storage_phys, 0);
		qbman_eq_desc_set_token(&eqdesc, 0x89);
#ifdef QBMAN_USE_QD
		/**********************************/
		/* Prepare a Queueing Destination */
		/**********************************/
		qbman_eq_desc_set_qd(&eqdesc, QBMAN_TEST_QDID, 0, 1);
#else
		qbman_eq_desc_set_fq(&eqdesc, QBMAN_TEST_FQID);
#endif

		/******************/
		/* Try an enqueue */
		/******************/
		ret = qbman_swp_enqueue(p, &eqdesc, (const struct qbman_fd *)&fd);
		BUG_ON(ret);
		for (j = 0; j < 8; j++)
			fd_eq[i].words[j] = *((uint32_t *)&fd + j);
		fd_inc(&fd);
	}
}

static void do_push_dequeue(struct qbman_swp *p)
{
	int i, j;
	const struct qbman_dq_entry *dq_storage1;
	const struct qbman_fd *__fd;

	pr_info("*****QBMan_test: Start push dequeue\n");
	for (i = 0; i < NUM_DQ_FRAME; i++) {
		dq_storage1 = qbman_swp_dqrr_next(p);
		if (dq_storage1) {
			__fd = qbman_dq_entry_DQ_fd(dq_storage1);
			for (j = 0; j < 8; j++)
				fd_dq[i].words[j] = __fd->words[j];
			if (fd_cmp(&fd_eq[i], &fd_dq[i]))
				pr_info("The dequeue FD %d in DQRR"
					" doesn't match enqueue FD\n", i);
			qbman_swp_dqrr_consume(p, dq_storage1);
		} else {
			pr_info("The push dequeue fails\n");
		}
	}
}

static void do_pull_dequeue(struct qbman_swp *p)
{
	int i, j, ret;
	struct qbman_dq_entry *dq_storage;
	dma_addr_t dq_storage_phys;
	const struct qbman_dq_entry *dq_storage1;
	const struct qbman_fd *__fd;

	pr_info("*****QBMan_test: Dequeue %d frames from FQ %d,"
					" write dq entry in DQRR\n",
					NUM_DQ_IN_DQRR, QBMAN_TEST_FQID);
	for (i = 0; i < NUM_DQ_IN_DQRR; i++) {
		qbman_pull_desc_clear(&pulldesc);
		qbman_pull_desc_set_storage(&pulldesc, NULL, 0, 0);
		qbman_pull_desc_set_numframes(&pulldesc, 1);
		qbman_pull_desc_set_token(&pulldesc, 0xab);
		qbman_pull_desc_set_fq(&pulldesc, QBMAN_TEST_FQID);

		ret = qbman_swp_pull(p, &pulldesc);
		BUG_ON(ret);
		dq_storage1 = qbman_swp_dqrr_next(p);
		if (dq_storage1) {
			__fd = qbman_dq_entry_DQ_fd(dq_storage1);
			for (j = 0; j < 8; j++)
				fd_dq[i].words[j] = __fd->words[j];
			if (fd_cmp(&fd_eq[i], &fd_dq[i]))
				pr_info("The dequeue FD %d in DQRR"
					" doesn't match enqueue FD\n", i);
			qbman_swp_dqrr_consume(p, dq_storage1);
		} else {
			pr_info("Dequeue with dq entry in DQRR fails\n");
		}
	}

	pr_info("*****QBMan_test: Dequeue %d frames from FQ %d,"
					" write dq entry in memory\n",
					NUM_DQ_IN_MEM, QBMAN_TEST_FQID);
	dq_storage = ((struct qbman_dq_entry *)mem_map.ptr[32]);
	for (i = 0; i < NUM_DQ_IN_MEM; i++) {
		qbman_dq_entry_set_oldtoken(&dq_storage[i], 1, i);
		dq_storage_phys = (dma_addr_t)(mem_map.phys_addr + 2048 +
					 i * sizeof(dq_storage_phys));
		qbman_pull_desc_clear(&pulldesc);
		qbman_pull_desc_set_storage(&pulldesc, &dq_storage[i],
						dq_storage_phys, 0);
		qbman_pull_desc_set_numframes(&pulldesc, 1);
		qbman_pull_desc_set_token(&pulldesc, 0xab + i);
		qbman_pull_desc_set_fq(&pulldesc, QBMAN_TEST_FQID);
		ret = qbman_swp_pull(p, &pulldesc);
		BUG_ON(ret);
		ret = qbman_dq_entry_has_newtoken(p, &dq_storage[i],
							0xab + i);
		if (ret) {
			for (j = 0; j < 8; j++)
				fd_dq[i + NUM_DQ_IN_DQRR].words[j] =
				dq_storage[i].dont_manipulate_directly[j + 8];
			j = i + NUM_DQ_IN_DQRR;
			if (fd_cmp(&fd_eq[j], &fd_dq[j]))
				pr_info("The dequeue FD %d in memory"
					" doesn't match enqueue FD\n", i);
		} else {
			pr_info("Dequeue with dq entry in memory fails\n");
		}
	}
}

static void release_buffer(struct qbman_swp *p)
{
	int ret;
	qbman_release_desc_clear(&releasedesc);
	qbman_release_desc_set_bpid(&releasedesc, QBMAN_TEST_BPID);
	pr_info("*****QBMan_test: Release buffer to BP %d\n",
					QBMAN_TEST_BPID);
	ret = qbman_swp_release(p, &releasedesc, &rbufs[0],
					ARRAY_SIZE(rbufs));
	BUG_ON(ret);
}

static void acquire_buffer(struct qbman_swp *p)
{
	int ret;
	pr_info("*****QBMan_test: Acquire buffer from BP %d\n",
					QBMAN_TEST_BPID);
	ret = qbman_swp_acquire(p, QBMAN_TEST_BPID, &abufs[0], 2);
	BUG_ON(ret != 2);
}


static void ceetm_test(struct qbman_swp *p)
{
	int i, j, ret;

	qbman_eq_desc_clear(&eqdesc);
	qbman_eq_desc_set_no_orp(&eqdesc, 0);
	qbman_eq_desc_set_fq(&eqdesc, QBMAN_TEST_LFQID);
	pr_info("*****QBMan_test: Enqueue to LFQID %x\n",
						QBMAN_TEST_LFQID);
	for (i = 0; i < NUM_EQ_FRAME; i++) {
		ret = qbman_swp_enqueue(p, &eqdesc,
					(const struct qbman_fd *)&fd);
		BUG_ON(ret);
		for (j = 0; j < 8; j++)
			fd_eq[i].words[j] = *((uint32_t *)&fd + j);
		fd_inc(&fd);
	}
}

int qbman_test(void)
{
	struct qbman_swp_desc pd;
	uint32_t reg;
	int ret;

	pd.idx = QBMAN_PORTAL_IDX;
	
	ret = qbman_swp_mmap(&portal_map);
	if (ret) {
		error(0, ret, "process_portal_map()");
		return ret;
	}

	pd.cena_bar = (void *)portal_map.portal1_cena;
	pd.cinh_bar = (void *)portal_map.portal1_cinh;
	pd.irq = -1;

	/* Detect whether the mc image is the test image with GPP setup */
	reg = __raw_readl(pd.cena_bar + 0x4);
	if (reg != 0xdeadbeef) {
		pr_err("The MC image doesn't have GPP test setup, stop testing\n");
		qbman_swp_munmap(&portal_map);
		return -1;
	}

	pr_info("*****QBMan_test: Init QBMan SWP %d\n", QBMAN_PORTAL_IDX);
	swp = qbman_swp_init(&pd);
	if (!swp) {
		qbman_swp_munmap(&portal_map);
		return -1;
	}

	ret = qbman_dma_mmap(&mem_map);
	if (ret) {
		error(0, ret, "process_dma_mmap()");
		return ret;
	}

	/*******************/
	/* Enqueue frames  */
	/*******************/
	do_enqueue(swp);

	/*******************/
	/* Do pull dequeue */
	/*******************/
	do_pull_dequeue(swp);

	/*******************/
	/* Enqueue frames  */
	/*******************/
	qbman_swp_push_set(swp, 1, 1);
	qbman_swp_fq_schedule(swp, QBMAN_TEST_FQID);
	do_enqueue(swp);

	/*******************/
	/* Do push dequeue */
	/*******************/
	do_push_dequeue(swp);

	/*****************/
	/* Try a release */
	/*****************/
	release_buffer(swp);

	/******************/
	/* Try an acquire */
	/******************/
	acquire_buffer(swp);

	/******************/
	/* CEETM test     */
	/******************/
	ceetm_test(swp);

	qbman_dma_munmap(&mem_map);
	qbman_swp_finish(swp);
	pr_info("*****QBMan_test: User space test Passed\n");
	return 0;
}

int main(int argc, char *argv[])
{
	fprintf(stderr, "Start QBMan test\n");
	qbman_test();
	return 0;
}
