/*
 * @XMHF_LICENSE_HEADER_START@
 *
 * eXtensible, Modular Hypervisor Framework (XMHF)
 * Copyright (c) 2009-2012 Carnegie Mellon University
 * Copyright (c) 2010-2012 VDG Inc.
 * All Rights Reserved.
 *
 * Developed by: XMHF Team
 *               Carnegie Mellon University / CyLab
 *               VDG Inc.
 *               http://xmhf.org
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in
 * the documentation and/or other materials provided with the
 * distribution.
 *
 * Neither the names of Carnegie Mellon or VDG Inc, nor the names of
 * its contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
 * CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * @XMHF_LICENSE_HEADER_END@
 */

/*
 * XMHF: The following file is taken from:
 *  tboot-1.10.5/tboot/txt/txt.c
 * Changes made include:
 *  TODO: Added extra function declarations.
 *  Remove unused symbols.
 *  Update g_mle_hdr.
 *  Remove g_mle_pt.
 *  Adapted find_platform_sinit_module() to check_sinit_module().
 *  Assume force_tpm2_legacy_log=false on command line.
 *  Replace g_sinit with sinit.
 *  Add argument sinit for get_evtlog_type().
 *  Change return type of get_sinit_capabilities() to uint32_t.
 *  TODO: configure_vtd() removed.
 *  Change arguments of init_txt_heap() from loader_ctx *lctx.
 *  Copy populated MLE header into SL
 *  semi-hardcode VT-d PMRs
 *  Assume no LCP module exists.
 *  Assume is_loader_launch_efi() returns false.
 *  Assume get_tboot_prefer_da() returns false.
 *  Move txt_is_launched() out.
 *  Change delay implementation.
 *  Change arguments of txt_launch_environment() from loader_ctx *lctx.
 *  Use sinit = sinit_ptr instead of g_sinit.
 *  TODO: Added copy_sinit() call here.
 *  Skip disabling VGA logging.
 *  Skip printk_flush().
 */

/*
 * txt.c: Intel(r) TXT support functions, including initiating measured
 *        launch, post-launch, AP wakeup, etc.
 *
 * Copyright (c) 2003-2011, Intel Corporation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of the Intel Corporation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <xmhf.h>

// XMHF: TODO: Added extra function declarations.
bool get_parameters(getsec_parameters_t *params);

// XMHF: Remove unused symbols.
///* counter timeout for waiting for all APs to enter wait-for-sipi */
//#define AP_WFS_TIMEOUT     0x10000000

// XMHF: Remove unused symbols.
//struct acpi_rsdp g_rsdp;
//extern char _start[];             /* start of module */
//extern char _end[];               /* end of module */
//extern char _mle_start[];         /* start of text section */
//extern char _mle_end[];           /* end of text section */
//extern char _post_launch_entry[]; /* entry point post SENTER, in boot.S */
//extern char _txt_wakeup[];        /* RLP join address for GETSEC[WAKEUP] */

// XMHF: Remove unused symbols.
//extern long s3_flag;

// XMHF: Remove unused symbols.
//extern struct mutex ap_lock;

// XMHF: Remove unused symbols.
/* MLE/kernel shared data page (in boot.S) */
//extern tboot_shared_t _tboot_shared;
extern void apply_policy(tb_error_t error);
extern void cpu_wakeup(uint32_t cpuid, uint32_t sipi_vec);
extern void print_event(const tpm12_pcr_event_t *evt);
extern void print_event_2(void *evt, uint16_t alg);
extern uint32_t print_event_2_1(void *evt);

// XMHF: Remove unused symbols.
//extern void __enable_nmi(void);

/*
 * this is the structure whose addr we'll put in TXT heap
 * it needs to be within the MLE pages, so force it to the .text section
 */
static const mle_hdr_t g_mle_hdr = {
    uuid              :  MLE_HDR_UUID,
    length            :  sizeof(mle_hdr_t),
    version           :  MLE_HDR_VER,
    // XMHF: Update g_mle_hdr.
    //entry_point       :  (uint32_t)&_post_launch_entry - TBOOT_START,
    entry_point       :  TEMPORARY_HARDCODED_MLE_ENTRYPOINT,
    first_valid_page  :  0,
    // XMHF: Update g_mle_hdr.
    //mle_start_off     :  (uint32_t)&_mle_start - TBOOT_BASE_ADDR,
    //mle_end_off       :  (uint32_t)&_mle_end - TBOOT_BASE_ADDR,
    mle_start_off     :  0,
    mle_end_off       :  TEMPORARY_HARDCODED_MLE_SIZE,
    capabilities      :  { MLE_HDR_CAPS },
    // XMHF: Update g_mle_hdr.
    //cmdline_start_off :  (uint32_t)g_cmdline - TBOOT_BASE_ADDR,
    //cmdline_end_off   :  (uint32_t)g_cmdline + CMDLINE_SIZE - 1 -
    //                                                   TBOOT_BASE_ADDR,
    cmdline_start_off :  0,
    cmdline_end_off   :  0,
};

