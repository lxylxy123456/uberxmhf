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

// nested-x86vmx-ept12.c
// Handle EPT in the guest
// author: Eric Li (xiaoyili@andrew.cmu.edu)
#include <xmhf.h>
#include "nested-x86vmx-ept12.h"

/* Format of EPT12 context information */
typedef struct {
	/* Context of EPT12 */
	hptw_ctx_t ctx;
	/* Context of EPT01 */
	guestmem_hptw_ctx_pair_t ctx01;
} ept12_ctx_t;

static u8 ept02_page_pool[MAX_VCPU_ENTRIES][VMX_NESTED_MAX_ACTIVE_VMCS]
	[EPT02_PAGE_POOL_SIZE][PAGE_SIZE_4K]
	__attribute__((section(".bss.palign_data")));

static u8 ept02_page_alloc[MAX_VCPU_ENTRIES][VMX_NESTED_MAX_ACTIVE_VMCS]
	[EPT02_PAGE_POOL_SIZE];

static void *ept02_gzp(void *vctx, size_t alignment, size_t sz)
{
	ept02_ctx_t *ept02_ctx = (ept02_ctx_t *) vctx;
	u32 i;
	HALT_ON_ERRORCOND(alignment == PAGE_SIZE_4K);
	HALT_ON_ERRORCOND(sz == PAGE_SIZE_4K);
	for (i = 0; i < EPT02_PAGE_POOL_SIZE; i++) {
		if (!ept02_ctx->page_alloc[i]) {
			void *page = ept02_ctx->page_pool[i];
			ept02_ctx->page_alloc[i] = 1;
			memset(page, 0, PAGE_SIZE_4K);
			return page;
		}
	}
	return NULL;
}

static hpt_pa_t ept02_ptr2pa(void *vctx, void *ptr)
{
	(void)vctx;
	return hva2spa(ptr);
}

static void *ept02_pa2ptr(void *vctx, hpt_pa_t spa, size_t sz,
						  hpt_prot_t access_type, hptw_cpl_t cpl,
						  size_t *avail_sz)
{
	(void)vctx;
	(void)access_type;
	(void)cpl;
	*avail_sz = sz;
	return spa2hva(spa);
}

static void *ept12_gzp(void *vctx, size_t alignment, size_t sz)
{
	(void)vctx;
	(void)alignment;
	(void)sz;
	HALT_ON_ERRORCOND(0 && "EPT12 should not call gzp()");
	return NULL;
}

static hpt_pa_t ept12_ptr2pa(void *vctx, void *ptr)
{
	(void)vctx;
	return hva2spa(ptr);
}

static void *ept12_pa2ptr(void *vctx, hpt_pa_t spa, size_t sz,
						  hpt_prot_t access_type, hptw_cpl_t cpl,
						  size_t *avail_sz)
{
	ept12_ctx_t *ctx = vctx;
	return hptw_checked_access_va(&ctx->ctx01.host_ctx,
								  access_type, cpl, spa, sz, avail_sz);
}

/*
 * Initialize the ept02_ctx_t structure in ept02_ctx.
 * The current CPU and VMCS12 is vcpu and vmcs12_info.
 */
void xmhf_nested_arch_x86vmx_ept02_init(VCPU * vcpu,
										vmcs12_info_t * vmcs12_info,
										ept02_ctx_t * ept02_ctx)
{
	u32 i;
	spa_t root_pa;
	ept02_ctx->page_pool = ept02_page_pool[vcpu->idx][vmcs12_info->index];
	ept02_ctx->page_alloc = ept02_page_alloc[vcpu->idx][vmcs12_info->index];
	for (i = 0; i < EPT02_PAGE_POOL_SIZE; i++) {
		ept02_ctx->page_alloc[i] = 0;
	}
	ept02_ctx->ctx.gzp = ept02_gzp;
	ept02_ctx->ctx.pa2ptr = ept02_pa2ptr;
	ept02_ctx->ctx.ptr2pa = ept02_ptr2pa;
	root_pa = hva2spa(ept02_gzp(&ept02_ctx->ctx, PAGE_SIZE_4K, PAGE_SIZE_4K));
	ept02_ctx->ctx.root_pa = root_pa;
	ept02_ctx->ctx.t = HPT_TYPE_EPT;
	__vmx_invept(VMX_INVEPT_SINGLECONTEXT, root_pa);
}

/*
 * Get pointer to EPT02 for current VMCS12. Will fill EPT02 as EPT violation
 * happens.
 */
spa_t xmhf_nested_arch_x86vmx_get_ept02(VCPU * vcpu,
										vmcs12_info_t * vmcs12_info)
{
	spa_t addr = (uintptr_t) vmcs12_info->ept02_ctx.ctx.root_pa;
	(void)vcpu;
	return addr | 0x1e;			// TODO: remove magic number
}

