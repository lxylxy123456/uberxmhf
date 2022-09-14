#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define HALT_ON_ERRORCOND assert
#define MAX_VCPU_ENTRIES 10

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef struct _vcpu {
	u32 id;
	u32 idx;
} VCPU;