/*
 * counts of APs going into wait-for-sipi
 */
/* count of APs in WAIT-FOR-SIPI */
// XMHF: Remove unused symbols.
//atomic_t ap_wfs_count;

static void print_file_info(void)
{
    printf("file addresses:\n");
    // XMHF: Remove unused symbols.
    //printf("\t &_start=%p\n", &_start);
    //printf("\t &_end=%p\n", &_end);
    //printf("\t &_mle_start=%p\n", &_mle_start);
    //printf("\t &_mle_end=%p\n", &_mle_end);
    //printf("\t &_post_launch_entry=%p\n", &_post_launch_entry);
    //printf("\t &_txt_wakeup=%p\n", &_txt_wakeup);
    printf("\t &g_mle_hdr=%p\n", &g_mle_hdr);
}

static void print_mle_hdr(const mle_hdr_t *mle_hdr)
{
    printf("MLE header:\n");
    printf("\t uuid="); print_uuid(&mle_hdr->uuid);
    printf("\n");
    printf("\t length=%x\n", mle_hdr->length);
    printf("\t version=%08x\n", mle_hdr->version);
    printf("\t entry_point=%08x\n", mle_hdr->entry_point);
    printf("\t first_valid_page=%08x\n", mle_hdr->first_valid_page);
    printf("\t mle_start_off=%x\n", mle_hdr->mle_start_off);
    printf("\t mle_end_off=%x\n", mle_hdr->mle_end_off);
    print_txt_caps("\t ", mle_hdr->capabilities);
}

/*
 * build_mle_pagetable()
 */

/* page dir/table entry is phys addr + P + R/W + PWT */
#define MAKE_PDTE(addr)  (((uint64_t)(unsigned long)(addr) & PAGE_MASK_4K) | 0x01)

/* we assume/know that our image is <2MB and thus fits w/in a single */
/* PT (512*4KB = 2MB) and thus fixed to 1 pg dir ptr and 1 pgdir and */
/* 1 ptable = 3 pages and just 1 loop loop for ptable MLE page table */
/* can only contain 4k pages */

// XMHF: Remove g_mle_pt.
//static __mlept uint8_t g_mle_pt[3 * PAGE_SIZE_4K];
/* pgdir ptr + pgdir + ptab = 3 */

static void *build_mle_pagetable(uint32_t mle_start, uint32_t mle_size)
{
    void *ptab_base;
    uint32_t ptab_size, mle_off;
    void *pg_dir_ptr_tab, *pg_dir, *pg_tab;
    uint64_t *pte;

    printf("MLE start=0x%x, end=0x%x, size=0x%x\n",
           mle_start, mle_start+mle_size, mle_size);
    if ( mle_size > 512*PAGE_SIZE_4K ) {
        printf("MLE size too big for single page table\n");
        return NULL;
    }


    /* should start on page boundary */
    if ( mle_start & ~PAGE_MASK_4K ) {
        printf("MLE start is not page-aligned\n");
        return NULL;
    }

    /* place ptab_base below MLE */
    // XMHF: Remove g_mle_pt.
    //ptab_size = sizeof(g_mle_pt);
    //ptab_base = &g_mle_pt;
    ptab_size = 3 * PAGE_SIZE_4K;      /* pgdir ptr + pgdir + ptab = 3 */
    ptab_base = (void *)(mle_start - ptab_size);
    memset(ptab_base, 0, ptab_size);
    printf("ptab_size=%x, ptab_base=%p\n", ptab_size, ptab_base);

    pg_dir_ptr_tab = ptab_base;
    pg_dir         = pg_dir_ptr_tab + PAGE_SIZE_4K;
    pg_tab         = pg_dir + PAGE_SIZE_4K;

    /* only use first entry in page dir ptr table */
    *(uint64_t *)pg_dir_ptr_tab = MAKE_PDTE(pg_dir);

    /* only use first entry in page dir */
    *(uint64_t *)pg_dir = MAKE_PDTE(pg_tab);

    pte = pg_tab;
    mle_off = 0;
    do {
        *pte = MAKE_PDTE(mle_start + mle_off);

        pte++;
        mle_off += PAGE_SIZE_4K;
    } while ( mle_off < mle_size );

    return ptab_base;
}

// XMHF: Adapted find_platform_sinit_module() to check_sinit_module().
static bool check_sinit_module(void *base, size_t size)
{
    if ( base == NULL )
        return false;

    if ( is_sinit_acmod(base, size, false) &&
         does_acmod_match_platform((acm_hdr_t *)base) ) {
        printf("SINIT matches platform\n");
        return true;
    }
    /* no SINIT found for this platform */
    printf("no SINIT AC module found\n");
    return false;
}

