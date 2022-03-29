// gcc (GCC) 11.2.1 20220127 (Red Hat 11.2.1-9)
// Fedora release 35 (Thirty Five)
// 5.16.16-200.fc35.x86_64

// gcc -O3 -Wall -Werror -c -o a.o a.c

#include <stdint.h>

typedef struct {
	uint16_t field_a;
} type_a_t;

typedef struct {
	uint32_t field_b;
} type_b_t;

typedef struct {
	type_a_t field_c;
} type_c_t;

extern uint8_t buf[100];

void _tpm_seal(uint8_t * ptr)
{
	const uint8_t *p1 = buf;
	if ((*(uint32_t *) (ptr)) == 12) {
		uint8_t *p2 = (uint8_t *) & (((type_b_t *) ptr)->field_b);
		for (uint32_t i = 0; i < 4; i++) {
			p2[i] = p1[0];
		}
	} else {
		type_c_t *a = (type_c_t *) ptr;
		uint8_t *p3 = (uint8_t *) & (((a->field_c)).field_a);
		for (uint32_t i = 0; i < 2; i++) {
			p3[i] = p1[0];
		}
	}
}
