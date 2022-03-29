
#include <stdint.h>

typedef struct __attribute__ ((packed)) {
    uint32_t                    enc_data_size;
} tpm_short_t;

typedef struct __attribute__ ((packed)) {
    uint16_t         tag;
} tpm_pcr_info_long_t;

typedef struct __attribute__ ((packed)) {
    tpm_pcr_info_long_t         seal_info;
} tpm_t;


/* These go with _tpm_submit_cmd in tpm.c */
extern uint8_t     rsp_buf[100];

void _tpm_seal(uint8_t *sealed_data)
{
	{
	   const uint8_t *p1 = rsp_buf;
	   if ( (*(uint32_t *)(sealed_data)) == 12 ) {
			uint8_t *p2 = (uint8_t *)&(((tpm_short_t *)sealed_data)->enc_data_size);
			// p2[0] = p1[0];
			// p2[1] = p1[1];
			// p2[2] = p1[2];
			// p2[3] = p1[3];
			{
				for (uint32_t i = 0; i < 4; i++ )
					p2[i] = p1[0];
			}
	   }
	   else {
			uint8_t *p3 = (uint8_t *)&((&((tpm_t *)sealed_data)->seal_info)->tag);
			// p3[0] = p1[0];
			// p3[1] = p1[1];
			{
				for (uint32_t i = 0; i < 2; i++ )
					p3[i] = p1[0];
			}
	   }
	}

}