static event_log_container_t *g_elog = NULL;
static heap_event_log_ptr_elt2_t *g_elog_2 = NULL;
static heap_event_log_ptr_elt2_1_t *g_elog_2_1 = NULL;

/* should be called after os_mle_data initialized */
static void *init_event_log(void)
{
    os_mle_data_t *os_mle_data = get_os_mle_data_start(get_txt_heap());
    g_elog = (event_log_container_t *)&os_mle_data->event_log_buffer;

    memcpy((void *)g_elog->signature, EVTLOG_SIGNATURE,
           sizeof(g_elog->signature));
    g_elog->container_ver_major = EVTLOG_CNTNR_MAJOR_VER;
    g_elog->container_ver_minor = EVTLOG_CNTNR_MINOR_VER;
    g_elog->pcr_event_ver_major = EVTLOG_EVT_MAJOR_VER;
    g_elog->pcr_event_ver_minor = EVTLOG_EVT_MINOR_VER;
    g_elog->size = sizeof(os_mle_data->event_log_buffer);
    g_elog->pcr_events_offset = sizeof(*g_elog);
    g_elog->next_event_offset = sizeof(*g_elog);

    return (void *)g_elog;
}

/* initialize TCG compliant TPM 2.0 event log descriptor */
static void init_evtlog_desc_1(heap_event_log_ptr_elt2_1_t *evt_log)
{
    os_mle_data_t *os_mle_data = get_os_mle_data_start(get_txt_heap());

    evt_log->phys_addr = (uint64_t)(unsigned long)(os_mle_data->event_log_buffer);
    evt_log->allcoated_event_container_size = 2*PAGE_SIZE_4K;
    evt_log->first_record_offset = 0;
    evt_log->next_record_offset = 0;
    printf("TCG compliant TPM 2.0 event log descriptor:\n");
    printf("\t phys_addr = 0x%llX\n",  evt_log->phys_addr);
    printf("\t allcoated_event_container_size = 0x%x \n", evt_log->allcoated_event_container_size);
    printf("\t first_record_offset = 0x%x \n", evt_log->first_record_offset);
    printf("\t next_record_offset = 0x%x \n", evt_log->next_record_offset);
}

static void init_evtlog_desc(heap_event_log_ptr_elt2_t *evt_log)
{
    unsigned int i;
    os_mle_data_t *os_mle_data = get_os_mle_data_start(get_txt_heap());
    struct tpm_if *tpm = get_tpm();
    switch (tpm->extpol) {
    case TB_EXTPOL_AGILE:
        for (i=0; i<evt_log->count; i++) {
            evt_log->event_log_descr[i].alg = tpm->algs_banks[i];
            evt_log->event_log_descr[i].phys_addr =
                    (uint64_t)(unsigned long)(os_mle_data->event_log_buffer + i*4096);
            evt_log->event_log_descr[i].size = 4096;
            evt_log->event_log_descr[i].pcr_events_offset = 0;
            evt_log->event_log_descr[i].next_event_offset = 0;
        }
        break;
    case TB_EXTPOL_EMBEDDED:
        for (i=0; i<evt_log->count; i++) {
            evt_log->event_log_descr[i].alg = tpm->algs[i];
            evt_log->event_log_descr[i].phys_addr =
                    (uint64_t)(unsigned long)(os_mle_data->event_log_buffer + i*4096);
            evt_log->event_log_descr[i].size = 4096;
            evt_log->event_log_descr[i].pcr_events_offset = 0;
            evt_log->event_log_descr[i].next_event_offset = 0;
        }
        break;
    case TB_EXTPOL_FIXED:
        evt_log->event_log_descr[0].alg = tpm->cur_alg;
        evt_log->event_log_descr[0].phys_addr =
                    (uint64_t)(unsigned long)os_mle_data->event_log_buffer;
        evt_log->event_log_descr[0].size = 4096;
        evt_log->event_log_descr[0].pcr_events_offset = 0;
        evt_log->event_log_descr[0].next_event_offset = 0;
        break;
    default:
        return;
    }
}

