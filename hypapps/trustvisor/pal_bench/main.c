#include <assert.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>

#include "vmcall.h"
#include "pal.h"
#include "caller.h"
#include "trustvisor.h"
#include "record.h"

void *g_entry;
bool g_record = false;
int g_cpu = -1;

uint64_t g_time_records[4][2] = {};

void add64(uint64_t *dst, uint64_t src)
{
	assert(((uint64_t)0 - 1) - src >= *dst);
	*dst += src;
}

void record_time(uint64_t ts, uint64_t te, int type)
{
	assert(te > ts);
	if (g_record) {
		assert(type < sizeof(g_time_records) / sizeof(g_time_records[0]));
		add64(&g_time_records[type][0], 1);
		add64(&g_time_records[type][1], te - ts);
	}
}

void end_record(void)
{
#define P(x, y) \
	do { \
		printf(x "\t%" PRId64 "\t%" PRId64 "\n", g_time_records[y][0], \
				g_time_records[y][1]); \
	} while (0)
	P("reg", RECORD_TYPE_REGISTER);
	P("mar", RECORD_TYPE_MARSHALL);
	P("unmar", RECORD_TYPE_UNMARSHALL);
	P("unreg", RECORD_TYPE_UNREGISTER);
#undef P
}

/* Register */
void pal_reg(void)
{
	struct tv_pal_params params = {
		num_params: 2,
		params: {
			{ TV_PAL_PM_INTEGER, 4 },
			{ TV_PAL_PM_POINTER, 4 }
		}
	};
	assert(g_entry == NULL);
	g_entry = register_pal(&params, my_pal, begin_pal_c, end_pal_c, 0);
}

/* Unregister */
void pal_unreg(void)
{
	assert(g_entry != NULL);
	unregister_pal(g_entry);
	g_entry = NULL;
}

/* Call */
void pal_call(void)
{
	uint64_t ts = 0, tm = 0, te = 0;
	uint32_t cs = 0, cm = 0, ce = 0;
	typeof(my_pal) *func = (typeof(my_pal) *)g_entry;
	uintptr_t b[4] = {};
	uintptr_t ret;
	assert(*(uint64_t *)b == 0);
	ts = rdtsc(&cs);
	ret = func(789012, b);
	te = rdtsc(&ce);
	tm = *(uint64_t *)b;
	cm = *(uint32_t *)&((uint64_t *)b)[1];
	assert(ret == 789012);
	assert(tm != 0);
	if (cs == cm) {
		record_time(ts, tm, RECORD_TYPE_MARSHALL);
	}
	if (cm == ce) {
		record_time(tm, te, RECORD_TYPE_UNMARSHALL);
	}
}

void alarm_handler(int signum)
{
	static uint64_t t_prev = 0;
	static uint32_t c_prev = -1;
	uint64_t t = 0;
	uint32_t c = 0;
	t = rdtsc(&c);
	if (c == c_prev) {
		record_time(t_prev, t, RECORD_TYPE_REGISTER);
	}
	t_prev = t;
	c_prev = c;
}

int main_measure_clock(int duration)
{
	struct itimerval timer = {
		.it_interval = { .tv_sec = 1, .tv_usec = 0 },
		.it_value = { .tv_sec = 1, .tv_usec = 0 },
	};
	alarm_handler(1);
	signal(SIGALRM, alarm_handler);
	assert(setitimer(ITIMER_REAL, &timer, NULL) == 0);
	for (int i = -2; i < duration; i++) {
		/* Alarm will let sleep only sleep for 1 second */
		sleep(2);
		if (i >= 0) {
			g_record = true;
		}
	}
	timer.it_value.tv_sec = 0;
	assert(setitimer(ITIMER_REAL, &timer, NULL) == 0);
	{
		printf("alarm\t%" PRId64 "\t%" PRId64 "\n", g_time_records[0][0],
				g_time_records[0][1]);
	}
	return 0;

/*
	struct timespec start;
	struct timespec end;
	long long int i;
	assert(test_duration > 0);
	while ()
	for (i = -128; i <= 0 || time_cmp(&start, &end) >= 0; i++) {
		if (i == 0) {
			memcpy(&start, &end, sizeof(end));
			start.tv_sec += test_duration;
		}
		f(arg);
		assert(clock_gettime(CLOCK_REALTIME, &end) == 0);
	}
	assert(i > 0);
	return 0;
*/
}

int main(int argc, char *argv[])
{
	int n1 = atoi(argv[1]);
	int n2 = atoi(argv[2]);

	if (n1 == 0) {
		main_measure_clock(n2);
		return 0;
	}

	assert(n1 > 0);
	assert(n2 > 0);

#if 0
	/* Fix CPU */
	// #define _GNU_SOURCE
	// #include <sched.h>
	{
		g_cpu = sched_getcpu();
		/* For all machines we have, there are exactly 4 CPUs */
		assert(g_cpu >= 0 && g_cpu < 4);
		cpu_set_t mask;
		CPU_ZERO(&mask);
		CPU_SET(g_cpu, &mask);
		assert(sched_setaffinity(0, sizeof(mask), &mask) == 0);
	}
#endif

	/* Pre-heat cache */
	for (int i = 0; i < 5; i++) {
		pal_reg();
		for (int j = 0; j < 5; j++) {
			pal_call();
		}
		pal_unreg();
	}

	/* Measure */
	g_record = true;
	for (int i = 0; i < n1; i++) {
		pal_reg();
		for (int j = 0; j < n2; j++) {
			pal_call();
		}
		pal_unreg();
	}

	/* Check CPU */
	//assert(g_cpu == sched_getcpu());

	/* Print result */
	end_record();
}
