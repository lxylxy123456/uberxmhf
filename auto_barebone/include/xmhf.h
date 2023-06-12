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

#define xmhf_cpu_relax cpu_relax
#define HALT cpu_halt