// XMHF: Add argument sinit for get_evtlog_type().
int get_evtlog_type(acm_hdr_t *sinit)
{
    struct tpm_if *tpm = get_tpm();

    if (tpm->major == TPM12_VER_MAJOR) {
        return EVTLOG_TPM12;
    } else if (tpm->major == TPM20_VER_MAJOR) {
        // XMHF: Assume force_tpm2_legacy_log=false on command line.
        ///*
        // * Force use of legacy TPM2 log format to deal with a bug in some SINIT
        // * ACMs that where they don't log the MLE hash to the event log.
        // */
        //if (get_tboot_force_tpm2_legacy_log()) {
        //    return EVTLOG_TPM2_LEGACY;
        //}
        // XMHF: Replace g_sinit with sinit.
        if (sinit) {
            // XMHF: Change return type of get_sinit_capabilities() to uint32_t.
            // XMHF: Replace g_sinit with sinit.
            //txt_caps_t sinit_caps = get_sinit_capabilities(g_sinit);
            txt_caps_t sinit_caps;
            sinit_caps._raw = get_sinit_capabilities(sinit);
            if (sinit_caps.tcg_event_log_format) {
                printf("get_evtlog_type(): returning EVTLOG_TPM2_TCG\n");
            } else {
                printf("get_evtlog_type(): returning EVTLOG_TPM2_LEGACY\n");
                // TODO: Workaround: txt_heap.c assumes EVTLOG_TPM2_TCG.
                HALT_ON_ERRORCOND(0);
            }
            return sinit_caps.tcg_event_log_format ? EVTLOG_TPM2_TCG : EVTLOG_TPM2_LEGACY;
        } else {
            printf("SINIT not found\n");
        }
    } else {
        printf("Unknown TPM major version: %d\n", tpm->major);
    }
    printf("Unable to determine log type\n");
    return EVTLOG_UNKNOWN;
}

// XMHF: Add argument sinit for get_evtlog_type().
static void init_os_sinit_ext_data(heap_ext_data_element_t* elts, acm_hdr_t *sinit)
{
    heap_ext_data_element_t* elt = elts;
    heap_event_log_ptr_elt_t* evt_log;
    struct tpm_if *tpm = get_tpm();

    int log_type = get_evtlog_type(sinit);
    if ( log_type == EVTLOG_TPM12 ) {
        evt_log = (heap_event_log_ptr_elt_t *)elt->data;
        evt_log->event_log_phys_addr = (uint64_t)(unsigned long)init_event_log();
        elt->type = HEAP_EXTDATA_TYPE_TPM_EVENT_LOG_PTR;
        elt->size = sizeof(*elt) + sizeof(*evt_log);
    } else if ( log_type == EVTLOG_TPM2_TCG ) {
        g_elog_2_1 = (heap_event_log_ptr_elt2_1_t *)elt->data;
        init_evtlog_desc_1(g_elog_2_1);
        elt->type = HEAP_EXTDATA_TYPE_TPM_EVENT_LOG_PTR_2_1;
        elt->size = sizeof(*elt) + sizeof(heap_event_log_ptr_elt2_1_t);
        printf("heap_ext_data_element TYPE = %d \n", elt->type);
        printf("heap_ext_data_element SIZE = %d \n", elt->size);
    }  else if ( log_type == EVTLOG_TPM2_LEGACY ) {
        g_elog_2 = (heap_event_log_ptr_elt2_t *)elt->data;
        if ( tpm->extpol == TB_EXTPOL_AGILE )
            g_elog_2->count = tpm->banks;
        else
            if ( tpm->extpol == TB_EXTPOL_EMBEDDED )
                g_elog_2->count = tpm->alg_count;
            else
                g_elog_2->count = 1;
        init_evtlog_desc(g_elog_2);
        elt->type = HEAP_EXTDATA_TYPE_TPM_EVENT_LOG_PTR_2;
        elt->size = sizeof(*elt) + sizeof(u32) +
            g_elog_2->count * sizeof(heap_event_log_descr_t);
        printf("INTEL TXT LOG elt SIZE = %d \n", elt->size);
    }

    elt = (void *)elt + elt->size;
    elt->type = HEAP_EXTDATA_TYPE_END;
    elt->size = sizeof(*elt);
}

bool evtlog_append_tpm12(uint8_t pcr, tb_hash_t *hash, uint32_t type)
{
    tpm12_pcr_event_t *next;

    if ( g_elog == NULL )
        return true;

    next = (tpm12_pcr_event_t *)((void*)g_elog + g_elog->next_event_offset);

    if ( g_elog->next_event_offset + sizeof(*next) > g_elog->size )
        return false;

    next->pcr_index = pcr;
    next->type = type;
    memcpy(next->digest, hash, sizeof(next->digest));
    next->data_size = 0;

    g_elog->next_event_offset += sizeof(*next) + next->data_size;

    print_event(next);
    return true;
}

void dump_event_2(void)
{
    heap_event_log_descr_t *log_descr;

    for ( unsigned int i=0; i<g_elog_2->count; i++ ) {
        uint32_t hash_size, data_size;
        void *curr, *next;
        log_descr = &g_elog_2->event_log_descr[i];
        printf("\t\t\t Log Descrption:\n");
        printf("\t\t\t             Alg: %u\n", log_descr->alg);
        printf("\t\t\t            Size: %u\n", log_descr->size);
        printf("\t\t\t    EventsOffset: [%u,%u)\n",
                log_descr->pcr_events_offset,
                log_descr->next_event_offset);

        hash_size = get_hash_size(log_descr->alg);
        if ( hash_size == 0 )
            return;

        *((u64 *)(&curr)) = log_descr->phys_addr +
                log_descr->pcr_events_offset;
        *((u64 *)(&next)) = log_descr->phys_addr +
                log_descr->next_event_offset;

        if ( log_descr->alg != TB_HALG_SHA1 ) {
            print_event_2(curr, TB_HALG_SHA1);
            curr += sizeof(tpm12_pcr_event_t) + sizeof(tpm20_log_descr_t);
        }

        while ( curr < next ) {
            print_event_2(curr, log_descr->alg);
            data_size = *(uint32_t *)(curr + 2*sizeof(uint32_t) + hash_size);
            curr += 3*sizeof(uint32_t) + hash_size + data_size;
        }
    }
}

