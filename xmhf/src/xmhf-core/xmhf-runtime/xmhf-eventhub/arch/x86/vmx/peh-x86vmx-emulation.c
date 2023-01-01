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

/* Select a segment that will be used to access memory */
typedef enum cpu_segment_t {
	CPU_SEG_ES,
	CPU_SEG_CS,
	CPU_SEG_SS,
	CPU_SEG_DS,
	CPU_SEG_FS,
	CPU_SEG_GS,
} cpu_segment_t;

/* Environment used to access memory */
typedef struct mem_access_env_t {
	void *haddr;
	gva_t gaddr;
	cpu_segment_t seg;
	size_t size;
	hpt_prot_t mode;
	hptw_cpl_t cpl;
} mem_access_env_t;

/*
 * Given a segment index, translate logical address to linear address.
 * seg: index of segment used by hardware.
 * addr: logical address as input, modified to linear address as output.
 * size: size of access.
 * mode: access mode (read / write / execute). Use HPT_PROT_*_MASK macros.
 * cpl: permission of access.
 *
 * Note: currently XMHF halts when guest makes an invalid access. Ideally XMHF
 * should inject corresponding exceptions like #GP(0).
 *
 * TODO: merge this function with nested-x86vmx-handler1.c
 */
static gva_t _vmx_decode_seg(VCPU * vcpu, cpu_segment_t seg, gva_t addr,
							 size_t size, hpt_prot_t mode, hptw_cpl_t cpl)
{
	/* Get segment fields from VMCS */
	ulong_t base;
	u32 access_rights;
	u32 limit;
	bool g64 = VCPU_g64(vcpu);
	switch (seg) {
	case CPU_SEG_ES:
		base = vcpu->vmcs.guest_ES_base;
		access_rights = vcpu->vmcs.guest_ES_access_rights;
		limit = vcpu->vmcs.guest_ES_limit;
		break;
	case CPU_SEG_CS:
		base = vcpu->vmcs.guest_CS_base;
		access_rights = vcpu->vmcs.guest_CS_access_rights;
		limit = vcpu->vmcs.guest_CS_limit;
		break;
	case CPU_SEG_SS:
		base = vcpu->vmcs.guest_SS_base;
		access_rights = vcpu->vmcs.guest_SS_access_rights;
		limit = vcpu->vmcs.guest_SS_limit;
		break;
	case CPU_SEG_DS:
		base = vcpu->vmcs.guest_DS_base;
		access_rights = vcpu->vmcs.guest_DS_access_rights;
		limit = vcpu->vmcs.guest_DS_limit;
		break;
	case CPU_SEG_FS:
		base = vcpu->vmcs.guest_FS_base;
		access_rights = vcpu->vmcs.guest_FS_access_rights;
		limit = vcpu->vmcs.guest_FS_limit;
		break;
	case CPU_SEG_GS:
		base = vcpu->vmcs.guest_GS_base;
		access_rights = vcpu->vmcs.guest_GS_access_rights;
		limit = vcpu->vmcs.guest_GS_limit;
		break;
	default:
		HALT_ON_ERRORCOND(0 && "Unexpected segment");
	}
	/*
	 * For 32-bit guest, check segment limit.
	 * For 64-bit guest, no limit check is performed (can even wrap around).
	 */
	if (!g64) {
		gva_t addr_max;
		HALT_ON_ERRORCOND(addr <= UINT_MAX);
		if (addr > UINT_MAX - size) {
			HALT_ON_ERRORCOND(0 && "Invalid access: logical address overflow");
		}
		addr_max = addr + size;
		if (addr_max >= limit) {
			HALT_ON_ERRORCOND(0 && "Invalid access: segment limit exceed");
		}
		if (base > UINT_MAX - addr_max) {
			HALT_ON_ERRORCOND(0 && "Not implemented: linear address overflow");
		}
	}
	/* Check access rights. Skip checking if amd64 SS/DS/ES/FS/GS. */
	if (seg == 1 || !g64) {
		/* Check Segment type */
		{
			hpt_prot_t supported_modes = HPT_PROTS_NONE;
			if ((access_rights & (1U << 3)) == 0) {
				/* type = 0 - 7, always has read */
				supported_modes |= HPT_PROT_READ_MASK;
				if (access_rights & (1U << 1)) {
					/* type = 2/3/6/7, has write */
					supported_modes |= HPT_PROT_WRITE_MASK;
				}
				if (access_rights & (1U << 2)) {
					/* type = 4 - 7, expand-down data segment not implemented */
					HALT_ON_ERRORCOND(0 && "Not implemented: expand-down");
				}
			} else {
				/* type = 8 - 15, always has execute */
				supported_modes |= HPT_PROT_EXEC_MASK;
				if (access_rights & (1U << 1)) {
					/* type = 10/11/14/15, has read */
					supported_modes |= HPT_PROT_READ_MASK;
				}
			}
			if ((supported_modes & mode) != mode) {
				HALT_ON_ERRORCOND(0 && "Invalid access: segment type");
			}
		}
		/* Check S - Descriptor type (0 = system; 1 = code or data) */
		HALT_ON_ERRORCOND((access_rights & (1U << 4)));
		/* Check DPL - Descriptor privilege level */
		{
			u32 dpl = (access_rights >> 5) & 0x3;
			if (dpl < (u32) cpl) {
				HALT_ON_ERRORCOND(0 && "Invalid access: DPL");
			}
		}
		/* Check P - Segment present */
		HALT_ON_ERRORCOND((access_rights & (1U << 7)));
	}
	return addr + base;
}

