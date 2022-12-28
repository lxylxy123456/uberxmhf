#include <assert.h>
#include <stdio.h>

#include "vmcall.h"
#include "pal.h"
#include "caller.h"
#include "trustvisor.h"

#define PAGE_SIZE ((uintptr_t) 4096)

void call_pal(void) {
	// Construct struct tv_pal_params
	struct tv_pal_params params = {
		num_params: 2,
		params: {
			{ TV_PAL_PM_INTEGER, 4 },
			{ TV_PAL_PM_POINTER, 4 }
		}
	};
	// Register scode
	void *data = NULL;
	void *entry1 = register_pal(&params, my_pal, begin_pal_c, end_pal_c, 1, &data);
	void *entry2 = register_pal(&params, my_pal, begin_pal_c, end_pal_c, 1, &data);
	typeof(my_pal) *func1 = (typeof(my_pal) *)entry1;
	typeof(my_pal) *func2 = (typeof(my_pal) *)entry2;
	// Call function
	{
		uintptr_t ans1 = func1(0x178e324a, (uintptr_t *)&data);
		printf(" ans1 = %p\n", (void *)ans1);
		fflush(stdout);
	}
	{
		uintptr_t ans2 = func2(0, (uintptr_t *)&data);
		printf(" ans2 = %p\n", (void *)ans2);
		fflush(stdout);
	}
	// Unregister scode
	unregister_pal(entry1);
	unregister_pal(entry2);
}

int main(int argc, char *argv[]) {
	call_pal();
	return 0;
}
