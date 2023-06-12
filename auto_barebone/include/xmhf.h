#include <stdio.h>
#include <stdint.h>
#include <string.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

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

#define HALT() \
	do { \
		asm volatile("hlt"); \
	} while(1)

static inline void cpu_relax(void)
{
	asm volatile("pause");
}

static inline u8 inb(u16 port)
{
	u8 ans;
	asm volatile("inb %1, %0" : "=a"(ans) : "d"(port));
	return ans;
}

static inline void outb(u16 port, u8 val)
{
	asm volatile("outb %1, %0" : : "d"(port), "a"(val));
}

