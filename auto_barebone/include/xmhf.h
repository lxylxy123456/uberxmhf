#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stddef.h>

#include <shv_types.h>

#include <cpu.h>

typedef uint32_t spin_lock_t;

// TODO: move to a non-static function
static inline void spin_lock(spin_lock_t *lock)
{
	// TODO
}

// TODO: move to a non-static function
static inline void spin_unlock(spin_lock_t *lock)
{
	// TODO
}

extern void *emhfc_putchar_arg;
extern spin_lock_t *emhfc_putchar_linelock_arg;
void emhfc_putchar(int c, void *arg);
extern void emhfc_putchar_linelock(spin_lock_t *arg);
extern void emhfc_putchar_lineunlock(spin_lock_t *arg);

#define xmhf_cpu_relax cpu_relax
#define HALT cpu_halt