bool evtlog_append_tpm2_legacy(uint8_t pcr, uint16_t alg, tb_hash_t *hash, uint32_t type)
{
    heap_event_log_descr_t *cur_desc = NULL;
    uint32_t hash_size;
    void *cur, *next;

    for ( unsigned int i=0; i<g_elog_2->count; i++ ) {
        if ( g_elog_2->event_log_descr[i].alg == alg ) {
            cur_desc = &g_elog_2->event_log_descr[i];
            break;
        }
    }
    if ( !cur_desc )
        return false;

    hash_size = get_hash_size(alg);
    if ( hash_size == 0 )
        return false;

    if ( cur_desc->next_event_offset + 32 > cur_desc->size )
        return false;

    cur = next = (void *)(unsigned long)cur_desc->phys_addr +
                     cur_desc->next_event_offset;
    *((u32 *)next) = pcr;
    next += sizeof(u32);
    *((u32 *)next) = type;
    next += sizeof(u32);
    memcpy((uint8_t *)next, hash, hash_size);
    next += hash_size;
    *((u32 *)next) = 0;
    cur_desc->next_event_offset += 3*sizeof(uint32_t) + hash_size;

    print_event_2(cur, alg);
    return true;
}

bool evtlog_append_tpm2_tcg(uint8_t pcr, uint32_t type, hash_list_t *hl)
{
    uint32_t i, event_size;
    unsigned int hash_size;
    tcg_pcr_event2 *event;
    uint8_t *hash_entry;
    tcg_pcr_event2 dummy;

    /*
     * Dont't use sizeof(tcg_pcr_event2) since that has TPML_DIGESTV_VALUES_1.digests
     * set to 5. Compute the static size as pcr_index + event_type +
     * digest.count + event_size. Then add the space taken up by the hashes.
     */
    event_size = sizeof(dummy.pcr_index) + sizeof(dummy.event_type) +
        sizeof(dummy.digest.count) + sizeof(dummy.event_size);

    for (i = 0; i < hl->count; i++) {
        hash_size = get_hash_size(hl->entries[i].alg);
        if (hash_size == 0) {
            return false;
        }
        event_size += sizeof(uint16_t); // hash_alg field
        event_size += hash_size;
    }

    // Check if event will fit in buffer.
    if (event_size + g_elog_2_1->next_record_offset >
        g_elog_2_1->allcoated_event_container_size) {
        return false;
    }

    event = (tcg_pcr_event2*)(void *)(unsigned long)(g_elog_2_1->phys_addr +
        g_elog_2_1->next_record_offset);
    event->pcr_index = pcr;
    event->event_type = type;
    event->event_size = 0;  // No event data passed by tboot.
    event->digest.count = hl->count;

    hash_entry = (uint8_t *)&event->digest.digests[0];
    for (i = 0; i < hl->count; i++) {
        // Populate individual TPMT_HA_1 structs.
        *((uint16_t *)hash_entry) = hl->entries[i].alg; // TPMT_HA_1.hash_alg
        hash_entry += sizeof(uint16_t);
        hash_size = get_hash_size(hl->entries[i].alg);  // already checked above
        memcpy(hash_entry, &(hl->entries[i].hash), hash_size);
        hash_entry += hash_size;
    }

    g_elog_2_1->next_record_offset += event_size;
    print_event_2_1(event);
    return true;
}

// XMHF: Add argument sinit for get_evtlog_type().
bool evtlog_append(uint8_t pcr, hash_list_t *hl, uint32_t type, acm_hdr_t *sinit)
{
    int log_type = get_evtlog_type(sinit);
    switch (log_type) {
    case EVTLOG_TPM12:
        if ( !evtlog_append_tpm12(pcr, &hl->entries[0].hash, type) )
            return false;
        break;
    case EVTLOG_TPM2_LEGACY:
        for (unsigned int i=0; i<hl->count; i++) {
            if ( !evtlog_append_tpm2_legacy(pcr, hl->entries[i].alg,
                &hl->entries[i].hash, type))
                return false;
	    }
        break;
    case EVTLOG_TPM2_TCG:
        if ( !evtlog_append_tpm2_tcg(pcr, type, hl) )
            return false;
        break;
    default:
        return false;
    }

    return true;
}

