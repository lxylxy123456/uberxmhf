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

// nested-x86vmx-handler.c
// Intercept handlers for nested virtualization
// author: Eric Li (xiaoyili@andrew.cmu.edu)
#include <xmhf.h>
#include "nested-x86vmx-handler.h"
#include "nested-x86vmx-vmcs12.h"
#include "nested-x86vmx-vminsterr.h"

/* Maximum number of active VMCS per CPU */
#define VMX_NESTED_MAX_ACTIVE_VMCS 10

#define CUR_VMCS_PTR_INVALID 0xffffffffffffffffULL

#define VMX_INST_RFLAGS_MASK \
	((u64) (EFLAGS_CF | EFLAGS_PF | EFLAGS_AF | EFLAGS_ZF | EFLAGS_SF | \
			EFLAGS_OF))

/*
 * This structure follows Table 26-14. Format of the VM-Exit
 * Instruction-Information Field as Used for VMREAD and VMWRITE in Intel's
 * System Programming Guide, Order Number 325384. It covers all structures in
 * Table 26-13. Format of the VM-Exit Instruction-Information Field as Used
 * for VMCLEAR, VMPTRLD, VMPTRST, VMXON, XRSTORS, and XSAVES.
 */
union _vmx_decode_vm_inst_info {
	struct {
		u32 scaling:2;
		u32 undefined2:1;
		u32 reg1:4;				/* Undefined in Table 26-13. */
		u32 address_size:3;
		u32 mem_reg:1;			/* Cleared to 0 in Table 26-13. */
		u32 undefined14_11:4;
		u32 segment_register:3;
		u32 index_reg:4;
		u32 index_reg_invalid:1;
		u32 base_reg:4;
		u32 base_reg_invalid:1;
		u32 reg2:4;				/* Undefined in Table 26-13. */
	};
	u32 raw;
};

/* A blank page in memory for VMCLEAR */
static u8 blank_page[PAGE_SIZE_4K] __attribute__((section(".bss.palign_data")));

/* Track all active VMCS12's in each CPU */
static vmcs12_info_t
	cpu_active_vmcs12[MAX_VCPU_ENTRIES][VMX_NESTED_MAX_ACTIVE_VMCS];

/* The VMCS02's in each CPU */
static u8 cpu_vmcs02[MAX_VCPU_ENTRIES][VMX_NESTED_MAX_ACTIVE_VMCS][PAGE_SIZE_4K]
	__attribute__((section(".bss.palign_data")));

/*
 * Given a segment index, return the segment offset
 * TODO: do we need to check access rights?
 */
static uintptr_t _vmx_decode_seg(u32 seg, VCPU * vcpu)
{
	switch (seg) {
	case 0:
		return vcpu->vmcs.guest_ES_base;
	case 1:
		return vcpu->vmcs.guest_CS_base;
	case 2:
		return vcpu->vmcs.guest_SS_base;
	case 3:
		return vcpu->vmcs.guest_DS_base;
	case 4:
		return vcpu->vmcs.guest_FS_base;
	case 5:
		return vcpu->vmcs.guest_GS_base;
	default:
		HALT_ON_ERRORCOND(0 && "Unexpected segment");
		return 0;
	}
}

/*
 * Access size bytes of memory referenced in memory operand of instruction.
 * The memory content in the guest is returned in dst.
 */
static gva_t _vmx_decode_mem_operand(VCPU * vcpu, struct regs *r)
{
	union _vmx_decode_vm_inst_info inst_info;
	gva_t addr;
	inst_info.raw = vcpu->vmcs.info_vmx_instruction_information;
	addr = _vmx_decode_seg(inst_info.segment_register, vcpu);
	addr += vcpu->vmcs.info_exit_qualification;
	if (!inst_info.base_reg_invalid) {
		addr += *_vmx_decode_reg(inst_info.base_reg, vcpu, r);
	}
	if (!inst_info.index_reg_invalid) {
		uintptr_t *index_reg = _vmx_decode_reg(inst_info.index_reg, vcpu, r);
		addr += *index_reg << inst_info.scaling;
	}
	switch (inst_info.address_size) {
	case 0:					/* 16-bit */
		addr &= 0xffffUL;
		break;
	case 1:					/* 32-bit */
		/* This case may happen if 32-bit guest hypervisor runs in AMD64 XMHF */
		addr &= 0xffffffffUL;
		break;
	case 2:					/* 64-bit */
#ifdef __I386__
		HALT_ON_ERRORCOND(0 && "Unexpected 64-bit address size in i386");
#elif !defined(__AMD64__)
#error "Unsupported Arch"
#endif							/* __I386__ */
		break;
	default:
		HALT_ON_ERRORCOND(0 && "Unexpected address size");
		break;
	}
	return addr;
}

/*
 * Decode the operand for instructions that take one m64 operand. Following
 * Table 26-13. Format of the VM-Exit Instruction-Information Field as Used
 * for VMCLEAR, VMPTRLD, VMPTRST, VMXON, XRSTORS, and XSAVES in Intel's
 * System Programming Guide, Order Number 325384
 */
static gva_t _vmx_decode_m64(VCPU * vcpu, struct regs *r)
{
	union _vmx_decode_vm_inst_info inst_info;
	inst_info.raw = vcpu->vmcs.info_vmx_instruction_information;
	HALT_ON_ERRORCOND(inst_info.mem_reg == 0);
	return _vmx_decode_mem_operand(vcpu, r);
}

/*
 * Decode the operand for instructions for VMREAD and VMWRITE. Following
 * Table 26-14. Format of the VM-Exit Instruction-Information Field as Used for
 * VMREAD and VMWRITE in Intel's System Programming Guide, Order Number 325384.
 * Return operand size in bytes (4 or 8, depending on guest mode).
 * When the value comes from a memory operand, put 0 in pvalue_mem_reg and
 * put the guest virtual address to ppvalue. When the value comes from a
 * register operand, put 1 in pvalue_mem_reg and put the host virtual address
 * to ppvalue.
 */
static size_t _vmx_decode_vmread_vmwrite(VCPU * vcpu, struct regs *r,
										 int is_vmwrite, ulong_t * pencoding,
										 uintptr_t * ppvalue,
										 int *pvalue_mem_reg)
{
	union _vmx_decode_vm_inst_info inst_info;
	ulong_t *encoding;
	size_t size = (VCPU_g64(vcpu) ? sizeof(u64) : sizeof(u32));
	HALT_ON_ERRORCOND(size == (VCPU_glm(vcpu) ? sizeof(u64) : sizeof(u32)));
	inst_info.raw = vcpu->vmcs.info_vmx_instruction_information;
	if (is_vmwrite) {
		encoding = _vmx_decode_reg(inst_info.reg2, vcpu, r);
	} else {
		encoding = _vmx_decode_reg(inst_info.reg1, vcpu, r);
	}
	*pencoding = 0;
	memcpy(pencoding, encoding, size);
	if (inst_info.mem_reg) {
		*pvalue_mem_reg = 1;
		if (is_vmwrite) {
			*ppvalue = (hva_t) _vmx_decode_reg(inst_info.reg1, vcpu, r);
		} else {
			*ppvalue = (hva_t) _vmx_decode_reg(inst_info.reg2, vcpu, r);
		}
	} else {
		*pvalue_mem_reg = 0;
		*ppvalue = _vmx_decode_mem_operand(vcpu, r);
	}
	return size;
}

/* Clear list of active VMCS12's tracked */
static void active_vmcs12_array_init(VCPU * vcpu)
{
	int i;
	for (i = 0; i < VMX_NESTED_MAX_ACTIVE_VMCS; i++) {
		spa_t vmcs02_ptr = hva2spa(cpu_vmcs02[vcpu->idx][i]);
		cpu_active_vmcs12[vcpu->idx][i].vmcs12_ptr = CUR_VMCS_PTR_INVALID;
		cpu_active_vmcs12[vcpu->idx][i].vmcs02_ptr = vmcs02_ptr;
	}
}

/*
 * Look up vmcs_ptr in list of active VMCS12's tracked in the current CPU.
 * A return value of 0 means the VMCS is not active.
 * A VMCS is defined to be active if this function returns non-zero.
 * When vmcs_ptr = CUR_VMCS_PTR_INVALID, a empty slot is returned.
 */
static vmcs12_info_t *find_active_vmcs12(VCPU * vcpu, gpa_t vmcs_ptr)
{
	int i;
	for (i = 0; i < VMX_NESTED_MAX_ACTIVE_VMCS; i++) {
		if (cpu_active_vmcs12[vcpu->idx][i].vmcs12_ptr == vmcs_ptr) {
			return &cpu_active_vmcs12[vcpu->idx][i];
		}
	}
	return NULL;
}

/* Add a new VMCS12 to the array of actives. Initializes underlying VMCS02 */
static void new_active_vmcs12(VCPU * vcpu, gpa_t vmcs_ptr, u32 rev)
{
	vmcs12_info_t *vmcs12_info;
	HALT_ON_ERRORCOND(vmcs_ptr != CUR_VMCS_PTR_INVALID);
	vmcs12_info = find_active_vmcs12(vcpu, CUR_VMCS_PTR_INVALID);
	if (vmcs12_info == NULL) {
		HALT_ON_ERRORCOND(0 && "Too many active VMCS's");
	}
	vmcs12_info->vmcs12_ptr = vmcs_ptr;
	HALT_ON_ERRORCOND(__vmx_vmclear(vmcs12_info->vmcs02_ptr));
	*(u32 *) spa2hva(vmcs12_info->vmcs02_ptr) = rev;
	vmcs12_info->launched = 0;
	memset(&vmcs12_info->vmcs12_value, 0, sizeof(vmcs12_info->vmcs12_value));
	memset(&vmcs12_info->vmcs02_vmexit_msr_store_area, 0,
		   sizeof(vmcs12_info->vmcs02_vmexit_msr_store_area));
	memset(&vmcs12_info->vmcs02_vmexit_msr_load_area, 0,
		   sizeof(vmcs12_info->vmcs02_vmexit_msr_load_area));
	memset(&vmcs12_info->vmcs02_vmentry_msr_load_area, 0,
		   sizeof(vmcs12_info->vmcs02_vmentry_msr_load_area));
}

/*
 * Find VMCS12 pointed by current VMCS pointer.
 * It is illegal to call this function with a invalid current VMCS pointer.
 * This function will never return NULL.
 */
static vmcs12_info_t *find_current_vmcs12(VCPU * vcpu)
{
	vmcs12_info_t *ans;
	gpa_t vmcs_ptr = vcpu->vmx_nested_current_vmcs_pointer;
	HALT_ON_ERRORCOND(vmcs_ptr != CUR_VMCS_PTR_INVALID);
	ans = find_active_vmcs12(vcpu, vmcs_ptr);
	HALT_ON_ERRORCOND(ans != NULL);
	return ans;
}

/* The VMsucceed pseudo-function in SDM "29.2 CONVENTIONS" */
static void _vmx_nested_vm_succeed(VCPU * vcpu)
{
	vcpu->vmcs.guest_RFLAGS &= ~VMX_INST_RFLAGS_MASK;
}

static void _vmx_nested_vm_fail_valid(VCPU * vcpu, u32 error_number)
{
	vmcs12_info_t *vmcs12_info = find_current_vmcs12(vcpu);
	vcpu->vmcs.guest_RFLAGS &= ~VMX_INST_RFLAGS_MASK;
	vcpu->vmcs.guest_RFLAGS |= EFLAGS_ZF;
	vmcs12_info->vmcs12_value.info_vminstr_error = error_number;
}

static void _vmx_nested_vm_fail_invalid(VCPU * vcpu)
{
	vcpu->vmcs.guest_RFLAGS &= ~VMX_INST_RFLAGS_MASK;
	vcpu->vmcs.guest_RFLAGS |= EFLAGS_CF;
}

static void _vmx_nested_vm_fail(VCPU * vcpu, u32 error_number)
{
	if (vcpu->vmx_nested_current_vmcs_pointer != CUR_VMCS_PTR_INVALID) {
		_vmx_nested_vm_fail_valid(vcpu, error_number);
	} else {
		_vmx_nested_vm_fail_invalid(vcpu);
	}
}

/*
 * Check whether addr sets any bit beyond the physical-address width for VMX
 *
 * If IA32_VMX_BASIC[48] = 1, the address is limited to 32-bits.
 */
static u32 _vmx_check_physical_addr_width(VCPU * vcpu, u64 addr)
{
	u32 eax, ebx, ecx, edx;
	u64 paddrmask;
	/* Check whether CPUID 0x80000008 is supported */
	cpuid(0x80000000U, &eax, &ebx, &ecx, &edx);
	HALT_ON_ERRORCOND(eax >= 0x80000008U);
	/* Compute paddrmask from CPUID.80000008H:EAX[7:0] (max phy addr) */
	cpuid(0x80000008U, &eax, &ebx, &ecx, &edx);
	eax &= 0xFFU;
	HALT_ON_ERRORCOND(eax >= 32 && eax < 64);
	paddrmask = (1ULL << eax) - 0x1ULL;
	if (vcpu->vmx_msrs[INDEX_IA32_VMX_BASIC_MSR] & (1ULL << 48)) {
		paddrmask &= (1ULL << 32);
	}
	// TODO: paddrmask can be cached, maybe move code to part-*.c
	return (addr & ~paddrmask) == 0;
}

u32 cpu1202_done[2];

/*
 * Perform VMENTRY. Never returns if succeed. If controls / host state check
 * fails, return error code for _vmx_nested_vm_fail().
 */
