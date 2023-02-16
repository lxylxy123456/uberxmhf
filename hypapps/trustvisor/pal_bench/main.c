#include <assert.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>

#include "vmcall.h"
#include "pal.h"
#include "caller.h"
#include "trustvisor.h"

time_t test_duration = 0;

static int time_cmp(struct timespec *lhs, struct timespec *rhs)
{
	if (lhs->tv_sec < rhs->tv_sec) {
		return -1;
	} else if (lhs->tv_sec > rhs->tv_sec) {
		return 1;
	} else if (lhs->tv_nsec < rhs->tv_nsec) {
		return -1;
	} else if (lhs->tv_nsec > rhs->tv_nsec) {
		return 1;
	} else {
		return 0;
	}
}

unsigned long long run_const_time(void (*f)(void *), void *arg)
{
	struct timespec start;
	struct timespec end;
	long long int i;
	assert(test_duration > 0);
	for (i = -128; i <= 0 || time_cmp(&start, &end) >= 0; i++) {
		if (i == 0) {
			memcpy(&start, &end, sizeof(end));
			start.tv_sec += test_duration;
		}
		f(arg);
		assert(clock_gettime(CLOCK_REALTIME, &end) == 0);
	}
	assert(i > 0);
	return (unsigned long long)i;
}

/* Register and unregister */
void f_ru(void *arg)
{
	struct tv_pal_params params = {
		num_params: 2,
		params: {
			{ TV_PAL_PM_INTEGER, 4 },
			{ TV_PAL_PM_POINTER, 4 }
		}
	};
	void *entry = register_pal(&params, my_pal, begin_pal_c, end_pal_c, 0);
	unregister_pal(entry);
}

/* Register, call unregister */
void f_rcu(void *arg)
{
	struct tv_pal_params params = {
		num_params: 2,
		params: {
			{ TV_PAL_PM_INTEGER, 4 },
			{ TV_PAL_PM_POINTER, 4 }
		}
	};
	void *entry = register_pal(&params, my_pal, begin_pal_c, end_pal_c, 0);
	typeof(my_pal) *func = (typeof(my_pal) *)entry;
	{
		uintptr_t b = 123456;
		uintptr_t ret = func(789012, &b);
		assert(ret == 912468);
	}
	unregister_pal(entry);
}

/* Call */
void *f_c_init(void)
{
	struct tv_pal_params params = {
		num_params: 2,
		params: {
			{ TV_PAL_PM_INTEGER, 4 },
			{ TV_PAL_PM_POINTER, 4 }
		}
	};
	void *entry = register_pal(&params, my_pal, begin_pal_c, end_pal_c, 0);
	return entry;
}

void f_c(void *arg)
{
	typeof(my_pal) *func = arg;
	{
		uintptr_t b = 123456;
		uintptr_t ret = func(789012, &b);
		assert(ret == 912468);
	}
}

void f_c_fini(void *arg)
{
	unregister_pal(arg);
}

void run_test(char c)
{
	switch (c) {
	case '0':
		printf("f_ru:  %llu\n", run_const_time(f_ru, NULL));
		break;
	case '1':
		printf("f_rcu: %llu\n", run_const_time(f_rcu, NULL));
		break;
	case '2':
		{
			void *arg = f_c_init();
			printf("f_c:   %llu\n", run_const_time(f_c, arg));
			f_c_fini(arg);
		}
		break;
	default:
		printf("unknown test: %c\n", c);
		break;
	}
}

int main(int argc, char *argv[])
{
	assert(argc > 2);
	{
		int i = atoi(argv[2]);
		assert(i > 0);
		test_duration = i;
	}
	run_test(argv[1][0]);
	return 0;
}
