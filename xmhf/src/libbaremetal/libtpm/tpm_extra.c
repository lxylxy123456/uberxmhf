
#include <stddef.h>
#include <stdint.h>
#include <sha1.h>

typedef uint16_t tpm_structure_tag_t;
typedef uint16_t tpm_entity_type_t;

typedef struct __attribute__ ((packed)) {
    tpm_structure_tag_t         tag;
    tpm_entity_type_t           et;
    uint32_t                    seal_info_size;
} tpm_stored_data12_header_t;

typedef struct __attribute__ ((packed)) {
    tpm_stored_data12_header_t  header;
    uint32_t                    enc_data_size;
    uint8_t                     enc_data[];
} tpm_stored_data12_short_t;

typedef uint8_t tpm_locality_selection_t;

typedef struct __attribute__ ((packed)) {
    uint16_t    size_of_select;
    uint8_t     pcr_select[3];
} tpm_pcr_selection_t;

#define TPM_DIGEST_SIZE          20
typedef struct __attribute__ ((packed)) {
    uint8_t     digest[TPM_DIGEST_SIZE];
} tpm_digest_t;

typedef tpm_digest_t tpm_composite_hash_t;

typedef struct __attribute__ ((packed)) {
    tpm_structure_tag_t         tag;
    tpm_locality_selection_t    locality_at_creation;
    tpm_locality_selection_t    locality_at_release;
    tpm_pcr_selection_t         creation_pcr_selection;
    tpm_pcr_selection_t         release_pcr_selection;
    tpm_composite_hash_t        digest_at_creation;
    tpm_composite_hash_t        digest_at_release;
} tpm_pcr_info_long_t;

typedef struct __attribute__ ((packed)) {
    tpm_stored_data12_header_t  header;
    tpm_pcr_info_long_t         seal_info;
    uint32_t                    enc_data_size;
    uint8_t                     enc_data[];
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