static u32 _vmx_vmentry(VCPU * vcpu, vmcs12_info_t * vmcs12_info,
						struct regs *r)
{
	if (cpu1202_done[vcpu->idx]) {
		HALT_ON_ERRORCOND(__vmx_vmread16(0x0000) == 0x0001); /* control_vpid */
		HALT_ON_ERRORCOND(__vmx_vmread16(0x0002) == 0x0000); /* control_post_interrupt_notification_vec */
		HALT_ON_ERRORCOND(__vmx_vmread16(0x0800) == 0x0010); /* guest_ES_selector */
		HALT_ON_ERRORCOND(__vmx_vmread16(0x0802) == 0x0008); /* guest_CS_selector */
		HALT_ON_ERRORCOND(__vmx_vmread16(0x0804) == 0x0010); /* guest_SS_selector */
		HALT_ON_ERRORCOND(__vmx_vmread16(0x0806) == 0x0010); /* guest_DS_selector */
		HALT_ON_ERRORCOND(__vmx_vmread16(0x0808) == 0x0010); /* guest_FS_selector */
		HALT_ON_ERRORCOND(__vmx_vmread16(0x080a) == 0x0010); /* guest_GS_selector */
		HALT_ON_ERRORCOND(__vmx_vmread16(0x080c) == 0x0000); /* guest_LDTR_selector */
		HALT_ON_ERRORCOND(__vmx_vmread16(0x080e) == 0x0018); /* guest_TR_selector */
		HALT_ON_ERRORCOND(__vmx_vmread16(0x0810) == 0x0000); /* guest_interrupt_status */
		HALT_ON_ERRORCOND(__vmx_vmread16(0x0812) == 0x0000); /* guest_PML_index */
		HALT_ON_ERRORCOND(__vmx_vmread16(0x0c00) == 0x0010); /* host_ES_selector */
		HALT_ON_ERRORCOND(__vmx_vmread16(0x0c02) == 0x0008); /* host_CS_selector */
		HALT_ON_ERRORCOND(__vmx_vmread16(0x0c04) == 0x0010); /* host_SS_selector */
		HALT_ON_ERRORCOND(__vmx_vmread16(0x0c06) == 0x0010); /* host_DS_selector */
		HALT_ON_ERRORCOND(__vmx_vmread16(0x0c08) == 0x0010); /* host_FS_selector */
		HALT_ON_ERRORCOND(__vmx_vmread16(0x0c0a) == 0x0010); /* host_GS_selector */
		HALT_ON_ERRORCOND(__vmx_vmread16(0x0c0c) == 0x0018); /* host_TR_selector */
		HALT_ON_ERRORCOND(__vmx_vmread64(0x2000) == 0x000000001c77c000); /* control_IO_BitmapA_address */
		HALT_ON_ERRORCOND(__vmx_vmread64(0x2002) == 0x000000001c77d000); /* control_IO_BitmapB_address */
		HALT_ON_ERRORCOND(__vmx_vmread64(0x2004) == 0x0000000000000000); /* control_MSR_Bitmaps_address */
		HALT_ON_ERRORCOND(__vmx_vmread64(0x2006) == vcpu->vmcs.control_VM_exit_MSR_store_address); /* control_VM_exit_MSR_store_address */
		HALT_ON_ERRORCOND(__vmx_vmread64(0x2008) == vcpu->vmcs.control_VM_exit_MSR_load_address); /* control_VM_exit_MSR_load_address */
		HALT_ON_ERRORCOND(__vmx_vmread64(0x200a) == vcpu->vmcs.control_VM_entry_MSR_load_address); /* control_VM_entry_MSR_load_address */
		HALT_ON_ERRORCOND(__vmx_vmread64(0x200e) == 0x0000000000000000); /* control_PML_address */
		HALT_ON_ERRORCOND(__vmx_vmread64(0x2010) == 0x0000000000000000); /* control_TSC_offset */
		HALT_ON_ERRORCOND(__vmx_vmread64(0x2012) == 0x0000000000000000); /* control_virtual_APIC_address */
		HALT_ON_ERRORCOND(__vmx_vmread64(0x2014) == 0x0000000000000000); /* control_APIC_access_address */
		HALT_ON_ERRORCOND(__vmx_vmread64(0x2016) == 0x0000000000000000); /* control_posted_interrupt_desc_address */
		HALT_ON_ERRORCOND(__vmx_vmread64(0x2018) == 0x0000000000000000); /* control_VM_function_controls */
		HALT_ON_ERRORCOND(__vmx_vmread64(0x201a) == vcpu->vmcs.control_EPT_pointer); /* control_EPT_pointer */
		HALT_ON_ERRORCOND(__vmx_vmread64(0x201c) == 0x0000000000000000); /* control_EOI_exit_bitmap_0 */
		HALT_ON_ERRORCOND(__vmx_vmread64(0x201e) == 0x0000000000000000); /* control_EOI_exit_bitmap_1 */
		HALT_ON_ERRORCOND(__vmx_vmread64(0x2020) == 0x0000000000000000); /* control_EOI_exit_bitmap_2 */
		HALT_ON_ERRORCOND(__vmx_vmread64(0x2022) == 0x0000000000000000); /* control_EOI_exit_bitmap_3 */
		HALT_ON_ERRORCOND(__vmx_vmread64(0x2024) == 0x0000000000000000); /* control_EPTP_list_address */
		HALT_ON_ERRORCOND(__vmx_vmread64(0x2026) == 0x0000000000000000); /* control_VMREAD_bitmap_address */
		HALT_ON_ERRORCOND(__vmx_vmread64(0x2028) == 0x0000000000000000); /* control_VMWRITE_bitmap_address */
		HALT_ON_ERRORCOND(__vmx_vmread64(0x202c) == 0x0000000000000000); /* control_XSS_exiting_bitmap */
		HALT_ON_ERRORCOND(__vmx_vmread64(0x202e) == 0x0000000000000000); /* control_ENCLS_exiting_bitmap */
		HALT_ON_ERRORCOND(__vmx_vmread64(0x2032) == 0x0000000000000000); /* control_TSC_multiplier */
		HALT_ON_ERRORCOND(__vmx_vmread64(0x2400) == 0x00000000fee00300); /* guest_paddr */
		HALT_ON_ERRORCOND(__vmx_vmread64(0x2800) == 0xffffffffffffffff); /* guest_VMCS_link_pointer */
		HALT_ON_ERRORCOND(__vmx_vmread64(0x2802) == 0x0000000000000000); /* guest_IA32_DEBUGCTL */
		HALT_ON_ERRORCOND(__vmx_vmread64(0x2804) == 0x0000000000000000); /* guest_IA32_PAT */
		HALT_ON_ERRORCOND(__vmx_vmread64(0x2806) == 0x0000000000000000); /* guest_IA32_EFER */
		HALT_ON_ERRORCOND(__vmx_vmread64(0x2808) == 0x0000000000000000); /* guest_IA32_PERF_GLOBAL_CTRL */
		HALT_ON_ERRORCOND(__vmx_vmread64(0x280a) == 0x0000000008b61001); /* guest_PDPTE0 */
		HALT_ON_ERRORCOND(__vmx_vmread64(0x280c) == 0x0000000008b62001); /* guest_PDPTE1 */
		HALT_ON_ERRORCOND(__vmx_vmread64(0x280e) == 0x0000000008b63001); /* guest_PDPTE2 */
		HALT_ON_ERRORCOND(__vmx_vmread64(0x2810) == 0x0000000008b64001); /* guest_PDPTE3 */
		HALT_ON_ERRORCOND(__vmx_vmread64(0x2812) == 0x0000000000000000); /* guest_IA32_BNDCFGS */
		HALT_ON_ERRORCOND(__vmx_vmread64(0x2c00) == 0x0000000000000000); /* host_IA32_PAT */
		HALT_ON_ERRORCOND(__vmx_vmread64(0x2c02) == 0x0000000000000000); /* host_IA32_EFER */
		HALT_ON_ERRORCOND(__vmx_vmread64(0x2c04) == 0x0000000000000000); /* host_IA32_PERF_GLOBAL_CTRL */
		HALT_ON_ERRORCOND(__vmx_vmread32(0x4000) == 0x0000003e); /* control_VMX_pin_based */
		HALT_ON_ERRORCOND(__vmx_vmread32(0x4002) == 0x86006172); /* control_VMX_cpu_based */
		HALT_ON_ERRORCOND(__vmx_vmread32(0x4004) == 0x00000000); /* control_exception_bitmap */
		HALT_ON_ERRORCOND(__vmx_vmread32(0x4006) == 0x00000000); /* control_pagefault_errorcode_mask */
		HALT_ON_ERRORCOND(__vmx_vmread32(0x4008) == 0x00000000); /* control_pagefault_errorcode_match */
		HALT_ON_ERRORCOND(__vmx_vmread32(0x400a) == 0x00000000); /* control_CR3_target_count */
		HALT_ON_ERRORCOND(__vmx_vmread32(0x400c) == 0x00036dfb); /* control_VM_exit_controls */
		HALT_ON_ERRORCOND(__vmx_vmread32(0x400e) == 0x00000003); /* control_VM_exit_MSR_store_count */
		HALT_ON_ERRORCOND(__vmx_vmread32(0x4010) == 0x00000003); /* control_VM_exit_MSR_load_count */
		HALT_ON_ERRORCOND(__vmx_vmread32(0x4012) == 0x000011fb); /* control_VM_entry_controls */
		HALT_ON_ERRORCOND(__vmx_vmread32(0x4014) == 0x00000003); /* control_VM_entry_MSR_load_count */
		HALT_ON_ERRORCOND(__vmx_vmread32(0x4016) == 0x00000000); /* control_VM_entry_interruption_information */
		HALT_ON_ERRORCOND(__vmx_vmread32(0x4018) == 0x00000000); /* control_VM_entry_exception_errorcode */
		HALT_ON_ERRORCOND(__vmx_vmread32(0x401a) == 0x00000000); /* control_VM_entry_instruction_length */
		HALT_ON_ERRORCOND(__vmx_vmread32(0x401c) == 0x00000000); /* control_Task_PRivilege_Threshold */
		HALT_ON_ERRORCOND(__vmx_vmread32(0x401e) == 0x000010aa); /* control_VMX_seccpu_based */
		HALT_ON_ERRORCOND(__vmx_vmread32(0x4400) == 0x0000000c); /* info_vminstr_error */
		HALT_ON_ERRORCOND(__vmx_vmread32(0x4402) == 0x00000018); /* info_vmexit_reason */
		HALT_ON_ERRORCOND(__vmx_vmread32(0x4404) == 0x00000000); /* info_vmexit_interrupt_information */
		HALT_ON_ERRORCOND(__vmx_vmread32(0x4406) == 0x00000000); /* info_vmexit_interrupt_error_code */
		HALT_ON_ERRORCOND(__vmx_vmread32(0x4408) == 0x00000000); /* info_IDT_vectoring_information */
		HALT_ON_ERRORCOND(__vmx_vmread32(0x440a) == 0x00000000); /* info_IDT_vectoring_error_code */
		HALT_ON_ERRORCOND(__vmx_vmread32(0x440c) == 0x00000003); /* info_vmexit_instruction_length */
		HALT_ON_ERRORCOND(__vmx_vmread32(0x440e) == 0x00000000); /* info_vmx_instruction_information */
		HALT_ON_ERRORCOND(__vmx_vmread32(0x4800) == 0xffffffff); /* guest_ES_limit */
		HALT_ON_ERRORCOND(__vmx_vmread32(0x4802) == 0xffffffff); /* guest_CS_limit */
		HALT_ON_ERRORCOND(__vmx_vmread32(0x4804) == 0xffffffff); /* guest_SS_limit */
		HALT_ON_ERRORCOND(__vmx_vmread32(0x4806) == 0xffffffff); /* guest_DS_limit */
		HALT_ON_ERRORCOND(__vmx_vmread32(0x4808) == 0xffffffff); /* guest_FS_limit */
		HALT_ON_ERRORCOND(__vmx_vmread32(0x480a) == 0xffffffff); /* guest_GS_limit */
		HALT_ON_ERRORCOND(__vmx_vmread32(0x480c) == 0x00000000); /* guest_LDTR_limit */
		HALT_ON_ERRORCOND(__vmx_vmread32(0x480e) == 0x00000067); /* guest_TR_limit */
		HALT_ON_ERRORCOND(__vmx_vmread32(0x4810) == 0x0000001f); /* guest_GDTR_limit */
		HALT_ON_ERRORCOND(__vmx_vmread32(0x4812) == 0x000003ff); /* guest_IDTR_limit */
		HALT_ON_ERRORCOND(__vmx_vmread32(0x4814) == 0x0000c093); /* guest_ES_access_rights */
		HALT_ON_ERRORCOND(__vmx_vmread32(0x4816) == 0x0000c09b); /* guest_CS_access_rights */
		HALT_ON_ERRORCOND(__vmx_vmread32(0x4818) == 0x0000c093); /* guest_SS_access_rights */
		HALT_ON_ERRORCOND(__vmx_vmread32(0x481a) == 0x0000c093); /* guest_DS_access_rights */
		HALT_ON_ERRORCOND(__vmx_vmread32(0x481c) == 0x0000c093); /* guest_FS_access_rights */
		HALT_ON_ERRORCOND(__vmx_vmread32(0x481e) == 0x0000c093); /* guest_GS_access_rights */
		HALT_ON_ERRORCOND(__vmx_vmread32(0x4820) == 0x00010000); /* guest_LDTR_access_rights */
		HALT_ON_ERRORCOND(__vmx_vmread32(0x4822) == 0x0000008b); /* guest_TR_access_rights */
		HALT_ON_ERRORCOND(__vmx_vmread32(0x4824) == 0x00000008); /* guest_interruptibility */
		HALT_ON_ERRORCOND(__vmx_vmread32(0x4826) == 0x00000000); /* guest_activity_state */
		HALT_ON_ERRORCOND(__vmx_vmread32(0x482a) == 0x00000000); /* guest_SYSENTER_CS */
		HALT_ON_ERRORCOND(__vmx_vmread32(0x482e) == 0x00000000); /* guest_VMX_preemption_timer_value */
		HALT_ON_ERRORCOND(__vmx_vmread32(0x4c00) == 0x00000000); /* host_SYSENTER_CS */
		HALT_ON_ERRORCOND(__vmx_vmreadNW(0x6000) == 0x60000020); /* control_CR0_mask */
		HALT_ON_ERRORCOND(__vmx_vmreadNW(0x6002) == 0x00002000); /* control_CR4_mask */
		HALT_ON_ERRORCOND(__vmx_vmreadNW(0x6004) == 0x80000035); /* control_CR0_shadow */
		HALT_ON_ERRORCOND(__vmx_vmreadNW(0x6006) == 0x00002030); /* control_CR4_shadow */
		HALT_ON_ERRORCOND(__vmx_vmreadNW(0x6400) == 0x00000000); /* info_exit_qualification */
//		HALT_ON_ERRORCOND(__vmx_vmreadNW(0x640a) == 0x00000000); /* info_guest_linear_address */
		HALT_ON_ERRORCOND(__vmx_vmreadNW(0x6800) == 0x80000035); /* guest_CR0 */
		HALT_ON_ERRORCOND(__vmx_vmreadNW(0x6802) == 0x08b60000); /* guest_CR3 */
		HALT_ON_ERRORCOND(__vmx_vmreadNW(0x6804) == 0x00002030); /* guest_CR4 */
		HALT_ON_ERRORCOND(__vmx_vmreadNW(0x6806) == 0x00000000); /* guest_ES_base */
		HALT_ON_ERRORCOND(__vmx_vmreadNW(0x6808) == 0x00000000); /* guest_CS_base */
		HALT_ON_ERRORCOND(__vmx_vmreadNW(0x680a) == 0x00000000); /* guest_SS_base */
		HALT_ON_ERRORCOND(__vmx_vmreadNW(0x680c) == 0x00000000); /* guest_DS_base */
		HALT_ON_ERRORCOND(__vmx_vmreadNW(0x680e) == 0x00000000); /* guest_FS_base */
		HALT_ON_ERRORCOND(__vmx_vmreadNW(0x6810) == 0x00000000); /* guest_GS_base */
		HALT_ON_ERRORCOND(__vmx_vmreadNW(0x6812) == 0x00000000); /* guest_LDTR_base */
		HALT_ON_ERRORCOND(__vmx_vmreadNW(0x6814) == 0x08211240); /* guest_TR_base */
		HALT_ON_ERRORCOND(__vmx_vmreadNW(0x6816) == 0x082127d0); /* guest_GDTR_base */
		HALT_ON_ERRORCOND(__vmx_vmreadNW(0x6818) == 0x0820e010); /* guest_IDTR_base */
		HALT_ON_ERRORCOND(__vmx_vmreadNW(0x681a) == 0x00000400); /* guest_DR7 */
		HALT_ON_ERRORCOND(__vmx_vmreadNW(0x681c) == 0x08bb0ffc); /* guest_RSP */
		HALT_ON_ERRORCOND(__vmx_vmreadNW(0x681e) == 0x0820596f); /* guest_RIP */
		HALT_ON_ERRORCOND(__vmx_vmreadNW(0x6820) == 0x00000002); /* guest_RFLAGS */
		HALT_ON_ERRORCOND(__vmx_vmreadNW(0x6822) == 0x00000000); /* guest_pending_debug_x */
		HALT_ON_ERRORCOND(__vmx_vmreadNW(0x6824) == 0x00000000); /* guest_SYSENTER_ESP */
		HALT_ON_ERRORCOND(__vmx_vmreadNW(0x6826) == 0x00000000); /* guest_SYSENTER_EIP */
		HALT_ON_ERRORCOND(__vmx_vmreadNW(0x6c00) == vcpu->vmcs.host_CR0); /* host_CR0 */
		HALT_ON_ERRORCOND(__vmx_vmreadNW(0x6c02) == vcpu->vmcs.host_CR3); /* host_CR3 */
		HALT_ON_ERRORCOND(__vmx_vmreadNW(0x6c04) == 0x00042030); /* host_CR4 */
		HALT_ON_ERRORCOND(__vmx_vmreadNW(0x6c06) == 0x00000000); /* host_FS_base */
		HALT_ON_ERRORCOND(__vmx_vmreadNW(0x6c08) == 0x00000000); /* host_GS_base */
		HALT_ON_ERRORCOND(__vmx_vmreadNW(0x6c0a) == vcpu->vmcs.host_TR_base); /* host_TR_base */
		HALT_ON_ERRORCOND(__vmx_vmreadNW(0x6c0c) == vcpu->vmcs.host_GDTR_base); /* host_GDTR_base */
		HALT_ON_ERRORCOND(__vmx_vmreadNW(0x6c0e) == vcpu->vmcs.host_IDTR_base); /* host_IDTR_base */
		HALT_ON_ERRORCOND(__vmx_vmreadNW(0x6c10) == 0x00000000); /* host_SYSENTER_ESP */
		HALT_ON_ERRORCOND(__vmx_vmreadNW(0x6c12) == 0x00000000); /* host_SYSENTER_EIP */
		HALT_ON_ERRORCOND(__vmx_vmreadNW(0x6c14) == vcpu->vmcs.host_RSP); /* host_RSP */
		HALT_ON_ERRORCOND(__vmx_vmreadNW(0x6c16) == vcpu->vmcs.host_RIP); /* host_RIP */
	}

//	xmhf_nested_arch_x86vmx_vmread_all(vcpu, "VMENTRY01");
	/*
	   Features notes
	   * "enable VPID" not supported (currently ignore control_vpid in VMCS12)
	   * "VMCS shadowing" not supported (logic not written)
	   * writing to VM-exit information field not supported
	   * "Enable EPT" not supported yet
	   * "EPTP switching" not supported (the only VMFUNC in Intel SDM)
	   * "Sub-page write permissions for EPT" not supported
	   * "Activate tertiary controls" not supported
	 */

