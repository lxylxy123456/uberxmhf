/*
 * @XMHF_LICENSE_HEADER_START@
 *
 * eXtensible, Modular Hypervisor Framework 64 (XMHF64)
 * Copyright (c) 2009-2012 Carnegie Mellon University
 * Copyright (c) 2010-2012 VDG Inc.
 * Copyright (c) 2023 Eric Li
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

// EMHF memory protection component
// declarations
// author: amit vasudevan (amitvasudevan@acm.org)

#ifndef __EMHF_MEMPROT_H__
#define __EMHF_MEMPROT_H__


#ifndef __ASSEMBLY__

#include <hptw.h>

// memory protection types
#define MEMP_PROT_NOTPRESENT	(1)	// page not present
#define	MEMP_PROT_PRESENT		(2)	// page present
#define MEMP_PROT_READONLY		(4)	// page read-only
#define MEMP_PROT_READWRITE		(8) // page read-write
#define MEMP_PROT_EXECUTE		(16) // page execute
#define MEMP_PROT_NOEXECUTE		(32) // page no-execute
#define MEMP_PROT_MAXVALUE		(MEMP_PROT_NOTPRESENT+MEMP_PROT_PRESENT+MEMP_PROT_READONLY+MEMP_PROT_READWRITE+MEMP_PROT_NOEXECUTE+MEMP_PROT_EXECUTE)

// flush TLB flags
// These flags need to satisfy 2 properties:
// 1. flags can be logically or'ed (used in xmhf_nested_arch_x86vmx_flush_ept02)
// 2. when flags = 0, nothing is done
#define MEMP_FLUSHTLB_EPTP		1	// EPTP changed
#define MEMP_FLUSHTLB_ENTRY		2	// Entries in EPT changed
#define MEMP_FLUSHTLB_MT_ENTRY	4	// Entries changed, but only EPT MT bits

// Structures for guestmem
typedef struct {
	/* guest_ctx must be the first member, see guestmem_guest_ctx_pa2ptr() */
	hptw_ctx_t guest_ctx;
	hptw_ctx_t host_ctx;
	/* Pointer to vcpu */
	VCPU *vcpu;
} guestmem_hptw_ctx_pair_t;

typedef enum cpu_segment_t {
	CPU_SEG_ES,
	CPU_SEG_CS,
	CPU_SEG_SS,
	CPU_SEG_DS,
	CPU_SEG_FS,
	CPU_SEG_GS,
	CPU_SEG_UNKNOWN,
} cpu_segment_t;

//----------------------------------------------------------------------
//exported DATA
//----------------------------------------------------------------------


//----------------------------------------------------------------------
//exported FUNCTIONS
//----------------------------------------------------------------------

//initialize memory protection for a core
void xmhf_memprot_initialize(VCPU *vcpu);

// get level-1 page map address
u64 * xmhf_memprot_get_lvl1_pagemap_address(VCPU *vcpu);

//get level-2 page map address
u64 * xmhf_memprot_get_lvl2_pagemap_address(VCPU *vcpu);

//get level-3 page map address
u64 * xmhf_memprot_get_lvl3_pagemap_address(VCPU *vcpu);

//get level-4 page map address
u64 * xmhf_memprot_get_lvl4_pagemap_address(VCPU *vcpu);

//get default root page map address
u64 * xmhf_memprot_get_default_root_pagemap_address(VCPU *vcpu);

//flush the TLB of all nested page tables in the current core.
//flags is bitwise or of MEMP_FLUSHTLB_* macros. 0 is effectively NOP.
void xmhf_memprot_flushmappings_localtlb(VCPU *vcpu, u32 flags);

//flush the TLB of all nested page tables in all cores (need quiesce).
//flags is bitwise or of MEMP_FLUSHTLB_* macros. 0 is effectively NOP.
void xmhf_memprot_flushmappings_alltlb(VCPU *vcpu, u32 flags);

//set protection for a given physical memory address
void xmhf_memprot_setprot(VCPU *vcpu, u64 gpa, u32 prottype);

//get protection for a given physical memory address
u32 xmhf_memprot_getprot(VCPU *vcpu, u64 gpa);

// Is the given system paddr belong to mHV (XMHF + hypapp)?
bool xmhf_is_mhv_memory(spa_t spa);

// On 32bit machine, we always return 0 - 4G as the machine physical address range, no matter how many memory is installed
// On 64-bit machine, the function queries the E820 map for the used memory region.
bool xmhf_get_machine_paddr_range(spa_t* machine_base_spa, spa_t* machine_limit_spa);


//----------------------------------------------------------------------
//ARCH. BACKENDS
//----------------------------------------------------------------------

//initialize memory protection for a core
void xmhf_memprot_arch_initialize(VCPU *vcpu);

// get level-1 page map address
u64 * xmhf_memprot_arch_get_lvl1_pagemap_address(VCPU *vcpu);

//get level-2 page map address
u64 * xmhf_memprot_arch_get_lvl2_pagemap_address(VCPU *vcpu);

//get level-3 page map address
u64 * xmhf_memprot_arch_get_lvl3_pagemap_address(VCPU *vcpu);

//get level-4 page map address
u64 * xmhf_memprot_arch_get_lvl4_pagemap_address(VCPU *vcpu);

//get default root page map address
u64 * xmhf_memprot_arch_get_default_root_pagemap_address(VCPU *vcpu);

//handle RDMSR on MTRRs
u32 xmhf_memprot_arch_x86vmx_mtrr_read(VCPU *vcpu, u32 msr, u64 *val);

