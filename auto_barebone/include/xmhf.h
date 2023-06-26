#ifndef _XMHF_H_
#define _XMHF_H_

#ifndef __ASSEMBLY__

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stddef.h>

#include <shv_types.h>

#include <_msr.h>
#include <hptw.h>

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

// TODO: change its name
#define HALT_ON_ERRORCOND(expr) \
	do { \
		if (!expr) { \
			printf("Error: HALT_ON_ERRORCOND(" #expr ") @ %s %d failed\n", \
				   __FILE__, __LINE__); \
			cpu_halt(); \
		} \
	} while (0)

#define MAX_VCPU_ENTRIES 10

// TODO: 64-bit
struct regs {
	u32 eax;
	u32 ecx;
	u32 edx;
	u32 ebx;
	u32 esp;
	u32 ebp;
	u32 esi;
	u32 edi;
};

typedef struct {
	u32 id;
	u32 idx;
	bool isbsp;
	// TODO
} VCPU;

#endif	/* !__ASSEMBLY__ */

#endif	/* _XMHF_H_ */