	/* Translate VMCS12 to VMCS02 */
	HALT_ON_ERRORCOND(__vmx_vmptrld(vmcs12_info->vmcs02_ptr));
	if (cpu1202_done[vcpu->idx]++) {
		if (1 && vcpu->id != 0) {	/* TODO: manual quiesce */
			xmhf_smpguest_arch_x86vmx_quiesce(vcpu);
			printf("In quiesce %d\n", cpu1202_done[vcpu->idx]);
			xmhf_smpguest_arch_x86vmx_endquiesce(vcpu);
		}
	}

	printf("CPU(0x%02x): nested vmentry\n", vcpu->id);

	__vmx_vmwrite16(0x0000, 0x0000); /* control_vpid */
	__vmx_vmwrite16(0x0002, 0x0000); /* control_post_interrupt_notification_vec */
	__vmx_vmwrite16(0x0800, 0x0010); /* guest_ES_selector */
	__vmx_vmwrite16(0x0802, 0x0008); /* guest_CS_selector */
	__vmx_vmwrite16(0x0804, 0x0010); /* guest_SS_selector */
	__vmx_vmwrite16(0x0806, 0x0010); /* guest_DS_selector */
	__vmx_vmwrite16(0x0808, 0x0010); /* guest_FS_selector */
	__vmx_vmwrite16(0x080a, 0x0010); /* guest_GS_selector */
	__vmx_vmwrite16(0x080c, 0x0000); /* guest_LDTR_selector */
	__vmx_vmwrite16(0x080e, 0x0018); /* guest_TR_selector */
	__vmx_vmwrite16(0x0810, 0x0000); /* guest_interrupt_status */
	__vmx_vmwrite16(0x0812, 0x0000); /* guest_PML_index */
	__vmx_vmwrite16(0x0c00, 0x0010); /* host_ES_selector */
	__vmx_vmwrite16(0x0c02, 0x0008); /* host_CS_selector */
	__vmx_vmwrite16(0x0c04, 0x0010); /* host_SS_selector */
	__vmx_vmwrite16(0x0c06, 0x0010); /* host_DS_selector */
	__vmx_vmwrite16(0x0c08, 0x0010); /* host_FS_selector */
	__vmx_vmwrite16(0x0c0a, 0x0010); /* host_GS_selector */
	__vmx_vmwrite16(0x0c0c, 0x0018); /* host_TR_selector */
	__vmx_vmwrite64(0x2000, 0x0000000000000000); /* control_IO_BitmapA_address */
	__vmx_vmwrite64(0x2002, 0x0000000000000000); /* control_IO_BitmapB_address */
	__vmx_vmwrite64(0x2004, 0x0000000000000000); /* control_MSR_Bitmaps_address */
	__vmx_vmwrite64(0x2006, vcpu->vmcs.control_VM_exit_MSR_store_address); /* control_VM_exit_MSR_store_address */
	__vmx_vmwrite64(0x2008, vcpu->vmcs.control_VM_exit_MSR_load_address); /* control_VM_exit_MSR_load_address */
	__vmx_vmwrite64(0x200a, vcpu->vmcs.control_VM_entry_MSR_load_address); /* control_VM_entry_MSR_load_address */
	__vmx_vmwrite64(0x200e, 0x0000000000000000); /* control_PML_address */
	__vmx_vmwrite64(0x2010, 0x0000000000000000); /* control_TSC_offset */
	__vmx_vmwrite64(0x2012, 0x0000000000000000); /* control_virtual_APIC_address */
	__vmx_vmwrite64(0x2014, 0x0000000000000000); /* control_APIC_access_address */
	__vmx_vmwrite64(0x2016, 0x0000000000000000); /* control_posted_interrupt_desc_address */
	__vmx_vmwrite64(0x2018, 0x0000000000000000); /* control_VM_function_controls */
	__vmx_vmwrite64(0x201a, vcpu->vmcs.control_EPT_pointer); /* control_EPT_pointer */
	__vmx_vmwrite64(0x201c, 0x0000000000000000); /* control_EOI_exit_bitmap_0 */
	__vmx_vmwrite64(0x201e, 0x0000000000000000); /* control_EOI_exit_bitmap_1 */
	__vmx_vmwrite64(0x2020, 0x0000000000000000); /* control_EOI_exit_bitmap_2 */
	__vmx_vmwrite64(0x2022, 0x0000000000000000); /* control_EOI_exit_bitmap_3 */
	__vmx_vmwrite64(0x2024, 0x0000000000000000); /* control_EPTP_list_address */
	__vmx_vmwrite64(0x2026, 0x0000000000000000); /* control_VMREAD_bitmap_address */
	__vmx_vmwrite64(0x2028, 0x0000000000000000); /* control_VMWRITE_bitmap_address */
	__vmx_vmwrite64(0x202c, 0x0000000000000000); /* control_XSS_exiting_bitmap */
	__vmx_vmwrite64(0x202e, 0x0000000000000000); /* control_ENCLS_exiting_bitmap */
	__vmx_vmwrite64(0x2032, 0x0000000000000000); /* control_TSC_multiplier */
	__vmx_vmwrite64(0x2400, 0x0000000000000000); /* guest_paddr */
	__vmx_vmwrite64(0x2800, 0xffffffffffffffff); /* guest_VMCS_link_pointer */
	__vmx_vmwrite64(0x2802, 0x0000000000000000); /* guest_IA32_DEBUGCTL */
	__vmx_vmwrite64(0x2804, 0x0000000000000000); /* guest_IA32_PAT */
	__vmx_vmwrite64(0x2806, 0x0000000000000000); /* guest_IA32_EFER */
	__vmx_vmwrite64(0x2808, 0x0000000000000000); /* guest_IA32_PERF_GLOBAL_CTRL */
	__vmx_vmwrite64(0x280a, 0x0000000008b61001); /* guest_PDPTE0 */
	__vmx_vmwrite64(0x280c, 0x0000000008b62001); /* guest_PDPTE1 */
	__vmx_vmwrite64(0x280e, 0x0000000008b63001); /* guest_PDPTE2 */
	__vmx_vmwrite64(0x2810, 0x0000000008b64001); /* guest_PDPTE3 */
	__vmx_vmwrite64(0x2812, 0x0000000000000000); /* guest_IA32_BNDCFGS */
	__vmx_vmwrite64(0x2c00, 0x0000000000000000); /* host_IA32_PAT */
	__vmx_vmwrite64(0x2c02, 0x0000000000000000); /* host_IA32_EFER */
	__vmx_vmwrite64(0x2c04, 0x0000000000000000); /* host_IA32_PERF_GLOBAL_CTRL */
	__vmx_vmwrite32(0x4000, 0x0000003e); /* control_VMX_pin_based */
	__vmx_vmwrite32(0x4002, 0x8401e172); /* control_VMX_cpu_based */
	__vmx_vmwrite32(0x4004, 0x00000000); /* control_exception_bitmap */
	__vmx_vmwrite32(0x4006, 0x00000000); /* control_pagefault_errorcode_mask */
	__vmx_vmwrite32(0x4008, 0x00000000); /* control_pagefault_errorcode_match */
	__vmx_vmwrite32(0x400a, 0x00000000); /* control_CR3_target_count */
	__vmx_vmwrite32(0x400c, 0x00036dff); /* control_VM_exit_controls */
	__vmx_vmwrite32(0x400e, 0x00000003); /* control_VM_exit_MSR_store_count */
	__vmx_vmwrite32(0x4010, 0x00000003); /* control_VM_exit_MSR_load_count */
	__vmx_vmwrite32(0x4012, 0x000011ff); /* control_VM_entry_controls */
	__vmx_vmwrite32(0x4014, 0x00000003); /* control_VM_entry_MSR_load_count */
	__vmx_vmwrite32(0x4016, 0x00000000); /* control_VM_entry_interruption_information */
	__vmx_vmwrite32(0x4018, 0x00000000); /* control_VM_entry_exception_errorcode */
	__vmx_vmwrite32(0x401a, 0x00000000); /* control_VM_entry_instruction_length */
	__vmx_vmwrite32(0x401c, 0x00000000); /* control_Task_PRivilege_Threshold */
	__vmx_vmwrite32(0x401e, 0x00000002); /* control_VMX_seccpu_based */
	__vmx_vmwrite32(0x4400, 0x0000000c); /* info_vminstr_error */
	__vmx_vmwrite32(0x4402, 0x00000012); /* info_vmexit_reason */
	__vmx_vmwrite32(0x4404, 0x00000000); /* info_vmexit_interrupt_information */
	__vmx_vmwrite32(0x4406, 0x00000000); /* info_vmexit_interrupt_error_code */
	__vmx_vmwrite32(0x4408, 0x00000000); /* info_IDT_vectoring_information */
	__vmx_vmwrite32(0x440a, 0x00000000); /* info_IDT_vectoring_error_code */
	__vmx_vmwrite32(0x440c, 0x00000003); /* info_vmexit_instruction_length */
	__vmx_vmwrite32(0x440e, 0x00000000); /* info_vmx_instruction_information */
	__vmx_vmwrite32(0x4800, 0xffffffff); /* guest_ES_limit */
	__vmx_vmwrite32(0x4802, 0xffffffff); /* guest_CS_limit */
	__vmx_vmwrite32(0x4804, 0xffffffff); /* guest_SS_limit */
	__vmx_vmwrite32(0x4806, 0xffffffff); /* guest_DS_limit */
	__vmx_vmwrite32(0x4808, 0xffffffff); /* guest_FS_limit */
	__vmx_vmwrite32(0x480a, 0xffffffff); /* guest_GS_limit */
	__vmx_vmwrite32(0x480c, 0x00000000); /* guest_LDTR_limit */
	__vmx_vmwrite32(0x480e, 0x00000000); /* guest_TR_limit */
	__vmx_vmwrite32(0x4810, 0x0000ffff); /* guest_GDTR_limit */
	__vmx_vmwrite32(0x4812, 0x0000ffff); /* guest_IDTR_limit */
	__vmx_vmwrite32(0x4814, 0x0000c093); /* guest_ES_access_rights */
	__vmx_vmwrite32(0x4816, 0x0000c09b); /* guest_CS_access_rights */
	__vmx_vmwrite32(0x4818, 0x0000c093); /* guest_SS_access_rights */
	__vmx_vmwrite32(0x481a, 0x0000c093); /* guest_DS_access_rights */
	__vmx_vmwrite32(0x481c, 0x0000c093); /* guest_FS_access_rights */
	__vmx_vmwrite32(0x481e, 0x0000c093); /* guest_GS_access_rights */
	__vmx_vmwrite32(0x4820, 0x00010000); /* guest_LDTR_access_rights */
	__vmx_vmwrite32(0x4822, 0x0000008b); /* guest_TR_access_rights */
	__vmx_vmwrite32(0x4824, 0x00000008); /* guest_interruptibility */
	__vmx_vmwrite32(0x4826, 0x00000000); /* guest_activity_state */
	__vmx_vmwrite32(0x482a, 0x00000000); /* guest_SYSENTER_CS */
	__vmx_vmwrite32(0x482e, 0x00000000); /* guest_VMX_preemption_timer_value */
	__vmx_vmwrite32(0x4c00, 0x00000000); /* host_SYSENTER_CS */
	__vmx_vmwriteNW(0x6000, 0x60000020); /* control_CR0_mask */
	__vmx_vmwriteNW(0x6002, 0x00002000); /* control_CR4_mask */
	__vmx_vmwriteNW(0x6004, 0x00000000); /* control_CR0_shadow */
	__vmx_vmwriteNW(0x6006, 0x00000000); /* control_CR4_shadow */
	__vmx_vmwriteNW(0x6400, 0x00000000); /* info_exit_qualification */
	__vmx_vmwriteNW(0x640a, 0x00000000); /* info_guest_linear_address */
	__vmx_vmwriteNW(0x6800, 0x80000031); /* guest_CR0 */
	__vmx_vmwriteNW(0x6802, 0x08b60000); /* guest_CR3 */
	__vmx_vmwriteNW(0x6804, 0x00002020); /* guest_CR4 */
	__vmx_vmwriteNW(0x6806, 0x00000000); /* guest_ES_base */
	__vmx_vmwriteNW(0x6808, 0x00000000); /* guest_CS_base */
	__vmx_vmwriteNW(0x680a, 0x00000000); /* guest_SS_base */
	__vmx_vmwriteNW(0x680c, 0x00000000); /* guest_DS_base */
	__vmx_vmwriteNW(0x680e, 0x00000000); /* guest_FS_base */
	__vmx_vmwriteNW(0x6810, 0x00000000); /* guest_GS_base */
	__vmx_vmwriteNW(0x6812, 0x00000000); /* guest_LDTR_base */
	__vmx_vmwriteNW(0x6814, 0x00000000); /* guest_TR_base */
	__vmx_vmwriteNW(0x6816, 0x082127d0); /* guest_GDTR_base */
	__vmx_vmwriteNW(0x6818, 0x0820e010); /* guest_IDTR_base */
	__vmx_vmwriteNW(0x681a, 0x00000400); /* guest_DR7 */
	__vmx_vmwriteNW(0x681c, 0x08b8d000); /* guest_RSP */
	__vmx_vmwriteNW(0x681e, 0x0820b7cf); /* guest_RIP */
	__vmx_vmwriteNW(0x6820, 0x00000002); /* guest_RFLAGS */
	__vmx_vmwriteNW(0x6822, 0x00000000); /* guest_pending_debug_x */
	__vmx_vmwriteNW(0x6824, 0x00000000); /* guest_SYSENTER_ESP */
	__vmx_vmwriteNW(0x6826, 0x00000000); /* guest_SYSENTER_EIP */
	__vmx_vmwriteNW(0x6c00, vcpu->vmcs.host_CR0); /* host_CR0 */
	__vmx_vmwriteNW(0x6c02, vcpu->vmcs.host_CR3); /* host_CR3 */
	__vmx_vmwriteNW(0x6c04, 0x00042030); /* host_CR4 */
	__vmx_vmwriteNW(0x6c06, 0x00000000); /* host_FS_base */
	__vmx_vmwriteNW(0x6c08, 0x00000000); /* host_GS_base */
	__vmx_vmwriteNW(0x6c0a, vcpu->vmcs.host_TR_base); /* host_TR_base */
	__vmx_vmwriteNW(0x6c0c, vcpu->vmcs.host_GDTR_base); /* host_GDTR_base */
	__vmx_vmwriteNW(0x6c0e, vcpu->vmcs.host_IDTR_base); /* host_IDTR_base */
	__vmx_vmwriteNW(0x6c10, 0x00000000); /* host_SYSENTER_ESP */
	__vmx_vmwriteNW(0x6c12, 0x00000000); /* host_SYSENTER_EIP */
	__vmx_vmwriteNW(0x6c14, vcpu->vmcs.host_RSP); /* host_RSP */
	__vmx_vmwriteNW(0x6c16, vcpu->vmcs.host_RIP); /* host_RIP */

//	xmhf_nested_arch_x86vmx_vmread_all(vcpu, "VMENTRY02");