/* Access memory from guest logical address. */
static void access_memory_gv(guestmem_hptw_ctx_pair_t * ctx_pair,
							 mem_access_env_t * env)
{
	VCPU *vcpu = ctx_pair->vcpu;
	hpt_va_t lin_addr;
	size_t copied;
	memprot_x86vmx_eptlock_read_lock(vcpu);

	/* Segmentation: logical address -> linear address */
	lin_addr = _vmx_decode_seg(vcpu, env->seg, env->gaddr, env->size, env->mode,
							   env->cpl);

	/* Paging */
	copied = 0;
	while (copied < env->size) {
		hpt_va_t gva = lin_addr + copied;
		size_t size = env->size - copied;
		hpt_va_t gpa;
		void *hva;

		/* Linear address -> guest physical address */
		if (vcpu->vmcs.guest_CR0 & CR0_PG) {
			hpt_pmeo_t pmeo;
			int ret = hptw_checked_get_pmeo(&pmeo, &ctx_pair->guest_ctx,
										    env->mode, env->cpl, gva);
			HALT_ON_ERRORCOND(ret == 0);
			gpa = hpt_pmeo_va_to_pa(&pmeo, gva);
			size = MIN(size, hpt_remaining_on_page(&pmeo, gpa));
		} else {
			gpa = gva;
		}

		/* Guest physical address -> hypervisor physical address */
		hva = hptw_checked_access_va(&ctx_pair->host_ctx, env->mode, env->cpl,
									 gpa, size, &size);
		if (hva == NULL) {
			/* Memory not in EPT, need special treatment */
			HALT_ON_ERRORCOND(0);
			copied += size;
		} else {
			/* Perform normal memory access */
			if (env->mode & HPT_PROT_WRITE_MASK) {
				memcpy(hva, env->haddr + copied, size);
			} else {
				memcpy(env->haddr + copied, hva, size);
			}
			copied += size;
		}
	}

	memprot_x86vmx_eptlock_read_unlock(vcpu);
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
		{
			mem_access_env_t env = {
				.haddr = inst,
				.gaddr = rip,
				.seg = CPU_SEG_CS,
				.size = inst_len,
				.mode = HPT_PROT_EXEC_MASK,
				.cpl = vcpu->vmcs.guest_CS_selector & 3,
			};
			access_memory_gv(&ctx_pair, &env);
		}
		printf("CPU(0x%02x): emulation %d 0x%llx\n", vcpu->id, inst_len, *(u64 *)inst);
	}
}
