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

// peh-x86vmx-ucode.c
// Handle Intel microcode update
// author: Eric Li (xiaoyili@andrew.cmu.edu)
#include <xmhf.h>

#include <hptw.h>
#include <hpt_emhf.h>

// TODO: rename "wrmsr" in function names
static hpt_pa_t wrmsr_host_ctx_ptr2pa(void *vctx, void *ptr)
{
	(void)vctx;
	return hva2spa(ptr);
}

static void* wrmsr_host_ctx_pa2ptr(void *vctx, hpt_pa_t spa, size_t sz, hpt_prot_t access_type, hptw_cpl_t cpl, size_t *avail_sz)
{
	(void)vctx;
	(void)access_type;
	(void)cpl;
	*avail_sz = sz;
	return spa2hva(spa);
}

static hpt_pa_t wrmsr_guest_ctx_ptr2pa(void __attribute__((unused)) *ctx, void *ptr)
{
	return hva2gpa(ptr);
}

static void* wrmsr_guest_ctx_pa2ptr(void *vctx, hpt_pa_t gpa, size_t sz, hpt_prot_t access_type, hptw_cpl_t cpl, size_t *avail_sz)
{
	// TODO: bad practice
	hptw_ctx_t *ctx = vctx;

	return hptw_checked_access_va(ctx - 1,
		                        access_type,
		                        cpl,
		                        gpa,
		                        sz,
		                        avail_sz);
}

static void* wrmsr_ctx_unimplemented(void *vctx, size_t alignment, size_t sz)
{
	(void)vctx;
	(void)alignment;
	(void)sz;
	HALT_ON_ERRORCOND(0 && "Not implemented");
	return NULL;
}

/*
 * Update microcode.
 * This function should be called when handling WRMSR to IA32_BIOS_UPDT_TRIG.
 * update_data should be EDX:EAX, which is the guest virtual address of "Update
 * Data". The header and other metadata (e.g. size) starts at 48 bytes before
 * this address.
 */
void handle_intel_ucode_update(VCPU *vcpu, u64 update_data)
{
	hpt_type_t guest_t = hpt_emhf_get_guest_hpt_type(vcpu);
	hptw_ctx_t ctx[2] = {
		{
			.ptr2pa = wrmsr_host_ctx_ptr2pa,
			.pa2ptr = wrmsr_host_ctx_pa2ptr,
			.gzp = wrmsr_ctx_unimplemented,
			.root_pa = hpt_eptp_get_address(HPT_TYPE_EPT,
											vcpu->vmcs.control_EPT_pointer),
			.t = HPT_TYPE_EPT,
		},
		{
			.ptr2pa = wrmsr_guest_ctx_ptr2pa,
			.pa2ptr = wrmsr_guest_ctx_pa2ptr,
			.gzp = wrmsr_ctx_unimplemented,
			.root_pa = hpt_cr3_get_address(guest_t, vcpu->vmcs.guest_CR3),
			.t = guest_t,
		}
	};
	int ans[29];
	int result = hptw_checked_copy_from_va(&ctx[1], 0, ans, update_data - 48,
											sizeof(ans));
	HALT_ON_ERRORCOND(result == 0);
	for (int i = 0; i < 29; i++) {
		printf("\nans[%d] = 0x%08x", i, ans[i]);
	}
	HALT_ON_ERRORCOND(0 && "Not implemented");
}