	/* From now on, cannot fail */
	vcpu->vmx_nested_is_vmx_root_operation = 0;

	if (vmcs12_info->launched) {
		/* Simply skip L2 guest */
#ifdef SKIP_NESTED_GUEST
		xmhf_nested_arch_x86vmx_handle_vmexit(vcpu, r);
#endif
		__vmx_vmentry_vmresume(r);
	} else {
		vmcs12_info->launched = 1;
		/* Simply skip L2 guest */
#ifdef SKIP_NESTED_GUEST
		xmhf_nested_arch_x86vmx_handle_vmexit(vcpu, r);
#endif
		__vmx_vmentry_vmlaunch(r);
	}

	HALT_ON_ERRORCOND(0 && "VM entry should never return");
	return 0;
}

/*
 * Calculate virtual guest CR0
 *
 * Note: vcpu->vmcs.guest_CR0 is the CR0 on physical CPU when VMX non-root mode.
 * For bits in CR0 that are masked, use the CR0 shadow.
 * For other bits, use guest_CR0
 */
static u64 _vmx_guest_get_guest_cr0(VCPU * vcpu)
{
	return ((vcpu->vmcs.guest_CR0 & ~vcpu->vmcs.control_CR0_mask) |
			(vcpu->vmcs.control_CR0_shadow & vcpu->vmcs.control_CR0_mask));
}

/*
 * Calculate virtual guest CR4
 *
 * Note: vcpu->vmcs.guest_CR4 is the CR4 on physical CPU when VMX non-root mode.
 * For bits in CR4 that are masked, use the CR4 shadow.
 * For other bits, use guest_CR4
 */
static u64 _vmx_guest_get_guest_cr4(VCPU * vcpu)
{
	return ((vcpu->vmcs.guest_CR4 & ~vcpu->vmcs.control_CR4_mask) |
			(vcpu->vmcs.control_CR4_shadow & vcpu->vmcs.control_CR4_mask));
}

/* Get CPL of guest */
static u32 _vmx_guest_get_cpl(VCPU * vcpu)
{
	return (vcpu->vmcs.guest_CS_selector & 0x3);
}

/*
 * Check for conditions that should result in #UD
 *
 * Always check:
 *   (CR0.PE = 0) or (RFLAGS.VM = 1) or (IA32_EFER.LMA = 1 and CS.L = 0)
 * Check for VMXON:
 *   (CR4.VMXE = 0)
 * Check for other than VMXON:
 *   (not in VMX operation)
 * Not checked:
 *   (register operand)
 *
 * Return whether should inject #UD to guest
 */
static u32 _vmx_nested_check_ud(VCPU * vcpu, int is_vmxon)
{
	if ((_vmx_guest_get_guest_cr0(vcpu) & CR0_PE) == 0 ||
		(vcpu->vmcs.guest_RFLAGS & EFLAGS_VM) ||
		((_vmx_get_guest_efer(vcpu) & (1ULL << EFER_LMA)) && !VCPU_g64(vcpu))) {
		return 1;
	}
	if (is_vmxon) {
		if ((_vmx_guest_get_guest_cr4(vcpu) & CR4_VMXE) == 0) {
			return 1;
		}
	} else {
		if (!vcpu->vmx_nested_is_vmx_operation) {
			return 1;
		}
	}
	return 0;
}

/*
 * Virtualize fields in VCPU that tracks nested virtualization information
 */
void xmhf_nested_arch_x86vmx_vcpu_init(VCPU * vcpu)
{
	u32 i;
	vcpu->vmx_nested_is_vmx_operation = 0;
	vcpu->vmx_nested_vmxon_pointer = 0;
	vcpu->vmx_nested_is_vmx_root_operation = 0;
	vcpu->vmx_nested_current_vmcs_pointer = CUR_VMCS_PTR_INVALID;
	/* Compute MSRs for the guest */
	for (i = 0; i < IA32_VMX_MSRCOUNT; i++) {
		vcpu->vmx_nested_msrs[i] = vcpu->vmx_msrs[i];
	}
	{
		u64 vmx_basic_msr = vcpu->vmx_msrs[INDEX_IA32_VMX_BASIC_MSR];
		/* Set bits 44:32 to 4096 (require a full page of VMCS region) */
		vmx_basic_msr &= ~0x00001fff00000000ULL;
		vmx_basic_msr |= 0x0000100000000000ULL;
		/* Do not support dual-monitor treatment of SMI and SMM */
		vmx_basic_msr &= ~(1ULL << 49);
		vcpu->vmx_nested_msrs[INDEX_IA32_VMX_BASIC_MSR] = vmx_basic_msr;
	}
	/* INDEX_IA32_VMX_PINBASED_CTLS_MSR: not changed */
	{
		/* "Activate tertiary controls" not supported */
		u64 mask = ~(1ULL << (32 + VMX_PROCBASED_ACTIVATE_TERTIARY_CONTROLS));
		vcpu->vmx_nested_msrs[INDEX_IA32_VMX_PROCBASED_CTLS_MSR] &= mask;
		vcpu->vmx_nested_msrs[INDEX_IA32_VMX_TRUE_PROCBASED_CTLS_MSR] &= mask;
	}
	{
		/*
		 * Modifying IA32_PAT and IA32_EFER and CET state not supported yet.
		 * Need some extra logic to protect XMHF's states.
		 */
		u64 mask = ~(1ULL << (32 + VMX_VMEXIT_SAVE_IA32_PAT));
		mask &= ~(1ULL << (32 + VMX_VMEXIT_LOAD_IA32_PAT));
		mask &= ~(1ULL << (32 + VMX_VMEXIT_SAVE_IA32_EFER));
		mask &= ~(1ULL << (32 + VMX_VMEXIT_LOAD_IA32_EFER));
		mask &= ~(1ULL << (32 + VMX_VMEXIT_LOAD_CET_STATE));
		vcpu->vmx_nested_msrs[INDEX_IA32_VMX_EXIT_CTLS_MSR] &= mask;
	}
	{
		/*
		 * Modifying IA32_PAT and IA32_EFER and CET state not supported yet.
		 * Need some extra logic to protect XMHF's states.
		 */
		u64 mask = ~(1ULL << (32 + VMX_VMENTRY_LOAD_IA32_PAT));
		mask &= ~(1ULL << (32 + VMX_VMENTRY_LOAD_IA32_EFER));
		mask &= ~(1ULL << (32 + VMX_VMENTRY_LOAD_CET_STATE));
		vcpu->vmx_nested_msrs[INDEX_IA32_VMX_ENTRY_CTLS_MSR] &= mask;
	}
	{
		/* VMWRITE to VM-exit information field not supported */
		u64 mask = ~(1ULL << 29);
		vcpu->vmx_nested_msrs[INDEX_IA32_VMX_MISC_MSR] &= mask;
	}
	/* INDEX_IA32_VMX_CR0_FIXED0_MSR: not changed */
	/* INDEX_IA32_VMX_CR0_FIXED1_MSR: not changed */
	/* INDEX_IA32_VMX_CR4_FIXED0_MSR: not changed */
	/* INDEX_IA32_VMX_CR4_FIXED1_MSR: not changed */
	/* INDEX_IA32_VMX_VMCS_ENUM_MSR: not changed */
	{
		/* "Enable VPID" not supported */
		u64 mask = ~(1ULL << (32 + VMX_SECPROCBASED_ENABLE_VPID));
		/* "VMCS shadowing" not supported */
		mask &= ~(1ULL << (32 + VMX_SECPROCBASED_VMCS_SHADOWING));
		/* "Enable EPT" not supported */
		mask &= ~(1ULL << (32 + VMX_SECPROCBASED_ENABLE_EPT));
		/* "Unrestricted guest" not supported */
		mask &= ~(1ULL << (32 + VMX_SECPROCBASED_UNRESTRICTED_GUEST));
		/* "Sub-page write permissions for EPT" not supported */
		mask &=
			~(1ULL <<
			  (32 + VMX_SECPROCBASED_SUB_PAGE_WRITE_PERMISSIONS_FOR_EPT));
		vcpu->vmx_nested_msrs[INDEX_IA32_VMX_PROCBASED_CTLS2_MSR] &= mask;
	}
	/* Select IA32_VMX_* or IA32_VMX_TRUE_* in guest mode */
	if (vcpu->vmx_msrs[INDEX_IA32_VMX_BASIC_MSR] & (1ULL << 55)) {
		vcpu->vmx_nested_pinbased_ctls =
			vcpu->vmx_nested_msrs[INDEX_IA32_VMX_TRUE_PINBASED_CTLS_MSR];
		vcpu->vmx_nested_procbased_ctls =
			vcpu->vmx_nested_msrs[INDEX_IA32_VMX_TRUE_PROCBASED_CTLS_MSR];
		vcpu->vmx_nested_exit_ctls =
			vcpu->vmx_nested_msrs[INDEX_IA32_VMX_TRUE_EXIT_CTLS_MSR];
		vcpu->vmx_nested_entry_ctls =
			vcpu->vmx_nested_msrs[INDEX_IA32_VMX_TRUE_ENTRY_CTLS_MSR];
	} else {
		vcpu->vmx_nested_pinbased_ctls =
			vcpu->vmx_nested_msrs[INDEX_IA32_VMX_PINBASED_CTLS_MSR];
		vcpu->vmx_nested_procbased_ctls =
			vcpu->vmx_nested_msrs[INDEX_IA32_VMX_PROCBASED_CTLS_MSR];
		vcpu->vmx_nested_exit_ctls =
			vcpu->vmx_nested_msrs[INDEX_IA32_VMX_EXIT_CTLS_MSR];
		vcpu->vmx_nested_entry_ctls =
			vcpu->vmx_nested_msrs[INDEX_IA32_VMX_ENTRY_CTLS_MSR];
	}
}

