#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#include "vmcall.h"
#include "pal.h"
#include "caller.h"
#include "trustvisor.h"

extern void *g_data;

void call_pal(uintptr_t a, uintptr_t b) {
	// Construct struct tv_pal_params
	struct tv_pal_params params = {
		num_params: 2,
		params: {
			{ TV_PAL_PM_INTEGER, 4 },
			{ TV_PAL_PM_POINTER, 4 }
		}
	};
	// Register scode
	void *entry = register_pal(&params, my_pal, begin_pal_c, end_pal_c, 1);
	typeof(my_pal) *func = (typeof(my_pal) *)entry;
	// Call function
	printf("With PAL:\n");
	printf(" a = %p\n", (void *)a);
	printf(" g_data = %p\n", g_data);
	assert(g_data != NULL);
	fflush(stdout);
	uintptr_t ret = func(a | PAL_FLAG_MASK, (uintptr_t *)&g_data);
	printf(" %p = my_pal(%p, %p)\n", (void *)ret, (void *)a, g_data);
	printf("sleeping ");
	fflush(stdout);
	while (1) {
		printf(".");
		fflush(stdout);
		sleep(1);
	}
	// Unregister scode
	//unregister_pal(entry);
}

int main(int argc, char *argv[]) {
	uintptr_t a = 0, b = 0;
	srand(time(0));
	a = ((unsigned long long)rand() >> 32) | (unsigned long long)rand();
//	if (!check_cpuid()) {
//		printf("Error: TrustVisor not present according to CPUID\n");
//		return 1;
//	}
	call_pal(a, b);
	return 0;
}
