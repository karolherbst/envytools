/*
 * Copyright (C) 2011 Marcin Kościelnicki <koriakin@0x04.net>
 * Copyright (C) 2016 Roy Spliet <rs855@cam.ac.uk>
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include "nva.h"
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <sched.h>
#include <inttypes.h>

int cnum = 0;
int32_t a;

static const int SZ = 1024 * 1024;

uint32_t queue[1024 * 1024];
uint64_t tqueue64[1024 * 1024];
volatile int get = 0, put = 0;

#define NV04_PTIMER_TIME_0                                 0x00009400
#define NV04_PTIMER_TIME_1                                 0x00009410

#define NVE0_HUB_SCRATCH_7                                 0x0040981c

struct nva_interval {
	uint64_t start;
	uint64_t diff;
};

uint64_t get_time(unsigned int card)
{
	uint64_t low;

	uint64_t high2 = nva_rd32(card, NV04_PTIMER_TIME_1);
	uint64_t high1;
	do {
		high1 = high2;
		low = nva_rd32(card, NV04_PTIMER_TIME_0);
		high2 = nva_rd32(card, NV04_PTIMER_TIME_1);
	} while (high1 != high2);
	return ((((uint64_t)high2) << 32) | (uint64_t)low);
}

void *t64watchfun(void *x) {
	uint32_t val = nva_rd32(cnum, a);
	queue[put] = val;
	tqueue64[put] = get_time(cnum);
	put = (put + 1) % SZ;
	while (1) {
		uint32_t nval = nva_rd32(cnum, a);
		if ((nval) != (val)) {
			queue[put] = nval;
			tqueue64[put] = get_time(cnum);
			put = (put + 1) % SZ;
		}
		val = nval;
	}

	return NULL;
}

void state_time(uint32_t val, uint32_t mask, uint64_t time,
		struct nva_interval *intv) {

	if (!intv->start && (val & mask) == mask) {
		intv->start = time;
	} else if(intv->start && (val & mask) == 0) {
		intv->diff = time - intv->start;
		intv->start = 0ull;
	}
}

#define RESET_INTV(s)	do {\
							(s).start = 0ull; \
							(s).diff = 0ull; \
						} while (0)

void state(uint32_t val, uint64_t time)
{
	static enum {
		STATE_INV,
		STATE_IDLE,
		STATE_RUN
	} state = STATE_INV;
	static uint64_t auto_start = 0ull;
	static struct nva_interval save = {0ull, 0ull};
	static struct nva_interval load = {0ull, 0ull};
	static struct nva_interval save_mmctx = {0ull, 0ull};
	static struct nva_interval load_mmctx = {0ull, 0ull};

	uint64_t diff;

	switch (state) {
	case STATE_INV:
		if (val == 0)
			state = STATE_IDLE;
		break;

	case STATE_IDLE:
		if (!(val & 0x00000010))
			break;

		auto_start = time;
		state = STATE_RUN;
		/* no break */
	case STATE_RUN:
		state_time(val, 0x00000080, time, &save);
		state_time(val, 0x00000040, time, &load);
		state_time(val, 0x00000082, time, &save_mmctx);
		state_time(val, 0x00000042, time, &load_mmctx);

		if (!(val & 0x00000010)) {
			/* print results */
			diff = time - auto_start;
			printf("%016"PRIu64": %"PRIu64",%"PRIu64"(%"PRIu64"),%"PRIu64
					"(%"PRIu64")\n",
					time, diff, save.diff, save_mmctx.diff, load.diff,
					load_mmctx.diff);

			auto_start = 0ull;
			RESET_INTV(save);
			RESET_INTV(load);
			RESET_INTV(save_mmctx);
			RESET_INTV(load_mmctx);

			state = STATE_IDLE;
		}
		break;
	default:
		break;
	}
}

uint32_t ctxsize_strands(uint32_t reg_base)
{
	uint32_t strand_count, strand_size = 0, io_idx, i;

	strand_count = nva_rd32(cnum, reg_base + 0x2880);
	io_idx = nva_rd32(cnum, reg_base + 0x2ffc);

	for (i = 0; i < strand_count; i++) {
		nva_wr32(cnum, reg_base + 0x2ffc, i);
		strand_size += (nva_rd32(cnum, reg_base + 0x2910) << 2);
	}
	nva_wr32(cnum, reg_base + 0x2ffc, io_idx);

	return strand_size;
}

void print_header()
{
	uint32_t dev, gpc_cnt, gpc_size, gpc_area, smx_cnt, ctx_size, i;

	dev = nva_rd32(cnum, 0x000000);
	dev &= 0x1ff00000;
	dev >>= 20;
	printf("Card: NV%X\n", dev);

	smx_cnt = (nva_rd32(cnum, 0x418bb8) >> 8) & 0xff;
	printf("SMX count          : %d\n", smx_cnt);

	gpc_cnt = nva_rd32(cnum, 0x22430);
	printf("GPC count          : %d\n", gpc_cnt);

	gpc_size = 0;
	gpc_area = 0;
	for (i = 0; i < gpc_cnt; i++) {
		ctx_size = nva_rd32(cnum, 0x50274c + (i * 0x8000)) << 2;
		ctx_size += ctxsize_strands(0x500000 + (i * 0x8000));
		printf("  GPC[%2u] ctx size : %d bytes\n", i, ctx_size);
		gpc_size += ctx_size;

		ctx_size = nva_rd32(cnum, 0x502804 + (i * 0x8000));
		printf("  GPC[%2u] ctx area : %d bytes\n", i, ctx_size);
		gpc_area += ctx_size;
	}

	/* XXX: Maxwell TPC strand context data */
	printf("GPC context size   : %d bytes\n", gpc_size);

	ctx_size = nva_rd32(cnum, 0x41a804);
	printf("GPC context area   : %d bytes\n", gpc_area);
	printf("\n");

	ctx_size = nva_rd32(cnum,0x40974c) << 2;
	ctx_size += ctxsize_strands(0x407000);
	printf("HUB context size   : %d bytes\n", ctx_size);

	printf("Total context size : %d bytes\n", ctx_size + gpc_size);

	ctx_size = nva_rd32(cnum, 0x409804);
	printf("Total context area : %d bytes\n", ctx_size);

	printf("\n");

	printf("time            : total, save (mmctx), load (mmctx)\n");
}

int main(int argc, char **argv) {
	if (nva_init()) {
		fprintf (stderr, "PCI init failure!\n");
		return 1;
	}
	int c;
	while ((c = getopt (argc, argv, "c:")) != -1)
		switch (c) {
			case 'c':
				sscanf(optarg, "%d", &cnum);
				break;
		}
	if (cnum >= nva_cardsnum) {
		if (nva_cardsnum)
			fprintf (stderr, "No such card.\n");
		else
			fprintf (stderr, "No cards found.\n");
		return 1;
	}

	a = NVE0_HUB_SCRATCH_7;

	print_header();

	pthread_t thr;
	pthread_create(&thr, 0, t64watchfun, 0);

	while (1) {
		while (get == put)
			sched_yield();

		state(queue[get],tqueue64[get]);

		get = (get + 1) % SZ;
	}
}