extern u32 global_bad;

u32 cpu0212_done[2];

/* Handle VMEXIT from nested guest */
void xmhf_nested_arch_x86vmx_handle_vmexit(VCPU * vcpu, struct regs *r)
{
	u32 tmp;

	HALT_ON_ERRORCOND(__vmx_vmread16(0x0000) == 0x0000); /* control_vpid */
	HALT_ON_ERRORCOND(__vmx_vmread16(0x0002) == 0x0000); /* control_post_interrupt_notification_vec */
	HALT_ON_ERRORCOND(__vmx_vmread16(0x0800) == 0x0010); /* guest_ES_selector */
	HALT_ON_ERRORCOND(__vmx_vmread16(0x0802) == 0x0008); /* guest_CS_selector */
	HALT_ON_ERRORCOND(__vmx_vmread16(0x0804) == 0x0010); /* guest_SS_selector */
	HALT_ON_ERRORCOND(__vmx_vmread16(0x0806) == 0x0010); /* guest_DS_selector */
	HALT_ON_ERRORCOND(__vmx_vmread16(0x0808) == 0x0010); /* guest_FS_selector */
	HALT_ON_ERRORCOND(__vmx_vmread16(0x080a) == 0x0010); /* guest_GS_selector */
	HALT_ON_ERRORCOND(__vmx_vmread16(0x080c) == 0x0000); /* guest_LDTR_selector */
	HALT_ON_ERRORCOND(__vmx_vmread16(0x080e) == 0x0018); /* guest_TR_selector */
	HALT_ON_ERRORCOND(__vmx_vmread16(0x0810) == 0x0000); /* guest_interrupt_status */
	HALT_ON_ERRORCOND(__vmx_vmread16(0x0812) == 0x0000); /* guest_PML_index */
	HALT_ON_ERRORCOND(__vmx_vmread16(0x0c00) == 0x0010); /* host_ES_selector */
	HALT_ON_ERRORCOND(__vmx_vmread16(0x0c02) == 0x0008); /* host_CS_selector */
	HALT_ON_ERRORCOND(__vmx_vmread16(0x0c04) == 0x0010); /* host_SS_selector */
	HALT_ON_ERRORCOND(__vmx_vmread16(0x0c06) == 0x0010); /* host_DS_selector */
	HALT_ON_ERRORCOND(__vmx_vmread16(0x0c08) == 0x0010); /* host_FS_selector */
	HALT_ON_ERRORCOND(__vmx_vmread16(0x0c0a) == 0x0010); /* host_GS_selector */
	HALT_ON_ERRORCOND(__vmx_vmread16(0x0c0c) == 0x0018); /* host_TR_selector */
	HALT_ON_ERRORCOND(__vmx_vmread64(0x2000) == 0x0000000000000000); /* control_IO_BitmapA_address */
	HALT_ON_ERRORCOND(__vmx_vmread64(0x2002) == 0x0000000000000000); /* control_IO_BitmapB_address */
	HALT_ON_ERRORCOND(__vmx_vmread64(0x2004) == 0x0000000000000000); /* control_MSR_Bitmaps_address */
	HALT_ON_ERRORCOND(__vmx_vmread64(0x2006) == vcpu->vmcs.control_VM_exit_MSR_store_address); /* control_VM_exit_MSR_store_address */
	HALT_ON_ERRORCOND(__vmx_vmread64(0x2008) == vcpu->vmcs.control_VM_exit_MSR_load_address); /* control_VM_exit_MSR_load_address */
	HALT_ON_ERRORCOND(__vmx_vmread64(0x200a) == vcpu->vmcs.control_VM_entry_MSR_load_address); /* control_VM_entry_MSR_load_address */
	HALT_ON_ERRORCOND(__vmx_vmread64(0x200e) == 0x0000000000000000); /* control_PML_address */
	HALT_ON_ERRORCOND(__vmx_vmread64(0x2010) == 0x0000000000000000); /* control_TSC_offset */
	HALT_ON_ERRORCOND(__vmx_vmread64(0x2012) == 0x0000000000000000); /* control_virtual_APIC_address */
	HALT_ON_ERRORCOND(__vmx_vmread64(0x2014) == 0x0000000000000000); /* control_APIC_access_address */
	HALT_ON_ERRORCOND(__vmx_vmread64(0x2016) == 0x0000000000000000); /* control_posted_interrupt_desc_address */
	HALT_ON_ERRORCOND(__vmx_vmread64(0x2018) == 0x0000000000000000); /* control_VM_function_controls */
	HALT_ON_ERRORCOND(__vmx_vmread64(0x201a) == vcpu->vmcs.control_EPT_pointer); /* control_EPT_pointer */
	HALT_ON_ERRORCOND(__vmx_vmread64(0x201c) == 0x0000000000000000); /* control_EOI_exit_bitmap_0 */
	HALT_ON_ERRORCOND(__vmx_vmread64(0x201e) == 0x0000000000000000); /* control_EOI_exit_bitmap_1 */
	HALT_ON_ERRORCOND(__vmx_vmread64(0x2020) == 0x0000000000000000); /* control_EOI_exit_bitmap_2 */
	HALT_ON_ERRORCOND(__vmx_vmread64(0x2022) == 0x0000000000000000); /* control_EOI_exit_bitmap_3 */
	HALT_ON_ERRORCOND(__vmx_vmread64(0x2024) == 0x0000000000000000); /* control_EPTP_list_address */
	HALT_ON_ERRORCOND(__vmx_vmread64(0x2026) == 0x0000000000000000); /* control_VMREAD_bitmap_address */
	HALT_ON_ERRORCOND(__vmx_vmread64(0x2028) == 0x0000000000000000); /* control_VMWRITE_bitmap_address */
	HALT_ON_ERRORCOND(__vmx_vmread64(0x202c) == 0x0000000000000000); /* control_XSS_exiting_bitmap */
	HALT_ON_ERRORCOND(__vmx_vmread64(0x202e) == 0x0000000000000000); /* control_ENCLS_exiting_bitmap */
	HALT_ON_ERRORCOND(__vmx_vmread64(0x2032) == 0x0000000000000000); /* control_TSC_multiplier */
	HALT_ON_ERRORCOND(__vmx_vmread64(0x2400) == 0x0000000000000000); /* guest_paddr */
	HALT_ON_ERRORCOND(__vmx_vmread64(0x2800) == 0xffffffffffffffff); /* guest_VMCS_link_pointer */
	HALT_ON_ERRORCOND(__vmx_vmread64(0x2802) == 0x0000000000000000); /* guest_IA32_DEBUGCTL */
	HALT_ON_ERRORCOND(__vmx_vmread64(0x2804) == 0x0000000000000000); /* guest_IA32_PAT */
	HALT_ON_ERRORCOND(__vmx_vmread64(0x2806) == 0x0000000000000000); /* guest_IA32_EFER */
	HALT_ON_ERRORCOND(__vmx_vmread64(0x2808) == 0x0000000000000000); /* guest_IA32_PERF_GLOBAL_CTRL */
	HALT_ON_ERRORCOND(__vmx_vmread64(0x280a) == 0x0000000008b61001); /* guest_PDPTE0 */
	HALT_ON_ERRORCOND(__vmx_vmread64(0x280c) == 0x0000000008b62001); /* guest_PDPTE1 */
	HALT_ON_ERRORCOND(__vmx_vmread64(0x280e) == 0x0000000008b63001); /* guest_PDPTE2 */
	HALT_ON_ERRORCOND(__vmx_vmread64(0x2810) == 0x0000000008b64001); /* guest_PDPTE3 */
	HALT_ON_ERRORCOND(__vmx_vmread64(0x2812) == 0x0000000000000000); /* guest_IA32_BNDCFGS */
	HALT_ON_ERRORCOND(__vmx_vmread64(0x2c00) == 0x0000000000000000); /* host_IA32_PAT */
	HALT_ON_ERRORCOND(__vmx_vmread64(0x2c02) == 0x0000000000000000); /* host_IA32_EFER */
	HALT_ON_ERRORCOND(__vmx_vmread64(0x2c04) == 0x0000000000000000); /* host_IA32_PERF_GLOBAL_CTRL */
	HALT_ON_ERRORCOND(__vmx_vmread32(0x4000) == 0x0000003e); /* control_VMX_pin_based */
	HALT_ON_ERRORCOND(__vmx_vmread32(0x4002) == 0x8401e172); /* control_VMX_cpu_based */
	HALT_ON_ERRORCOND(__vmx_vmread32(0x4004) == 0x00000000); /* control_exception_bitmap */
	HALT_ON_ERRORCOND(__vmx_vmread32(0x4006) == 0x00000000); /* control_pagefault_errorcode_mask */
	HALT_ON_ERRORCOND(__vmx_vmread32(0x4008) == 0x00000000); /* control_pagefault_errorcode_match */
	HALT_ON_ERRORCOND(__vmx_vmread32(0x400a) == 0x00000000); /* control_CR3_target_count */
	HALT_ON_ERRORCOND(__vmx_vmread32(0x400c) == 0x00036dff); /* control_VM_exit_controls */
	HALT_ON_ERRORCOND(__vmx_vmread32(0x400e) == 0x00000003); /* control_VM_exit_MSR_store_count */
	HALT_ON_ERRORCOND(__vmx_vmread32(0x4010) == 0x00000003); /* control_VM_exit_MSR_load_count */
	HALT_ON_ERRORCOND(__vmx_vmread32(0x4012) == 0x000011ff); /* control_VM_entry_controls */
	HALT_ON_ERRORCOND(__vmx_vmread32(0x4014) == 0x00000003); /* control_VM_entry_MSR_load_count */
	HALT_ON_ERRORCOND(__vmx_vmread32(0x4016) == 0x00000000); /* control_VM_entry_interruption_information */
	HALT_ON_ERRORCOND(__vmx_vmread32(0x4018) == 0x00000000); /* control_VM_entry_exception_errorcode */
	HALT_ON_ERRORCOND(__vmx_vmread32(0x401a) == 0x00000000); /* control_VM_entry_instruction_length */
	HALT_ON_ERRORCOND(__vmx_vmread32(0x401c) == 0x00000000); /* control_Task_PRivilege_Threshold */
	HALT_ON_ERRORCOND(__vmx_vmread32(0x401e) == 0x00000002); /* control_VMX_seccpu_based */
	HALT_ON_ERRORCOND(__vmx_vmread32(0x4400) == 0x0000000c); /* info_vminstr_error */
	HALT_ON_ERRORCOND(__vmx_vmread32(0x4402) == 0x00000012); /* info_vmexit_reason */
	HALT_ON_ERRORCOND(__vmx_vmread32(0x4404) == 0x00000000); /* info_vmexit_interrupt_information */
	HALT_ON_ERRORCOND(__vmx_vmread32(0x4406) == 0x00000000); /* info_vmexit_interrupt_error_code */
	HALT_ON_ERRORCOND(__vmx_vmread32(0x4408) == 0x00000000); /* info_IDT_vectoring_information */
	HALT_ON_ERRORCOND(__vmx_vmread32(0x440a) == 0x00000000); /* info_IDT_vectoring_error_code */
	HALT_ON_ERRORCOND(__vmx_vmread32(0x440c) == 0x00000003); /* info_vmexit_instruction_length */
	HALT_ON_ERRORCOND(__vmx_vmread32(0x440e) == 0x00000000); /* info_vmx_instruction_information */
	HALT_ON_ERRORCOND(__vmx_vmread32(0x4800) == 0xffffffff); /* guest_ES_limit */
	HALT_ON_ERRORCOND(__vmx_vmread32(0x4802) == 0xffffffff); /* guest_CS_limit */
	HALT_ON_ERRORCOND(__vmx_vmread32(0x4804) == 0xffffffff); /* guest_SS_limit */
	HALT_ON_ERRORCOND(__vmx_vmread32(0x4806) == 0xffffffff); /* guest_DS_limit */
	HALT_ON_ERRORCOND(__vmx_vmread32(0x4808) == 0xffffffff); /* guest_FS_limit */
	HALT_ON_ERRORCOND(__vmx_vmread32(0x480a) == 0xffffffff); /* guest_GS_limit */
	HALT_ON_ERRORCOND(__vmx_vmread32(0x480c) == 0x00000000); /* guest_LDTR_limit */
	HALT_ON_ERRORCOND(__vmx_vmread32(0x480e) == 0x00000000); /* guest_TR_limit */
	HALT_ON_ERRORCOND(__vmx_vmread32(0x4810) == 0x0000ffff); /* guest_GDTR_limit */
	HALT_ON_ERRORCOND(__vmx_vmread32(0x4812) == 0x0000ffff); /* guest_IDTR_limit */
	HALT_ON_ERRORCOND(__vmx_vmread32(0x4814) == 0x0000c093); /* guest_ES_access_rights */
	HALT_ON_ERRORCOND(__vmx_vmread32(0x4816) == 0x0000c09b); /* guest_CS_access_rights */
	HALT_ON_ERRORCOND(__vmx_vmread32(0x4818) == 0x0000c093); /* guest_SS_access_rights */
	HALT_ON_ERRORCOND(__vmx_vmread32(0x481a) == 0x0000c093); /* guest_DS_access_rights */
	HALT_ON_ERRORCOND(__vmx_vmread32(0x481c) == 0x0000c093); /* guest_FS_access_rights */
	HALT_ON_ERRORCOND(__vmx_vmread32(0x481e) == 0x0000c093); /* guest_GS_access_rights */
	HALT_ON_ERRORCOND(__vmx_vmread32(0x4820) == 0x00010000); /* guest_LDTR_access_rights */
	HALT_ON_ERRORCOND(__vmx_vmread32(0x4822) == 0x0000008b); /* guest_TR_access_rights */
	HALT_ON_ERRORCOND(__vmx_vmread32(0x4824) == 0x00000008); /* guest_interruptibility */
	HALT_ON_ERRORCOND(__vmx_vmread32(0x4826) == 0x00000000); /* guest_activity_state */
	HALT_ON_ERRORCOND(__vmx_vmread32(0x482a) == 0x00000000); /* guest_SYSENTER_CS */
	HALT_ON_ERRORCOND(__vmx_vmread32(0x482e) == 0x00000000); /* guest_VMX_preemption_timer_value */
	HALT_ON_ERRORCOND(__vmx_vmread32(0x4c00) == 0x00000000); /* host_SYSENTER_CS */
	HALT_ON_ERRORCOND(__vmx_vmreadNW(0x6000) == 0x60000020); /* control_CR0_mask */
	HALT_ON_ERRORCOND(__vmx_vmreadNW(0x6002) == 0x00002000); /* control_CR4_mask */
	HALT_ON_ERRORCOND(__vmx_vmreadNW(0x6004) == 0x00000000); /* control_CR0_shadow */
	HALT_ON_ERRORCOND(__vmx_vmreadNW(0x6006) == 0x00000000); /* control_CR4_shadow */
	HALT_ON_ERRORCOND(__vmx_vmreadNW(0x6400) == 0x00000000); /* info_exit_qualification */
//	HALT_ON_ERRORCOND(__vmx_vmreadNW(0x640a) == 0x00000000); /* info_guest_linear_address */
	HALT_ON_ERRORCOND(__vmx_vmreadNW(0x6800) == 0x80000031); /* guest_CR0 */
	HALT_ON_ERRORCOND(__vmx_vmreadNW(0x6802) == 0x08b60000); /* guest_CR3 */
	HALT_ON_ERRORCOND(__vmx_vmreadNW(0x6804) == 0x00002020); /* guest_CR4 */
	HALT_ON_ERRORCOND(__vmx_vmreadNW(0x6806) == 0x00000000); /* guest_ES_base */
	HALT_ON_ERRORCOND(__vmx_vmreadNW(0x6808) == 0x00000000); /* guest_CS_base */
	HALT_ON_ERRORCOND(__vmx_vmreadNW(0x680a) == 0x00000000); /* guest_SS_base */
	HALT_ON_ERRORCOND(__vmx_vmreadNW(0x680c) == 0x00000000); /* guest_DS_base */
	HALT_ON_ERRORCOND(__vmx_vmreadNW(0x680e) == 0x00000000); /* guest_FS_base */
	HALT_ON_ERRORCOND(__vmx_vmreadNW(0x6810) == 0x00000000); /* guest_GS_base */
	HALT_ON_ERRORCOND(__vmx_vmreadNW(0x6812) == 0x00000000); /* guest_LDTR_base */
	HALT_ON_ERRORCOND(__vmx_vmreadNW(0x6814) == 0x00000000); /* guest_TR_base */
	HALT_ON_ERRORCOND(__vmx_vmreadNW(0x6816) == 0x082127d0); /* guest_GDTR_base */
	HALT_ON_ERRORCOND(__vmx_vmreadNW(0x6818) == 0x0820e010); /* guest_IDTR_base */
	HALT_ON_ERRORCOND(__vmx_vmreadNW(0x681a) == 0x00000400); /* guest_DR7 */
	HALT_ON_ERRORCOND(__vmx_vmreadNW(0x681c) == 0x08b8d000); /* guest_RSP */
	HALT_ON_ERRORCOND(__vmx_vmreadNW(0x681e) == 0x0820b7cf); /* guest_RIP */
	HALT_ON_ERRORCOND(__vmx_vmreadNW(0x6820) == 0x00000002); /* guest_RFLAGS */
	HALT_ON_ERRORCOND(__vmx_vmreadNW(0x6822) == 0x00000000); /* guest_pending_debug_x */
	HALT_ON_ERRORCOND(__vmx_vmreadNW(0x6824) == 0x00000000); /* guest_SYSENTER_ESP */
	HALT_ON_ERRORCOND(__vmx_vmreadNW(0x6826) == 0x00000000); /* guest_SYSENTER_EIP */
	HALT_ON_ERRORCOND(__vmx_vmreadNW(0x6c00) == vcpu->vmcs.host_CR0); /* host_CR0 */
	HALT_ON_ERRORCOND(__vmx_vmreadNW(0x6c02) == vcpu->vmcs.host_CR3); /* host_CR3 */
	HALT_ON_ERRORCOND(__vmx_vmreadNW(0x6c04) == 0x00042030); /* host_CR4 */
	HALT_ON_ERRORCOND(__vmx_vmreadNW(0x6c06) == 0x00000000); /* host_FS_base */
	HALT_ON_ERRORCOND(__vmx_vmreadNW(0x6c08) == 0x00000000); /* host_GS_base */
	HALT_ON_ERRORCOND(__vmx_vmreadNW(0x6c0a) == vcpu->vmcs.host_TR_base); /* host_TR_base */
	HALT_ON_ERRORCOND(__vmx_vmreadNW(0x6c0c) == vcpu->vmcs.host_GDTR_base); /* host_GDTR_base */
	HALT_ON_ERRORCOND(__vmx_vmreadNW(0x6c0e) == vcpu->vmcs.host_IDTR_base); /* host_IDTR_base */
	HALT_ON_ERRORCOND(__vmx_vmreadNW(0x6c10) == 0x00000000); /* host_SYSENTER_ESP */
	HALT_ON_ERRORCOND(__vmx_vmreadNW(0x6c12) == 0x00000000); /* host_SYSENTER_EIP */
	HALT_ON_ERRORCOND(__vmx_vmreadNW(0x6c14) == vcpu->vmcs.host_RSP); /* host_RSP */
	HALT_ON_ERRORCOND(__vmx_vmreadNW(0x6c16) == vcpu->vmcs.host_RIP); /* host_RIP */

//	xmhf_nested_arch_x86vmx_vmread_all(vcpu, "VMEXIT02");
	xmhf_smpguest_arch_x86vmx_unblock_nmi();	// TODO: hacking fix
	tmp = __vmx_vmread32(0x4824);
	if (cpu0212_done[vcpu->idx]++) {
		HALT_ON_ERRORCOND((tmp & (1 << 3)) != 0);
		if (vcpu->id == 0) {
			printf("hello! %d\n", cpu0212_done[vcpu->idx]);
		}
	} else {
		HALT_ON_ERRORCOND((tmp & (1 << 3)) != 0);
	}
	printf("CPU(0x%02x): nested vmexit ?\n", vcpu->id);
	/* Follow SDM to load host state */
	vcpu->vmcs.guest_DR7 = 0x400UL;
	vcpu->vmcs.guest_IA32_DEBUGCTL = 0ULL;
	vcpu->vmcs.guest_RFLAGS = (1UL << 1);
	/* Prepare VMRESUME to guest hypervisor */
	HALT_ON_ERRORCOND(__vmx_vmptrld(hva2spa((void *)vcpu->vmx_vmcs_vaddr)));

	tmp = __vmx_vmread32(0x4824);
	if (cpu0212_done[vcpu->idx] > 1) {
		HALT_ON_ERRORCOND((tmp & (1 << 3)) != 0);
	} else {
		HALT_ON_ERRORCOND((tmp & (1 << 3)) == 0);
		tmp |= (1 << 3);
		__vmx_vmwrite32(0x4824, tmp);
	}

	__vmx_vmwrite16(0x0000, 0x0001); /* control_vpid */
	__vmx_vmwrite16(0x0002, 0x0000); /* control_post_interrupt_notification_vec */
	__vmx_vmwrite16(0x0800, 0x0010); /* guest_ES_selector */
	__vmx_vmwrite16(0x0802, 0x0008); /* guest_CS_selector */
	__vmx_vmwrite16(0x0804, 0x0010); /* guest_SS_selector */
	__vmx_vmwrite16(0x0806, 0x0010); /* guest_DS_selector */
	__vmx_vmwrite16(0x0808, 0x0010); /* guest_FS_selector */
	__vmx_vmwrite16(0x080a, 0x0010); /* guest_GS_selector */
	__vmx_vmwrite16(0x080c, 0x0000); /* guest_LDTR_selector */
	__vmx_vmwrite16(0x080e, 0x0018); /* guest_TR_selector */
	__vmx_vmwrite16(0x0810, 0x0000); /* guest_interrupt_status */
	__vmx_vmwrite16(0x0812, 0x0000); /* guest_PML_index */
	__vmx_vmwrite16(0x0c00, 0x0010); /* host_ES_selector */
	__vmx_vmwrite16(0x0c02, 0x0008); /* host_CS_selector */
	__vmx_vmwrite16(0x0c04, 0x0010); /* host_SS_selector */
	__vmx_vmwrite16(0x0c06, 0x0010); /* host_DS_selector */
	__vmx_vmwrite16(0x0c08, 0x0010); /* host_FS_selector */
	__vmx_vmwrite16(0x0c0a, 0x0010); /* host_GS_selector */
	__vmx_vmwrite16(0x0c0c, 0x0018); /* host_TR_selector */
	__vmx_vmwrite64(0x2000, 0x000000001c77c000); /* control_IO_BitmapA_address */
	__vmx_vmwrite64(0x2002, 0x000000001c77d000); /* control_IO_BitmapB_address */
	__vmx_vmwrite64(0x2004, 0x0000000000000000); /* control_MSR_Bitmaps_address */
	__vmx_vmwrite64(0x2006, vcpu->vmcs.control_VM_exit_MSR_store_address); /* control_VM_exit_MSR_store_address */
	__vmx_vmwrite64(0x2008, vcpu->vmcs.control_VM_exit_MSR_load_address); /* control_VM_exit_MSR_load_address */
	__vmx_vmwrite64(0x200a, vcpu->vmcs.control_VM_entry_MSR_load_address); /* control_VM_entry_MSR_load_address */
	__vmx_vmwrite64(0x200e, 0x0000000000000000); /* control_PML_address */
	__vmx_vmwrite64(0x2010, 0x0000000000000000); /* control_TSC_offset */
	__vmx_vmwrite64(0x2012, 0x0000000000000000); /* control_virtual_APIC_address */
	__vmx_vmwrite64(0x2014, 0x0000000000000000); /* control_APIC_access_address */
	__vmx_vmwrite64(0x2016, 0x0000000000000000); /* control_posted_interrupt_desc_address */
	__vmx_vmwrite64(0x2018, 0x0000000000000000); /* control_VM_function_controls */
	__vmx_vmwrite64(0x201a, vcpu->vmcs.control_EPT_pointer); /* control_EPT_pointer */
	__vmx_vmwrite64(0x201c, 0x0000000000000000); /* control_EOI_exit_bitmap_0 */
	__vmx_vmwrite64(0x201e, 0x0000000000000000); /* control_EOI_exit_bitmap_1 */
	__vmx_vmwrite64(0x2020, 0x0000000000000000); /* control_EOI_exit_bitmap_2 */
	__vmx_vmwrite64(0x2022, 0x0000000000000000); /* control_EOI_exit_bitmap_3 */
	__vmx_vmwrite64(0x2024, 0x0000000000000000); /* control_EPTP_list_address */
	__vmx_vmwrite64(0x2026, 0x0000000000000000); /* control_VMREAD_bitmap_address */
	__vmx_vmwrite64(0x2028, 0x0000000000000000); /* control_VMWRITE_bitmap_address */
	__vmx_vmwrite64(0x202c, 0x0000000000000000); /* control_XSS_exiting_bitmap */
	__vmx_vmwrite64(0x202e, 0x0000000000000000); /* control_ENCLS_exiting_bitmap */
	__vmx_vmwrite64(0x2032, 0x0000000000000000); /* control_TSC_multiplier */
	__vmx_vmwrite64(0x2400, 0x00000000fee00300); /* guest_paddr */
	__vmx_vmwrite64(0x2800, 0xffffffffffffffff); /* guest_VMCS_link_pointer */
	__vmx_vmwrite64(0x2802, 0x0000000000000000); /* guest_IA32_DEBUGCTL */
	__vmx_vmwrite64(0x2804, 0x0000000000000000); /* guest_IA32_PAT */
	__vmx_vmwrite64(0x2806, 0x0000000000000000); /* guest_IA32_EFER */
	__vmx_vmwrite64(0x2808, 0x0000000000000000); /* guest_IA32_PERF_GLOBAL_CTRL */
	__vmx_vmwrite64(0x280a, 0x0000000008b61001); /* guest_PDPTE0 */
	__vmx_vmwrite64(0x280c, 0x0000000008b62001); /* guest_PDPTE1 */
	__vmx_vmwrite64(0x280e, 0x0000000008b63001); /* guest_PDPTE2 */
	__vmx_vmwrite64(0x2810, 0x0000000008b64001); /* guest_PDPTE3 */
	__vmx_vmwrite64(0x2812, 0x0000000000000000); /* guest_IA32_BNDCFGS */
	__vmx_vmwrite64(0x2c00, 0x0000000000000000); /* host_IA32_PAT */
	__vmx_vmwrite64(0x2c02, 0x0000000000000000); /* host_IA32_EFER */
	__vmx_vmwrite64(0x2c04, 0x0000000000000000); /* host_IA32_PERF_GLOBAL_CTRL */
	__vmx_vmwrite32(0x4000, 0x0000003e); /* control_VMX_pin_based */
	__vmx_vmwrite32(0x4002, 0x86006172); /* control_VMX_cpu_based */
	__vmx_vmwrite32(0x4004, 0x00000000); /* control_exception_bitmap */
	__vmx_vmwrite32(0x4006, 0x00000000); /* control_pagefault_errorcode_mask */
	__vmx_vmwrite32(0x4008, 0x00000000); /* control_pagefault_errorcode_match */
	__vmx_vmwrite32(0x400a, 0x00000000); /* control_CR3_target_count */
	__vmx_vmwrite32(0x400c, 0x00036dfb); /* control_VM_exit_controls */
	__vmx_vmwrite32(0x400e, 0x00000003); /* control_VM_exit_MSR_store_count */
	__vmx_vmwrite32(0x4010, 0x00000003); /* control_VM_exit_MSR_load_count */
	__vmx_vmwrite32(0x4012, 0x000011fb); /* control_VM_entry_controls */
	__vmx_vmwrite32(0x4014, 0x00000003); /* control_VM_entry_MSR_load_count */
	__vmx_vmwrite32(0x4016, 0x00000000); /* control_VM_entry_interruption_information */
	__vmx_vmwrite32(0x4018, 0x00000000); /* control_VM_entry_exception_errorcode */
	__vmx_vmwrite32(0x401a, 0x00000000); /* control_VM_entry_instruction_length */
	__vmx_vmwrite32(0x401c, 0x00000000); /* control_Task_PRivilege_Threshold */
	__vmx_vmwrite32(0x401e, 0x000010aa); /* control_VMX_seccpu_based */
	__vmx_vmwrite32(0x4400, 0x0000000c); /* info_vminstr_error */
	__vmx_vmwrite32(0x4402, 0x00000018); /* info_vmexit_reason */
	__vmx_vmwrite32(0x4404, 0x00000000); /* info_vmexit_interrupt_information */
	__vmx_vmwrite32(0x4406, 0x00000000); /* info_vmexit_interrupt_error_code */
	__vmx_vmwrite32(0x4408, 0x00000000); /* info_IDT_vectoring_information */
	__vmx_vmwrite32(0x440a, 0x00000000); /* info_IDT_vectoring_error_code */
	__vmx_vmwrite32(0x440c, 0x00000003); /* info_vmexit_instruction_length */
	__vmx_vmwrite32(0x440e, 0x00000000); /* info_vmx_instruction_information */
	__vmx_vmwrite32(0x4800, 0xffffffff); /* guest_ES_limit */
	__vmx_vmwrite32(0x4802, 0xffffffff); /* guest_CS_limit */
	__vmx_vmwrite32(0x4804, 0xffffffff); /* guest_SS_limit */
	__vmx_vmwrite32(0x4806, 0xffffffff); /* guest_DS_limit */
	__vmx_vmwrite32(0x4808, 0xffffffff); /* guest_FS_limit */
	__vmx_vmwrite32(0x480a, 0xffffffff); /* guest_GS_limit */
	__vmx_vmwrite32(0x480c, 0x00000000); /* guest_LDTR_limit */
	__vmx_vmwrite32(0x480e, 0x00000067); /* guest_TR_limit */
	__vmx_vmwrite32(0x4810, 0x0000001f); /* guest_GDTR_limit */
	__vmx_vmwrite32(0x4812, 0x000003ff); /* guest_IDTR_limit */
	__vmx_vmwrite32(0x4814, 0x0000c093); /* guest_ES_access_rights */
	__vmx_vmwrite32(0x4816, 0x0000c09b); /* guest_CS_access_rights */
	__vmx_vmwrite32(0x4818, 0x0000c093); /* guest_SS_access_rights */
	__vmx_vmwrite32(0x481a, 0x0000c093); /* guest_DS_access_rights */
	__vmx_vmwrite32(0x481c, 0x0000c093); /* guest_FS_access_rights */
	__vmx_vmwrite32(0x481e, 0x0000c093); /* guest_GS_access_rights */
	__vmx_vmwrite32(0x4820, 0x00010000); /* guest_LDTR_access_rights */
	__vmx_vmwrite32(0x4822, 0x0000008b); /* guest_TR_access_rights */
	__vmx_vmwrite32(0x4824, 0x00000008); /* guest_interruptibility */
	__vmx_vmwrite32(0x4826, 0x00000000); /* guest_activity_state */
	__vmx_vmwrite32(0x482a, 0x00000000); /* guest_SYSENTER_CS */
	__vmx_vmwrite32(0x482e, 0x00000000); /* guest_VMX_preemption_timer_value */
	__vmx_vmwrite32(0x4c00, 0x00000000); /* host_SYSENTER_CS */
	__vmx_vmwriteNW(0x6000, 0x60000020); /* control_CR0_mask */
	__vmx_vmwriteNW(0x6002, 0x00002000); /* control_CR4_mask */
	__vmx_vmwriteNW(0x6004, 0x80000035); /* control_CR0_shadow */
	__vmx_vmwriteNW(0x6006, 0x00002030); /* control_CR4_shadow */
	__vmx_vmwriteNW(0x6400, 0x00000000); /* info_exit_qualification */
	__vmx_vmwriteNW(0x640a, 0x00000000); /* info_guest_linear_address */
	__vmx_vmwriteNW(0x6800, 0x80000035); /* guest_CR0 */
	__vmx_vmwriteNW(0x6802, 0x08b60000); /* guest_CR3 */
	__vmx_vmwriteNW(0x6804, 0x00002030); /* guest_CR4 */
	__vmx_vmwriteNW(0x6806, 0x00000000); /* guest_ES_base */
	__vmx_vmwriteNW(0x6808, 0x00000000); /* guest_CS_base */
	__vmx_vmwriteNW(0x680a, 0x00000000); /* guest_SS_base */
	__vmx_vmwriteNW(0x680c, 0x00000000); /* guest_DS_base */
	__vmx_vmwriteNW(0x680e, 0x00000000); /* guest_FS_base */
	__vmx_vmwriteNW(0x6810, 0x00000000); /* guest_GS_base */
	__vmx_vmwriteNW(0x6812, 0x00000000); /* guest_LDTR_base */
	__vmx_vmwriteNW(0x6814, 0x08211240); /* guest_TR_base */
	__vmx_vmwriteNW(0x6816, 0x082127d0); /* guest_GDTR_base */
	__vmx_vmwriteNW(0x6818, 0x0820e010); /* guest_IDTR_base */
	__vmx_vmwriteNW(0x681a, 0x00000400); /* guest_DR7 */
	__vmx_vmwriteNW(0x681c, 0x08bb0ffc); /* guest_RSP */
	__vmx_vmwriteNW(0x681e, 0x0820596f); /* guest_RIP */
	__vmx_vmwriteNW(0x6820, 0x00000002); /* guest_RFLAGS */
	__vmx_vmwriteNW(0x6822, 0x00000000); /* guest_pending_debug_x */
	__vmx_vmwriteNW(0x6824, 0x00000000); /* guest_SYSENTER_ESP */
	__vmx_vmwriteNW(0x6826, 0x00000000); /* guest_SYSENTER_EIP */
	__vmx_vmwriteNW(0x6c00, vcpu->vmcs.host_CR0); /* host_CR0 */
	__vmx_vmwriteNW(0x6c02, vcpu->vmcs.host_CR3); /* host_CR3 */
	__vmx_vmwriteNW(0x6c04, 0x00042030); /* host_CR4 */
	__vmx_vmwriteNW(0x6c06, 0x00000000); /* host_FS_base */
	__vmx_vmwriteNW(0x6c08, 0x00000000); /* host_GS_base */
	__vmx_vmwriteNW(0x6c0a, vcpu->vmcs.host_TR_base); /* host_TR_base */
	__vmx_vmwriteNW(0x6c0c, vcpu->vmcs.host_GDTR_base); /* host_GDTR_base */
	__vmx_vmwriteNW(0x6c0e, vcpu->vmcs.host_IDTR_base); /* host_IDTR_base */
	__vmx_vmwriteNW(0x6c10, 0x00000000); /* host_SYSENTER_ESP */
	__vmx_vmwriteNW(0x6c12, 0x00000000); /* host_SYSENTER_EIP */
	__vmx_vmwriteNW(0x6c14, vcpu->vmcs.host_RSP); /* host_RSP */
	__vmx_vmwriteNW(0x6c16, vcpu->vmcs.host_RIP); /* host_RIP */

//	xmhf_nested_arch_x86vmx_vmread_all(vcpu, "VMEXIT01");

	// TODO: handle vcpu->vmx_guest_inject_nmi?
	vcpu->vmx_nested_is_vmx_root_operation = 1;
	__vmx_vmentry_vmresume(r);
}

