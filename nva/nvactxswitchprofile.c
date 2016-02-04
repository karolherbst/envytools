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

void state_auto(uint32_t val, uint64_t time)
{
	static uint64_t start = 0ull;

	uint64_t diff;

	if (!start && (val & 0x00000010))
		start = time;

	if (start && (val & 0x00000010) == 0) {
		diff = time - start;
		start = 0ull;
		printf("AUTO: %016"PRIu64"\n", diff);
	}
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

	pthread_t thr;
	pthread_create(&thr, 0, t64watchfun, 0);

	uint64_t ptime64 = 0;
	while (1) {
		while (get == put)
			sched_yield();

		//printf("%016"PRIu64"[+%"PRIu64"]: %08x\n", tqueue64[get], tqueue64[get]-ptime64, queue[get]);
		state_auto(queue[get],tqueue64[get]);
		ptime64 = tqueue64[get];

		get = (get + 1) % SZ;
	}
}
