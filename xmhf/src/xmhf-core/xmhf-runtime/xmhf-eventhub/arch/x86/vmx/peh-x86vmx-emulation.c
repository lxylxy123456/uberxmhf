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

// peh-x86vmx-emulation.c
// Emulate selected x86 instruction
// author: Eric Li (xiaoyili@andrew.cmu.edu)
#include <xmhf.h>

#define INST_LEN_MAX 15

/*
 * Read memory from virtual address space.
 * For normal memory, exec=false. For instruction execution, exec=true.
 */
static void read_memory_gv(guestmem_hptw_ctx_pair_t *ctx_pair, hptw_cpl_t cpl,
						   void *dst, hpt_va_t src, size_t len, bool exec)
{
	VCPU *vcpu = ctx_pair->vcpu;
	memprot_x86vmx_eptlock_read_lock(vcpu);

	/* TODO: Segmentation: logical address -> linear address */
	/* TODO: Paging: linear address -> physical address  */
	//hptw_checked_copy_from_va(ctx, cpl, dst, src, len) == 0;

	if (vcpu->vmcs.guest_CR0 & CR0_PG) {
		guestmem_copy_gv2h(ctx_pair, cpl, dst, src, len);
	}
	memprot_x86vmx_eptlock_read_unlock(vcpu);
}

static void write_memory_gv(guestmem_hptw_ctx_pair_t *ctx_pair, hptw_cpl_t cpl,
							hpt_va_t dst, void *src, size_t len)
{
	(void)ctx_pair;
	(void)cpl;
	(void)dst;
	(void)src;
	(void)len;
	HALT_ON_ERRORCOND(0 && "TODO");
}

// TODO: doc
void emulate_instruction(VCPU * vcpu)
{
	guestmem_hptw_ctx_pair_t ctx_pair;
	guestmem_init(vcpu, &ctx_pair);
	{
		char inst[INST_LEN_MAX] = {};
		uintptr_t rip = vcpu->vmcs.guest_RIP;
		u32 inst_len = vcpu->vmcs.info_vmexit_instruction_length;
		HALT_ON_ERRORCOND(inst_len < INST_LEN_MAX);
		// TODO: use read_memory_gv
		if (vcpu->vmcs.guest_CR0 & CR0_PG) {
			guestmem_copy_gv2h(&ctx_pair, 0, inst, rip, inst_len);
		} else {
			guestmem_copy_gp2h(&ctx_pair, 0, inst, rip, inst_len);
		}
		printf("CPU(0x%02x): emulation %d 0x%llx\n", vcpu->id, inst_len, *(u64 *)inst);
	}
}
