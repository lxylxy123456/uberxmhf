// cd _build_libbaremetal/_objects
// gcc -O3 -Wall -Werror -c -o tpm_extra.o ../../xmhf/src/libbaremetal/libtpm/tpm_extra.c

#include <stdint.h>

typedef struct {
	uint32_t field_a;
} type_a_t;

typedef struct {
	uint16_t field_b;
} type_b_t;

typedef struct {
	type_b_t field_c;
} type_c_t;

extern uint8_t buf[100];

void _tpm_seal(uint8_t *ptr)
{
	const uint8_t *p1 = buf;
	if ( (*(uint32_t *)(ptr)) == 12 ) {
		uint8_t *p2 = (uint8_t *)&( ( (type_a_t *)ptr )->field_a );
		for (uint32_t i = 0; i < 4; i++) {
			p2[i] = p1[0];
		}
	} else {
		type_c_t *a = (type_c_t *)ptr;
		uint8_t *p3 = (uint8_t *)&( ( (a->field_c) ) .field_b );
		for (uint32_t i = 0; i < 2; i++) {
			p3[i] = p1[0];
		}
	}
}

