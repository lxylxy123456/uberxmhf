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

/*
 * Maximum supported microcode size. For some CPUs this may need to be much
 * larger (e.g. > 256K).
 */
#define UCODE_TOTAL_SIZE_MAX (PAGE_SIZE_4K)

/* Space to temporarily copy microcode update (prevent TOCTOU attack) */
static u8 ucode_copy_area[MAX_VCPU_ENTRIES * UCODE_TOTAL_SIZE_MAX]
__attribute__(( section(".bss.palign_data") ));

typedef struct __attribute__ ((packed)) {
	u32 header_version;
	u32 update_version;
	u32 date;
	u32 processor_signature;
	u32 checksum;
	u32 loader_version;
	u32 processor_flags;
	u32 data_size;
	u32 total_size;
	u32 reserved[3];
	u8 update_data[0];
} intel_ucode_update_t;

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
	u64 va_header = update_data - sizeof(intel_ucode_update_t);
	u8 *copy_area;
	intel_ucode_update_t *header;
	int result;
	size_t size;
	HALT_ON_ERRORCOND(vcpu->idx < UCODE_TOTAL_SIZE_MAX);
	copy_area = ucode_copy_area + UCODE_TOTAL_SIZE_MAX * vcpu->idx;
	/* Copy header of microcode update */
	header = (intel_ucode_update_t *) copy_area;
	size = sizeof(intel_ucode_update_t);
	result = hptw_checked_copy_from_va(&ctx[1], 0, header, va_header, size);
	HALT_ON_ERRORCOND(result == 0);
	printf("\ndate(mmddyyyy)=%08x, dsize=%d, tsize=%d", header->date,
			header->data_size, header->total_size);
	/* If the following check fails, increase UCODE_TOTAL_SIZE_MAX */
	HALT_ON_ERRORCOND(header->total_size <= UCODE_TOTAL_SIZE_MAX);
	/* Copy the rest of of microcode update */
	size = header->total_size - size;
	result = hptw_checked_copy_from_va(&ctx[1], 0, &header->update_data,
										update_data, size);
	// TODO: check whether update is compatible
	// TODO: compute hash and check
	for (u32 i = 0; i < header->total_size; i++) {
		if (i % 16 == 0) {
			printf("\n%08x  ", i);
		} else if (i % 8 == 0) {
			printf("  ");
		} else {
			printf(" ");
		}
		printf("%02x", copy_area[i]);
	}
	HALT_ON_ERRORCOND(0 && "Not implemented");
}

