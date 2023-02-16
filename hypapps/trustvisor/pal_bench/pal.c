#include "pal.h"
#include "vmcall.h"

void begin_pal_c() {}

uintptr_t my_pal(uintptr_t arg1, uintptr_t *arg2) {
	return arg1 + ((*arg2)++);
}

void end_pal_c() {}