void xmhf_nested_arch_x86vmx_handle_vmclear(VCPU * vcpu, struct regs *r)
{
	if (_vmx_nested_check_ud(vcpu, 0)) {
		_vmx_inject_exception(vcpu, CPU_EXCEPTION_UD, 0, 0);
	} else if (!vcpu->vmx_nested_is_vmx_root_operation) {
		/*
		 * Guest hypervisor is likely performing nested virtualization.
		 * This case should be handled in
		 * xmhf_parteventhub_arch_x86vmx_intercept_handler(). So panic if we
		 * end up here.
		 */
		HALT_ON_ERRORCOND(0 && "Nested vmexit should be handled elsewhere");
	} else if (_vmx_guest_get_cpl(vcpu) > 0) {
		_vmx_inject_exception(vcpu, CPU_EXCEPTION_GP, 1, 0);
	} else {
		gva_t addr = _vmx_decode_m64(vcpu, r);
		gpa_t vmcs_ptr;
		guestmem_hptw_ctx_pair_t ctx_pair;
		guestmem_init(vcpu, &ctx_pair);
		guestmem_copy_gv2h(&ctx_pair, 0, &vmcs_ptr, addr, sizeof(vmcs_ptr));
		if (!PA_PAGE_ALIGNED_4K(vmcs_ptr) ||
			!_vmx_check_physical_addr_width(vcpu, vmcs_ptr)) {
			_vmx_nested_vm_fail(vcpu, VM_INST_ERRNO_VMCLEAR_INVALID_PHY_ADDR);
		} else if (vmcs_ptr == vcpu->vmx_nested_vmxon_pointer) {
			_vmx_nested_vm_fail(vcpu, VM_INST_ERRNO_VMCLEAR_VMXON_PTR);
		} else {
			/*
			 * We do not distinguish a VMCS that is "Inactive, Not Current,
			 * Clear" from a VMCS that is "Inactive" with other states. This is
			 * because we do not track inactive guests. As a result, we expect
			 * guest hypervisors to VMCLEAR before and after using a VMCS.
			 * However, we cannot check whether the GUEST does so.
			 *
			 * SDM says that the launch state of VMCS should be set to clear.
			 * Here, we remove the VMCS from the list of active VMCS's we track.
			 */
			vmcs12_info_t *vmcs12_info = find_active_vmcs12(vcpu, vmcs_ptr);
			if (vmcs12_info != NULL) {
				vmcs12_info->vmcs12_ptr = CUR_VMCS_PTR_INVALID;
			}
			guestmem_copy_h2gp(&ctx_pair, 0, vmcs_ptr, blank_page,
							   PAGE_SIZE_4K);
			if (vmcs_ptr == vcpu->vmx_nested_current_vmcs_pointer) {
				vcpu->vmx_nested_current_vmcs_pointer = CUR_VMCS_PTR_INVALID;
			}
			_vmx_nested_vm_succeed(vcpu);
		}
		vcpu->vmcs.guest_RIP += vcpu->vmcs.info_vmexit_instruction_length;
	}
}