uint32_t g_using_da = 0;
// XMHF: Replace g_sinit with sinit.
//acm_hdr_t *g_sinit = 0;

// XMHF: TODO: configure_vtd() removed.
#if 0
static void configure_vtd(void)
{
    uint32_t remap_length;
    struct dmar_remapping *dmar_remap = vtd_get_dmar_remap(&remap_length);

    if (dmar_remap == NULL) {
        printf("cannot get DMAR remapping structures, skipping configuration\n");
        return;
    } else {
        printf("configuring DMAR remapping\n");
    }

    struct dmar_remapping *iter, *next;
    struct dmar_remapping *end = ((void *)dmar_remap) + remap_length;

    for (iter = dmar_remap; iter < end; iter = next) {
        next = (void *)iter + iter->length;
        if (iter->length == 0) {
            /* Avoid looping forever on bad ACPI tables */
            printf("    invalid 0-length structure\n");
            break;
        } else if (next > end) {
            /* Avoid passing table end */
            printf("    record passes table end\n");
            break;
        }

        if (iter->type == DMAR_REMAPPING_DRHD) {
            if (!vtd_disable_dma_remap(iter)) {
                printf("    vtd_disable_dma_remap failed!\n");
            }
        }
    }

}
#endif

void lxy(void) {
    init_os_sinit_ext_data(NULL, NULL);
}

/*
 * sets up TXT heap
 */
static txt_heap_t *init_txt_heap(void *ptab_base, acm_hdr_t *sinit,
                                 void *phys_mle_start, size_t mle_size)
{
    txt_heap_t *txt_heap;
    uint64_t *size;
    os_mle_data_t *os_mle_data;
    os_sinit_data_t *os_sinit_data;
    /* uint64_t min_lo_ram, max_lo_ram, min_hi_ram, max_hi_ram; */
    txt_caps_t sinit_caps;
    txt_caps_t caps_mask;

    txt_heap = get_txt_heap();

    /*
     * BIOS data already setup by BIOS
     */
    if ( !verify_txt_heap(txt_heap, true) )
        return NULL;

    /*
     * OS/loader to MLE data
     */
    os_mle_data = get_os_mle_data_start(txt_heap);
    size = (uint64_t *)((uint32_t)os_mle_data - sizeof(uint64_t));
    *size = sizeof(*os_mle_data) + sizeof(uint64_t);
    memset(os_mle_data, 0, sizeof(*os_mle_data));
    os_mle_data->version = 0x02;
    os_mle_data->lctx_addr = 0;
    os_mle_data->saved_misc_enable_msr = rdmsr64(MSR_IA32_MISC_ENABLE);

    /*
     * OS/loader to SINIT data
     */
    os_sinit_data = get_os_sinit_data_start(txt_heap);
    size = (uint64_t *)((uint32_t)os_sinit_data - sizeof(uint64_t));
    *size = sizeof(*os_sinit_data) + sizeof(uint64_t);
    memset(os_sinit_data, 0, sizeof(*os_sinit_data));
    os_sinit_data->version = 5; // XXX too magical
    /* this is phys addr */
    os_sinit_data->mle_ptab = (uint64_t)(unsigned long)ptab_base;
    os_sinit_data->mle_size = g_mle_hdr.mle_end_off - g_mle_hdr.mle_start_off;
    /* Copy populated MLE header into SL */
    HALT_ON_ERRORCOND(sizeof(mle_hdr_t) < TEMPORARY_MAX_MLE_HEADER_SIZE);
    memcpy(phys_mle_start, &g_mle_hdr, sizeof(mle_hdr_t));
    printf("Copied mle_hdr (0x%08x, 0x%x bytes) into SL (0x%08x)\n",
           (u32)&g_mle_hdr, sizeof(mle_hdr_t), (u32)phys_mle_start);
    /* this is linear addr (offset from MLE base) of mle header, in MLE page tables */
    os_sinit_data->mle_hdr_base = 0;
    //- (uint64_t)(unsigned long)&_mle_start;
    /* VT-d PMRs */
    /* Must protect MLE, o/w get: TXT.ERRORCODE=c0002871
       AC module error : acm_type=1, progress=07, error=a
       "page is not covered by DPR nor PMR regions" */
    {
		extern u32 sl_rt_size;	//XXX: Ugly hack to bring in SL + runtime size; ideally this should be passed in as another parameter
		(void)mle_size;
		os_sinit_data->vtd_pmr_lo_base = (u64)__TARGET_BASE_SL;
		os_sinit_data->vtd_pmr_lo_size = (u64)PAGE_ALIGN_UP_2M(sl_rt_size);
	}

    /* hi range is >4GB; unused for us */
    os_sinit_data->vtd_pmr_hi_base = 0;
    os_sinit_data->vtd_pmr_hi_size = 0;

    /* LCP owner policy data -- DELETED */

    /* capabilities : choose monitor wake mechanism first */
    ///XXX I don't really understand this
    sinit_caps._raw = get_sinit_capabilities(sinit);
    caps_mask._raw = 0;
    caps_mask.rlp_wake_getsec = caps_mask.rlp_wake_monitor = 1;
    os_sinit_data->capabilities._raw = MLE_HDR_CAPS & ~caps_mask._raw;
    if ( sinit_caps.rlp_wake_monitor )
        os_sinit_data->capabilities.rlp_wake_monitor = 1;
    else if ( sinit_caps.rlp_wake_getsec )
        os_sinit_data->capabilities.rlp_wake_getsec = 1;
    else {     /* should have been detected in verify_acmod() */
        printf("SINIT capabilities are icompatible (0x%x)\n", sinit_caps._raw);
        return NULL;
    }
    /* capabilities : require MLE pagetable in ECX on launch */
    /* TODO: when SINIT ready
     * os_sinit_data->capabilities.ecx_pgtbl = 1;
     */
    os_sinit_data->capabilities.ecx_pgtbl = 0;
    /* TODO: when tboot supports EFI then set efi_rsdt_ptr */

    print_os_sinit_data(os_sinit_data);

    /*
     * SINIT to MLE data will be setup by SINIT
     */

    return txt_heap;
}

