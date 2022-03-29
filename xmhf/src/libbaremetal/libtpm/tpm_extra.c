
#include <stddef.h>
#include <stdint.h>
#include <sha1.h>

typedef struct __attribute__ ((packed)) {
    uint32_t                    seal_info_size;
} tpm_stored_data12_header_t;

typedef struct __attribute__ ((packed)) {
    uint32_t                    enc_data_size;
} tpm_stored_data12_short_t;

typedef struct __attribute__ ((packed)) {
    uint16_t         tag;
} tpm_pcr_info_long_t;

typedef struct __attribute__ ((packed)) {
    tpm_pcr_info_long_t         seal_info;
} tpm_stored_data12_t;

#define RSP_HEAD_SIZE           10
#define WRAPPER_OUT_BUF         (rsp_buf + RSP_HEAD_SIZE)

#define TPM_CMD_SIZE_MAX        768
#define TPM_RSP_SIZE_MAX        768

/* These go with _tpm_submit_cmd in tpm.c */
extern uint8_t     cmd_buf[TPM_CMD_SIZE_MAX];
extern uint8_t     rsp_buf[TPM_RSP_SIZE_MAX];

void _tpm_seal(uint8_t *sealed_data)
{
//    LOAD_STORED_DATA12(WRAPPER_OUT_BUF, offset, sealed_data);

	{
	   const uint8_t *p1 = (const uint8_t *)(WRAPPER_OUT_BUF);
	   if ( ((tpm_stored_data12_header_t *)(sealed_data))->seal_info_size == 12 ) {
//	       LOAD_INTEGER(WRAPPER_OUT_BUF, offset,
//	                    ((tpm_stored_data12_short_t *)sealed_data)->enc_data_size);
			uint8_t *p2 = (uint8_t *)&(((tpm_stored_data12_short_t *)sealed_data)->enc_data_size);
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
//	       LOAD_PCR_INFO_LONG(WRAPPER_OUT_BUF, offset,
//	                          &((tpm_stored_data12_t *)sealed_data)->seal_info);

//			LOAD_INTEGER(WRAPPER_OUT_BUF, offset,
//						(&((tpm_stored_data12_t *)sealed_data)->seal_info)->tag);
			uint8_t *p3 = (uint8_t *)&((&((tpm_stored_data12_t *)sealed_data)->seal_info)->tag);
			// p3[0] = p1[0];
			// p3[1] = p1[1];
			{
				for (uint32_t i = 0; i < 2; i++ )
					p3[i] = p1[0];
			}
	   }
	}

}