void xmhf_nested_arch_x86vmx_handle_vmlaunch_vmresume(VCPU * vcpu,
													  struct regs *r,
													  int is_vmresume)
{
	if (_vmx_nested_check_ud(vcpu, 0)) {
		_vmx_inject_exception(vcpu, CPU_EXCEPTION_UD, 0, 0);
	} else if (!vcpu->vmx_nested_is_vmx_root_operation) {
		/*
		 * Guest hypervisor is likely performing nested virtualization.
		 * This case should be handled in
		 * xmhf_parteventhub_arch_x86vmx_intercept_handler(). So panic if we
		 * end up here.
		 */
		HALT_ON_ERRORCOND(0 && "Nested vmexit should be handled elsewhere");
	} else if (_vmx_guest_get_cpl(vcpu) > 0) {
		_vmx_inject_exception(vcpu, CPU_EXCEPTION_GP, 1, 0);
	} else {
		if (vcpu->vmx_nested_current_vmcs_pointer == CUR_VMCS_PTR_INVALID) {
			_vmx_nested_vm_fail_invalid(vcpu);
		} else if (vcpu->vmcs.guest_interruptibility & (1U << 1)) {
			/* Blocking by MOV SS */
			_vmx_nested_vm_fail_valid
				(vcpu, VM_INST_ERRNO_VMENTRY_EVENT_BLOCK_MOVSS);
		} else {
			vmcs12_info_t *vmcs12_info = find_current_vmcs12(vcpu);
			u32 error_number;
			if (!is_vmresume && vmcs12_info->launched) {
				error_number = VM_INST_ERRNO_VMLAUNCH_NONCLEAR_VMCS;
			} else if (is_vmresume && !vmcs12_info->launched) {
				error_number = VM_INST_ERRNO_VMRESUME_NONLAUNCH_VMCS;
			} else {
				error_number = _vmx_vmentry(vcpu, vmcs12_info, r);
			}
			_vmx_nested_vm_fail_valid(vcpu, error_number);
		}
		vcpu->vmcs.guest_RIP += vcpu->vmcs.info_vmexit_instruction_length;
	}
}

