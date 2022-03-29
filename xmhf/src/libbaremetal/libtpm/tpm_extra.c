// cd _build_libbaremetal/_objects
// gcc -O3 -Wall -Werror   -c -o tpm_extra.o ../../xmhf/src/libbaremetal/libtpm/tpm_extra.c


#include <stdint.h>

typedef struct {
    uint32_t         enc_data_size;
} tpm_short_t;

typedef struct {
    uint16_t         tag;
} tpm_pcr_t;

typedef struct {
    tpm_pcr_t        seal_info;
    uint16_t         tag2;
} tpm_t;


/* These go with _tpm_submit_cmd in tpm.c */
extern uint8_t     rsp_buf[100];

void _tpm_seal(uint8_t *sealed_data)
{
	{
	   const uint8_t *p1 = rsp_buf;
	   if ( (*(uint32_t *)(sealed_data)) == 12 ) {
			uint8_t *p2 = (uint8_t *)&(
				( (tpm_short_t *)sealed_data )->enc_data_size
			);
			for (uint32_t i = 0; i < 4; i++ )
				p2[i] = p1[0];
	   } else {
			tpm_t *a = (tpm_t *)sealed_data;
			uint8_t *p3 = (uint8_t *)&( ( (a->seal_info) ) .tag );
			for (uint32_t i = 0; i < 2; i++ )
				p3[i] = p1[0];
	   }
	}

}