void delay(u64 cycles)
{
    uint64_t start = rdtsc64();

    while ( rdtsc64()-start < cycles ) ;
}


tb_error_t txt_launch_environment(void *sinit_ptr, size_t sinit_size,
                                  void *phys_mle_start, size_t mle_size)
{
    acm_hdr_t *sinit;
    void *mle_ptab_base;
    os_mle_data_t *os_mle_data;
    txt_heap_t *txt_heap;

    if(NULL == sinit_ptr) return TB_ERR_SINIT_NOT_PRESENT;
    else sinit = (acm_hdr_t*)sinit_ptr;

    if(!check_sinit_module((void *)sinit, sinit_size)) {
        printf("check_sinit_module failed\n");
        return TB_ERR_SINIT_NOT_PRESENT;
    }
    /* if it is newer than BIOS-provided version, then copy it to */
    /* BIOS reserved region */
    sinit = copy_sinit(sinit);
    if ( sinit == NULL )
        return TB_ERR_SINIT_NOT_PRESENT;
    /* do some checks on it */
    if ( !verify_acmod(sinit) )
        return TB_ERR_ACMOD_VERIFY_FAILED;

    /* print some debug info */
    print_file_info();
    print_mle_hdr(&g_mle_hdr);

    /* create MLE page table */
    mle_ptab_base = build_mle_pagetable((u32)phys_mle_start, mle_size);
    if ( mle_ptab_base == NULL )
        return TB_ERR_FATAL;

    /* initialize TXT heap */
    txt_heap = init_txt_heap(mle_ptab_base, sinit,
                             phys_mle_start, mle_size);
    if ( txt_heap == NULL )
        return TB_ERR_FATAL;

    /* save MTRRs before we alter them for SINIT launch */
    os_mle_data = get_os_mle_data_start(txt_heap);
    save_mtrrs(&(os_mle_data->saved_mtrr_state));

    /* set MTRRs properly for AC module (SINIT) */
    if ( !set_mtrrs_for_acmod(sinit) )
        return TB_ERR_FATAL;

    printf("executing GETSEC[SENTER]...\n");
    /* pause before executing GETSEC[SENTER] */
    delay(0x80000000);

    __getsec_senter((uint32_t)sinit, (sinit->size)*4);
    printf("ERROR--we should not get here!\n");
    return TB_ERR_FATAL;
}