void xmhf_nested_arch_x86vmx_handle_vmptrld(VCPU * vcpu, struct regs *r)
{
	if (_vmx_nested_check_ud(vcpu, 0)) {
		_vmx_inject_exception(vcpu, CPU_EXCEPTION_UD, 0, 0);
	} else if (!vcpu->vmx_nested_is_vmx_root_operation) {
		/*
		 * Guest hypervisor is likely performing nested virtualization.
		 * This case should be handled in
		 * xmhf_parteventhub_arch_x86vmx_intercept_handler(). So panic if we
		 * end up here.
		 */
		HALT_ON_ERRORCOND(0 && "Nested vmexit should be handled elsewhere");
	} else if (_vmx_guest_get_cpl(vcpu) > 0) {
		_vmx_inject_exception(vcpu, CPU_EXCEPTION_GP, 1, 0);
	} else {
		gva_t addr = _vmx_decode_m64(vcpu, r);
		gpa_t vmcs_ptr;
		guestmem_hptw_ctx_pair_t ctx_pair;
		guestmem_init(vcpu, &ctx_pair);
		guestmem_copy_gv2h(&ctx_pair, 0, &vmcs_ptr, addr, sizeof(vmcs_ptr));
		if (!PA_PAGE_ALIGNED_4K(vmcs_ptr) ||
			!_vmx_check_physical_addr_width(vcpu, vmcs_ptr)) {
			_vmx_nested_vm_fail(vcpu, VM_INST_ERRNO_VMPTRLD_INVALID_PHY_ADDR);
		} else if (vmcs_ptr == vcpu->vmx_nested_vmxon_pointer) {
			_vmx_nested_vm_fail(vcpu, VM_INST_ERRNO_VMPTRLD_VMXON_PTR);
		} else {
			u32 rev;
			u64 basic_msr = vcpu->vmx_msrs[INDEX_IA32_VMX_BASIC_MSR];
			guestmem_copy_gp2h(&ctx_pair, 0, &rev, vmcs_ptr, sizeof(rev));
			/* Note: Currently does not support 1-setting of "VMCS shadowing" */
			if ((rev & (1U << 31)) || (rev != ((u32) basic_msr & 0x7fffffffU))) {
				_vmx_nested_vm_fail
					(vcpu, VM_INST_ERRNO_VMPTRLD_WITH_INCORRECT_VMCS_REV_ID);
			} else {
				vmcs12_info_t *vmcs12_info = find_active_vmcs12(vcpu, vmcs_ptr);
				if (vmcs12_info == NULL) {
					new_active_vmcs12(vcpu, vmcs_ptr, rev);
				}
				vcpu->vmx_nested_current_vmcs_pointer = vmcs_ptr;
				_vmx_nested_vm_succeed(vcpu);
			}
		}
		vcpu->vmcs.guest_RIP += vcpu->vmcs.info_vmexit_instruction_length;
	}
}

void xmhf_nested_arch_x86vmx_handle_vmptrst(VCPU * vcpu, struct regs *r)
{
	printf("CPU(0x%02x): %s(): r=%p\n", vcpu->id, __func__, r);
	HALT_ON_ERRORCOND(0 && "Not implemented");
}

void xmhf_nested_arch_x86vmx_handle_vmread(VCPU * vcpu, struct regs *r)
{
	if (_vmx_nested_check_ud(vcpu, 0)) {
		_vmx_inject_exception(vcpu, CPU_EXCEPTION_UD, 0, 0);
	} else if (!vcpu->vmx_nested_is_vmx_root_operation) {
		/*
		 * Guest hypervisor is likely performing nested virtualization.
		 * This case should be handled in
		 * xmhf_parteventhub_arch_x86vmx_intercept_handler(). So panic if we
		 * end up here.
		 */
		HALT_ON_ERRORCOND(0 && "Nested vmexit should be handled elsewhere");
	} else if (_vmx_guest_get_cpl(vcpu) > 0) {
		_vmx_inject_exception(vcpu, CPU_EXCEPTION_GP, 1, 0);
	} else {
		if (vcpu->vmx_nested_current_vmcs_pointer == CUR_VMCS_PTR_INVALID) {
			/* Note: Currently does not support 1-setting of "VMCS shadowing" */
			_vmx_nested_vm_fail_invalid(vcpu);
		} else {
			ulong_t encoding;
			uintptr_t pvalue;
			int value_mem_reg;
			size_t size = _vmx_decode_vmread_vmwrite(vcpu, r, 1, &encoding,
													 &pvalue, &value_mem_reg);
			size_t offset = xmhf_nested_arch_x86vmx_vmcs_field_find(encoding);
			if (offset == (size_t)(-1)) {
				_vmx_nested_vm_fail_valid
					(vcpu, VM_INST_ERRNO_VMRDWR_UNSUPP_VMCS_COMP);
			} else {
				/* Note: Currently does not support VMCS shadowing */
				vmcs12_info_t *vmcs12_info = find_current_vmcs12(vcpu);
				ulong_t value = xmhf_nested_arch_x86vmx_vmcs_read
					(&vmcs12_info->vmcs12_value, offset, size);
				if (value_mem_reg) {
					memcpy((void *)pvalue, &value, size);
				} else {
					guestmem_hptw_ctx_pair_t ctx_pair;
					guestmem_init(vcpu, &ctx_pair);
					guestmem_copy_h2gv(&ctx_pair, 0, pvalue, &value, size);
				}
				_vmx_nested_vm_succeed(vcpu);
			}
		}
		vcpu->vmcs.guest_RIP += vcpu->vmcs.info_vmexit_instruction_length;
	}
}

void xmhf_nested_arch_x86vmx_handle_vmwrite(VCPU * vcpu, struct regs *r)
{
	if (_vmx_nested_check_ud(vcpu, 0)) {
		_vmx_inject_exception(vcpu, CPU_EXCEPTION_UD, 0, 0);
	} else if (!vcpu->vmx_nested_is_vmx_root_operation) {
		/* Note: Currently does not support 1-setting of "VMCS shadowing" */
		/*
		 * Guest hypervisor is likely performing nested virtualization.
		 * This case should be handled in
		 * xmhf_parteventhub_arch_x86vmx_intercept_handler(). So panic if we
		 * end up here.
		 */
		HALT_ON_ERRORCOND(0 && "Nested vmexit should be handled elsewhere");
	} else if (_vmx_guest_get_cpl(vcpu) > 0) {
		_vmx_inject_exception(vcpu, CPU_EXCEPTION_GP, 1, 0);
	} else {
		if (vcpu->vmx_nested_current_vmcs_pointer == CUR_VMCS_PTR_INVALID) {
			/* Note: Currently does not support 1-setting of "VMCS shadowing" */
			_vmx_nested_vm_fail_invalid(vcpu);
		} else {
			ulong_t encoding;
			uintptr_t pvalue;
			int value_mem_reg;
			size_t size = _vmx_decode_vmread_vmwrite(vcpu, r, 1, &encoding,
													 &pvalue, &value_mem_reg);
			size_t offset = xmhf_nested_arch_x86vmx_vmcs_field_find(encoding);
			if (offset == (size_t)(-1)) {
				_vmx_nested_vm_fail_valid
					(vcpu, VM_INST_ERRNO_VMRDWR_UNSUPP_VMCS_COMP);
			} else if (!xmhf_nested_arch_x86vmx_vmcs_writable(offset)) {
				/*
				 * Note: currently not supporting writing to VM-exit
				 * information field
				 */
				_vmx_nested_vm_fail_valid
					(vcpu, VM_INST_ERRNO_VMWRITE_RO_VMCS_COMP);
			} else {
				/* Note: Currently does not support VMCS shadowing */
				vmcs12_info_t *vmcs12_info = find_current_vmcs12(vcpu);
				ulong_t value = 0;
				if (value_mem_reg) {
					memcpy(&value, (void *)pvalue, size);
				} else {
					guestmem_hptw_ctx_pair_t ctx_pair;
					guestmem_init(vcpu, &ctx_pair);
					guestmem_copy_gv2h(&ctx_pair, 0, &value, pvalue, size);
				}
				xmhf_nested_arch_x86vmx_vmcs_write(&vmcs12_info->vmcs12_value,
												   offset, value, size);
				_vmx_nested_vm_succeed(vcpu);
			}
		}
		vcpu->vmcs.guest_RIP += vcpu->vmcs.info_vmexit_instruction_length;
	}
}

void xmhf_nested_arch_x86vmx_handle_vmxoff(VCPU * vcpu, struct regs *r)
{
	printf("CPU(0x%02x): %s(): r=%p\n", vcpu->id, __func__, r);
	HALT_ON_ERRORCOND(0 && "Not implemented");
}

void xmhf_nested_arch_x86vmx_handle_vmxon(VCPU * vcpu, struct regs *r)
{
	if (_vmx_nested_check_ud(vcpu, 1)) {
		_vmx_inject_exception(vcpu, CPU_EXCEPTION_UD, 0, 0);
	} else if (!vcpu->vmx_nested_is_vmx_operation) {
		u64 vcr0 = _vmx_guest_get_guest_cr0(vcpu);
		u64 vcr4 = _vmx_guest_get_guest_cr4(vcpu);
		/*
		 * Note: A20M mode is not tested here.
		 *
		 * Note: IA32_FEATURE_CONTROL is not tested here, because running XMHF
		 * already requires entering VMX operation in physical CPU. This may
		 * need to be updated if IA32_FEATURE_CONTROL is virtualized.
		 */
		if (_vmx_guest_get_cpl(vcpu) > 0 ||
			(~vcr0 & vcpu->vmx_msrs[INDEX_IA32_VMX_CR0_FIXED0_MSR]) != 0 ||
			(vcr0 & ~vcpu->vmx_msrs[INDEX_IA32_VMX_CR0_FIXED1_MSR]) != 0 ||
			(~vcr4 & vcpu->vmx_msrs[INDEX_IA32_VMX_CR4_FIXED0_MSR]) != 0 ||
			(vcr4 & ~vcpu->vmx_msrs[INDEX_IA32_VMX_CR4_FIXED1_MSR]) != 0) {
			_vmx_inject_exception(vcpu, CPU_EXCEPTION_GP, 1, 0);
		} else {
			gva_t addr = _vmx_decode_m64(vcpu, r);
			gpa_t vmxon_ptr;
			guestmem_hptw_ctx_pair_t ctx_pair;
			guestmem_init(vcpu, &ctx_pair);
			guestmem_copy_gv2h(&ctx_pair, 0, &vmxon_ptr, addr,
							   sizeof(vmxon_ptr));
			if (!PA_PAGE_ALIGNED_4K(vmxon_ptr)
				|| !_vmx_check_physical_addr_width(vcpu, vmxon_ptr)) {
				_vmx_nested_vm_fail_invalid(vcpu);
			} else {
				u32 rev;
				u64 basic_msr = vcpu->vmx_msrs[INDEX_IA32_VMX_BASIC_MSR];
				guestmem_copy_gp2h(&ctx_pair, 0, &rev, vmxon_ptr, sizeof(rev));
				if ((rev & (1U << 31)) ||
					(rev != ((u32) basic_msr & 0x7fffffffU))) {
					_vmx_nested_vm_fail_invalid(vcpu);
				} else {
					vcpu->vmx_nested_is_vmx_operation = 1;
					vcpu->vmx_nested_vmxon_pointer = vmxon_ptr;
					vcpu->vmx_nested_is_vmx_root_operation = 1;
					vcpu->vmx_nested_current_vmcs_pointer =
						CUR_VMCS_PTR_INVALID;
					active_vmcs12_array_init(vcpu);
					_vmx_nested_vm_succeed(vcpu);
				}
			}
			vcpu->vmcs.guest_RIP += vcpu->vmcs.info_vmexit_instruction_length;
		}
	} else {
		/* This may happen if guest tries nested virtualization */
		printf("Not implemented, HALT!\n");
		HALT();
	}
}