//handle WRMSR on MTRRs
u32 xmhf_memprot_arch_x86vmx_mtrr_write(VCPU *vcpu, u32 msr, u64 val);

//flush the TLB of all nested page tables in the current core
void xmhf_memprot_arch_flushmappings_localtlb(VCPU *vcpu, u32 flags);

//set protection for a given physical memory address
void xmhf_memprot_arch_setprot(VCPU *vcpu, u64 gpa, u32 prottype);

//get protection for a given physical memory address
u32 xmhf_memprot_arch_getprot(VCPU *vcpu, u64 gpa);

// On 32bit machine, we always return 0 - 4G as the machine physical address range, no matter how many memory is installed
// On 64-bit machine, the function queries the E820 map for the used memory region.
bool xmhf_arch_get_machine_paddr_range(spa_t* machine_base_spa, spa_t* machine_limit_spa);


//----------------------------------------------------------------------
//x86 ARCH. INTERFACES
//----------------------------------------------------------------------


//----------------------------------------------------------------------
//x86vmx SUBARCH. INTERFACES
//----------------------------------------------------------------------

void xmhf_memprot_arch_x86vmx_initialize(VCPU *vcpu);	//initialize memory protection for a core
void xmhf_memprot_arch_x86vmx_flushmappings_localtlb(VCPU *vcpu, u32 flags); // flush TLB in current CPU
void xmhf_memprot_arch_x86vmx_setprot(VCPU *vcpu, u64 gpa, u32 prottype); //set protection for a given physical memory address
u32 xmhf_memprot_arch_x86vmx_getprot(VCPU *vcpu, u64 gpa); //get protection for a given physical memory address
u64 xmhf_memprot_arch_x86vmx_get_EPTP(VCPU *vcpu); // get or set EPTP01 (only valid on Intel)
void xmhf_memprot_arch_x86vmx_set_EPTP(VCPU *vcpu, u64 eptp);

void memprot_x86vmx_eptlock_write_lock(VCPU *vcpu);
void memprot_x86vmx_eptlock_write_unlock(VCPU *vcpu);
void memprot_x86vmx_eptlock_read_lock(VCPU *vcpu);
void memprot_x86vmx_eptlock_read_unlock(VCPU *vcpu);

void guestmem_init(VCPU *vcpu, guestmem_hptw_ctx_pair_t *ctx_pair);
void guestmem_copy_gv2h(guestmem_hptw_ctx_pair_t *ctx_pair, hptw_cpl_t cpl,
						void *dst, hpt_va_t src, size_t len);
void guestmem_copy_gp2h(guestmem_hptw_ctx_pair_t *ctx_pair, hptw_cpl_t cpl,
						void *dst, hpt_va_t src, size_t len);
void guestmem_copy_h2gv(guestmem_hptw_ctx_pair_t *ctx_pair, hptw_cpl_t cpl,
						hpt_va_t dst, void *src, size_t len);
void guestmem_copy_h2gp(guestmem_hptw_ctx_pair_t *ctx_pair, hptw_cpl_t cpl,
						hpt_va_t dst, void *src, size_t len);
spa_t guestmem_gpa2spa_page(guestmem_hptw_ctx_pair_t *ctx_pair,
							gpa_t guest_addr);
spa_t guestmem_gpa2spa_size(guestmem_hptw_ctx_pair_t *ctx_pair,
							gpa_t guest_addr, size_t size);
gva_t guestmem_desegment(VCPU * vcpu, cpu_segment_t seg, gva_t addr,
						 size_t size, hpt_prot_t mode, hptw_cpl_t cpl);

//VMX EPT PML4 table buffers
extern u8 g_vmx_ept_pml4_table_buffers[] __attribute__((aligned(PAGE_SIZE_4K)));

//VMX EPT PDP table buffers
extern u8 g_vmx_ept_pdp_table_buffers[] __attribute__((aligned(PAGE_SIZE_4K)));

//VMX EPT PD table buffers
extern u8 g_vmx_ept_pd_table_buffers[] __attribute__((aligned(PAGE_SIZE_4K)));

//VMX EPT P table buffers
extern u8 g_vmx_ept_p_table_buffers[] __attribute__((aligned(PAGE_SIZE_4K)));


//----------------------------------------------------------------------
//x86svm SUBARCH. INTERFACES
//----------------------------------------------------------------------

void xmhf_memprot_arch_x86svm_initialize(VCPU *vcpu);	//initialize memory protection for a core
void xmhf_memprot_arch_x86svm_flushmappings(VCPU *vcpu); //flush hardware page table mappings (TLB)
void xmhf_memprot_arch_x86svm_setprot(VCPU *vcpu, u64 gpa, u32 prottype); //set protection for a given physical memory address
u32 xmhf_memprot_arch_x86svm_getprot(VCPU *vcpu, u64 gpa); //get protection for a given physical memory address
u64 xmhf_memprot_arch_x86svm_get_h_cr3(VCPU *vcpu); // get or set host cr3 (only valid on AMD)
void xmhf_memprot_arch_x86svm_set_h_cr3(VCPU *vcpu, u64 hcr3);

//SVM NPT PDPT buffers
extern u8 g_svm_npt_pdpt_buffers[] __attribute__((aligned(PAGE_SIZE_4K)));

//SVM NPT PDT buffers
extern u8 g_svm_npt_pdts_buffers[] __attribute__((aligned(PAGE_SIZE_4K)));

//SVM NPT PT buffers
extern u8 g_svm_npt_pts_buffers[] __attribute__((aligned(PAGE_SIZE_4K)));


#endif	//__ASSEMBLY__

#endif //__EMHF_MEMPROT_H__
