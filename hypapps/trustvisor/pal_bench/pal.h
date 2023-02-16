#include <stdint.h>

#define PAL_FLAG_MASK ((uintptr_t)0x80000000U)

extern void begin_pal_c();
extern uintptr_t my_pal(uintptr_t arg1, uintptr_t *arg2);
extern void end_pal_c();
