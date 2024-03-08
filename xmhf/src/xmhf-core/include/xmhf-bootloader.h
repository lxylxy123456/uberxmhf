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

// EMHF boot loader component declarations
// author: Eric Li (xiaoyili@andrew.cmu.edu)

#ifndef __EMHF_BOOTLOADER_H__
#define __EMHF_BOOTLOADER_H__

#ifndef __ASSEMBLY__

#ifdef __UEFI__

typedef struct {
	/* Command line */
	char *cmdline;
	/* Start and end of XMHF runtime */
	uint64_t slrt_start;
	uint64_t slrt_end;
	/* End of nonzero part of XMHF runtime */
#ifdef __SKIP_RUNTIME_BSS__
	uint64_t slrt_nonzero_end;
#endif /* __SKIP_RUNTIME_BSS__ */
	/* Start and end of SINIT module. Not exist if both 0. */
	uint64_t sinit_start;
	uint64_t sinit_end;
	/* ACPI RSDP location */
	uint64_t acpi_rsdp;
	/* Guest state fields before calling efi2init */
	bool interrupt_enabled;
	struct {
		uint16_t guest_ES_selector;
		uint16_t guest_CS_selector;
		uint16_t guest_SS_selector;
		uint16_t guest_DS_selector;
		uint16_t guest_FS_selector;
		uint16_t guest_GS_selector;
		uint16_t guest_LDTR_selector;
		uint16_t guest_TR_selector;
		//uint16_t guest_interrupt_status;
		//uint16_t guest_PML_index;
		//uint64_t guest_VMCS_link_pointer;
		//uint64_t guest_IA32_DEBUGCTL;
		uint64_t guest_IA32_PAT;	/* Note: restored in MSR load area. */
		uint64_t guest_IA32_EFER;	/* Note: restored in MSR load area. */
		//uint64_t guest_IA32_PERF_GLOBAL_CTRL;
		uint64_t guest_PDPTE0;
		uint64_t guest_PDPTE1;
		uint64_t guest_PDPTE2;
		uint64_t guest_PDPTE3;
		//uint64_t guest_IA32_BNDCFGS;
		//uint64_t guest_IA32_RTIT_CTL;
		//uint64_t guest_IA32_PKRS;
		uint32_t guest_ES_limit;
		uint32_t guest_CS_limit;
		uint32_t guest_SS_limit;
		uint32_t guest_DS_limit;
		uint32_t guest_FS_limit;
		uint32_t guest_GS_limit;
		uint32_t guest_LDTR_limit;
		uint32_t guest_TR_limit;
		uint32_t guest_GDTR_limit;
		uint32_t guest_IDTR_limit;
		uint32_t guest_ES_access_rights;
		uint32_t guest_CS_access_rights;
		uint32_t guest_SS_access_rights;
		uint32_t guest_DS_access_rights;
		uint32_t guest_FS_access_rights;
		uint32_t guest_GS_access_rights;
		uint32_t guest_LDTR_access_rights;
		uint32_t guest_TR_access_rights;
		//uint32_t guest_interruptibility;
		//uint32_t guest_activity_state;
		//uint32_t guest_SMBASE;
		uint32_t guest_SYSENTER_CS;
		//uint32_t guest_VMX_preemption_timer_value;
		uintptr_t guest_CR0;	/* Note: guest_CR0 and control_CR0_shadow. */
		uintptr_t guest_CR3;
		uintptr_t guest_CR4;	/* Note: guest_CR4 and control_CR4_shadow. */
		uintptr_t guest_ES_base;
		uintptr_t guest_CS_base;
		uintptr_t guest_SS_base;
		uintptr_t guest_DS_base;
		uintptr_t guest_FS_base;
		uintptr_t guest_GS_base;
		uintptr_t guest_LDTR_base;
		uintptr_t guest_TR_base;
		uintptr_t guest_GDTR_base;
		uintptr_t guest_IDTR_base;
		uintptr_t guest_DR7;
		uintptr_t guest_RSP;
		uintptr_t guest_RIP;
		uintptr_t guest_RFLAGS;
		//uintptr_t guest_pending_debug_x;
		uintptr_t guest_SYSENTER_ESP;
		uintptr_t guest_SYSENTER_EIP;
		//uintptr_t guest_IA32_S_CET;
		//uintptr_t guest_SSP;
		//uintptr_t guest_IA32_INTERRUPT_SSP_TABLE_ADDR;
	};
} xmhf_efi_info_t;

extern void efi2init(xmhf_efi_info_t *xei, uintptr_t *rsp, uintptr_t *rip,
					 uintptr_t *rflags);
extern void cstartup(xmhf_efi_info_t *xei);

#endif /* __UEFI__ */

#endif	//__ASSEMBLY__

#endif //__EMHF_BOOTLOADER_H__

