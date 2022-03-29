
#include <stddef.h>
#include <tpm.h>
#include <sha1.h>

/* These go with _tpm_submit_cmd in tpm.c */
extern uint8_t     cmd_buf[TPM_CMD_SIZE_MAX];
extern uint8_t     rsp_buf[TPM_RSP_SIZE_MAX];

void _tpm_seal(uint8_t *sealed_data)
{
//    LOAD_STORED_DATA12(WRAPPER_OUT_BUF, offset, sealed_data);

	{
	   const uint8_t *p1 = (const uint8_t *)(WRAPPER_OUT_BUF);
	   if ( ((tpm_stored_data12_header_t *)(sealed_data))->seal_info_size == 0 ) {
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