bool txt_prepare_cpu(void)
{
    unsigned long eflags, cr0;
    uint64_t mcg_cap, mcg_stat;
    getsec_parameters_t params;
    unsigned int i;

    /* must be running at CPL 0 => this is implicit in even getting this far */
    /* since our bootstrap code loads a GDT, etc. */
    cr0 = read_cr0();

    /* must be in protected mode */
    if ( !(cr0 & CR0_PE) ) {
        printf("ERR: not in protected mode\n");
        return false;
    }

    /* cache must be enabled (CR0.CD = CR0.NW = 0) */
    if ( cr0 & CR0_CD ) {
        printf("CR0.CD set; clearing it.\n");
        cr0 &= ~CR0_CD;
    }
    if ( cr0 & CR0_NW ) {
        printf("CR0.NW set; clearing it.\n");
        cr0 &= ~CR0_NW;
    }

    /* native FPU error reporting must be enabled for proper */
    /* interaction behavior */
    if ( !(cr0 & CR0_NE) ) {
        printf("CR0.NE not set; setting it.\n");
        cr0 |= CR0_NE;
    }

    write_cr0(cr0);

    /* cannot be in virtual-8086 mode (EFLAGS.VM=1) */
    get_eflags(eflags);
    if ( eflags & EFLAGS_VM ) {
        printf("EFLAGS.VM set; clearing it.\n");
        set_eflags(eflags | ~EFLAGS_VM);
    }

    printf("CR0 and EFLAGS OK\n");

    /*
     * verify that we're not already in a protected environment
     */
    if ( txt_is_launched() ) {
        printf("already in protected environment\n");
        return false;
    }

    /*
     * verify all machine check status registers are clear (unless
     * support preserving them)
     */

    /* no machine check in progress (IA32_MCG_STATUS.MCIP=1) */
    mcg_stat = rdmsr64(MSR_MCG_STATUS);
    if ( mcg_stat & 0x04 ) {
        printf("machine check in progress\n");
        return false;
    }

    if ( !get_parameters(&params) ) {
        printf("get_parameters() failed\n");
        return false;
    }

    /* check if all machine check regs are clear */
    mcg_cap = rdmsr64(MSR_MCG_CAP);
    for ( i = 0; i < (mcg_cap & 0xff); i++ ) {
        mcg_stat = rdmsr64(MSR_MC0_STATUS + 4*i);
        if ( mcg_stat & (1ULL << 63) ) {
            printf("MCG[%u] = %llx ERROR\n", i, mcg_stat);
            if ( !params.preserve_mce )
                return false;
        }
    }

    if ( params.preserve_mce )
        printf("supports preserving machine check errors\n");
    else
        printf("no machine check errors\n");

    if ( params.proc_based_scrtm )
        printf("CPU support processor-based S-CRTM\n");

    /* all is well with the processor state */
    printf("CPU is ready for SENTER\n");

    return true;
}



#define ACM_MEM_TYPE_UC                 0x0100
#define ACM_MEM_TYPE_WC                 0x0200
#define ACM_MEM_TYPE_WT                 0x1000
#define ACM_MEM_TYPE_WP                 0x2000
#define ACM_MEM_TYPE_WB                 0x4000

#define DEF_ACM_MAX_SIZE                0x8000
#define DEF_ACM_VER_MASK                0xffffffff
#define DEF_ACM_VER_SUPPORTED           0x00
#define DEF_ACM_MEM_TYPES               ACM_MEM_TYPE_UC
#define DEF_SENTER_CTRLS                0x00

bool get_parameters(getsec_parameters_t *params)
{
    unsigned long cr4;
    uint32_t index, eax, ebx, ecx;
    int param_type;

    /* sanity check because GETSEC[PARAMETERS] will fail if not set */
    cr4 = read_cr4();
    if ( !(cr4 & CR4_SMXE) ) {
        printf("SMXE not enabled, can't read parameters\n");
        return false;
    }

    memset(params, 0, sizeof(*params));
    params->acm_max_size = DEF_ACM_MAX_SIZE;
    params->acm_mem_types = DEF_ACM_MEM_TYPES;
    params->senter_controls = DEF_SENTER_CTRLS;
    params->proc_based_scrtm = false;
    params->preserve_mce = false;

    index = 0;
    do {
        __getsec_parameters(index++, &param_type, &eax, &ebx, &ecx);
        /* the code generated for a 'switch' statement doesn't work in this */
        /* environment, so use if/else blocks instead */

        /* NULL - all reserved */
        if ( param_type == 0 )
            ;
        /* supported ACM versions */
        else if ( param_type == 1 ) {
            if ( params->n_versions == MAX_SUPPORTED_ACM_VERSIONS )
                printf("number of supported ACM version exceeds "
                       "MAX_SUPPORTED_ACM_VERSIONS\n");
            else {
                params->acm_versions[params->n_versions].mask = ebx;
                params->acm_versions[params->n_versions].version = ecx;
                params->n_versions++;
            }
        }
        /* max size AC execution area */
        else if ( param_type == 2 )
            params->acm_max_size = eax & 0xffffffe0;
        /* supported non-AC mem types */
        else if ( param_type == 3 )
            params->acm_mem_types = eax & 0xffffffe0;
        /* SENTER controls */
        else if ( param_type == 4 )
            params->senter_controls = (eax & 0x00007fff) >> 8;
        /* TXT extensions support */
        else if ( param_type == 5 ) {
            params->proc_based_scrtm = (eax & 0x00000020) ? true : false;
            params->preserve_mce = (eax & 0x00000040) ? true : false;
        }
        else {
            printf("unknown GETSEC[PARAMETERS] type: %d\n", param_type);
            param_type = 0;    /* set so that we break out of the loop */
        }
    } while ( param_type != 0 );

    if ( params->n_versions == 0 ) {
        params->acm_versions[0].mask = DEF_ACM_VER_MASK;
        params->acm_versions[0].version = DEF_ACM_VER_SUPPORTED;
        params->n_versions = 1;
    }

    return true;
}


/*
 * Local variables:
 * mode: C
 * c-set-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
