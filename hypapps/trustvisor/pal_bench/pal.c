#include "pal.h"
#include "vmcall.h"

void begin_pal_c() {}

uintptr_t my_pal(uintptr_t arg1, uintptr_t *arg2) {
	*((uint64_t *)arg2) = rdtsc((uint32_t *)&((uint64_t *)arg2)[1]);
	return arg1;
}

void end_pal_c() {}