/*
 * Handles an EPT exit received by L0 when running L2. There are 3 cases.
 * 1. L2 accesses legitimate memory, but L0 has not processed the EPT entry in
 *    L1 yet. In this case this function returns 1. XMHF should add EPT entry
 *    to EPT02 and continue running L2.
 * 2. L2 accesses memory not in EPT12. In this case this function returns 2.
 *    XMHF should forward EPT violation to L1.
 * 3. L2 accesses memory not valid in EPT01 (L1 sets up EPT that accesses
 *    illegal memory). In this case this function returns 3. XMHF should halt
 *    for security.
 */
int xmhf_nested_arch_x86vmx_handle_ept02_exit(VCPU * vcpu,
											  vmcs12_info_t * vmcs12_info)
{
	ept12_ctx_t ept12_ctx;
	u64 guest2_paddr = __vmx_vmread64(VMCSENC_guest_paddr);
	gpa_t guest1_paddr;
	spa_t xmhf_paddr;
	hpt_pmeo_t pmeo12;
	hpt_pmeo_t pmeo01;
	hpt_pmeo_t pmeo02;

	// TODO: should be able to cache ept12_ctx
	ept12_ctx.ctx.ptr2pa = ept12_ptr2pa;
	ept12_ctx.ctx.pa2ptr = ept12_pa2ptr;
	ept12_ctx.ctx.gzp = ept12_gzp;
	ept12_ctx.ctx.root_pa = vmcs12_info->guest_ept_root;
	ept12_ctx.ctx.t = HPT_TYPE_EPT;
	guestmem_init(vcpu, &ept12_ctx.ctx01);

	/* Get the entry in EPT12 and the L1 paddr that is to be accessed */
	hptw_get_pmeo(&pmeo12, (hptw_ctx_t *) & ept12_ctx, 1, guest2_paddr);
	if (!hpt_pmeo_is_page(&pmeo12)) {
		return 2;
	}
	/* TODO: Large pages not supported yet */
	HALT_ON_ERRORCOND(pmeo12.lvl == 1);
	guest1_paddr = hpt_pmeo_get_address(&pmeo12);

	/* Get the entry in EPT01 for the L1 paddr */
	hptw_get_pmeo(&pmeo01, (hptw_ctx_t *) & ept12_ctx.ctx01.host_ctx, 1,
				  guest1_paddr);
	if (!hpt_pmeo_is_page(&pmeo01)) {
		return 3;
	}
	/* TODO: Large pages not supported yet */
	HALT_ON_ERRORCOND(pmeo12.lvl == 1);
	xmhf_paddr = hpt_pmeo_get_address(&pmeo01);

	/* TODO: need some logic to decide whether should return 2 or 3. */

	/* Construct page map entry for EPT02 */
	pmeo02.pme = 0;
	pmeo02.t = HPT_TYPE_EPT;
	pmeo02.lvl = 1;
	hpt_pmeo_set_address(&pmeo02, xmhf_paddr);
	{
		hpt_prot_t prot01 = hpt_pmeo_getprot(&pmeo01);
		hpt_prot_t prot12 = hpt_pmeo_getprot(&pmeo12);
		hpt_pmeo_setprot(&pmeo02, prot01 & prot12);
	}
	{
		hpt_pmt_t cache01 = hpt_pmeo_getcache(&pmeo01);
		hpt_pmt_t cache12 = hpt_pmeo_getcache(&pmeo12);
		hpt_pmt_t cache02 = HPT_PMT_UC;
		/*
		 * TODO: full support of cache type operations not supported. Currently
		 * only support WB and UC.
		 */
		HALT_ON_ERRORCOND(cache01 == HPT_PMT_UC || cache01 == HPT_PMT_WB);
		HALT_ON_ERRORCOND(cache12 == HPT_PMT_UC || cache12 == HPT_PMT_WB);
		if (cache01 == HPT_PMT_WB && cache12 == HPT_PMT_WB) {
			cache02 = HPT_PMT_WB;
		}
		hpt_pmeo_setcache(&pmeo02, cache02);
	}
	{
		bool user01 = hpt_pmeo_getuser(&pmeo01);
		bool user12 = hpt_pmeo_getuser(&pmeo12);
		hpt_pmeo_setuser(&pmeo02, user01 && user12);
	}

	/* Put page map entry into EPT02 */
	HALT_ON_ERRORCOND(hptw_insert_pmeo_alloc(&vmcs12_info->ept02_ctx.ctx,
											 &pmeo02, guest2_paddr) == 0);
	printf("CPU(0x%02x): EPT: 0x%08llx 0x%08llx 0x%08llx\n", vcpu->id,
		   guest2_paddr, guest1_paddr, xmhf_paddr);
	return 1;
}
