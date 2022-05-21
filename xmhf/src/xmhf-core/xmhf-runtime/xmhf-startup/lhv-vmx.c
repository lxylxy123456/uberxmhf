#include <xmhf.h>
#include <lhv.h>

#define MAX_GUESTS 4

static u8 all_vmxon_region[MAX_VCPU_ENTRIES][PAGE_SIZE_4K]
__attribute__(( section(".bss.palign_data") ));

static u8 all_vmcs[MAX_VCPU_ENTRIES][MAX_GUESTS][PAGE_SIZE_4K]
__attribute__(( section(".bss.palign_data") ));

extern u32 x_gdt_start[];

static void lhv_vmx_vmcs_init(VCPU *vcpu)
{
	// From vmx_initunrestrictedguestVMCS

	write_cr0(read_cr0() | CR0_NE);

	vmcs_vmwrite(vcpu, VMCS_host_CR0, read_cr0());
	vmcs_vmwrite(vcpu, VMCS_host_CR4, read_cr4());
	vmcs_vmwrite(vcpu, VMCS_host_CR3, read_cr3());
	vmcs_vmwrite(vcpu, VMCS_host_CS_selector, read_segreg_cs());
	vmcs_vmwrite(vcpu, VMCS_host_DS_selector, read_segreg_ds());
	vmcs_vmwrite(vcpu, VMCS_host_ES_selector, read_segreg_es());
	vmcs_vmwrite(vcpu, VMCS_host_FS_selector, read_segreg_fs());
	vmcs_vmwrite(vcpu, VMCS_host_GS_selector, read_segreg_gs());
	vmcs_vmwrite(vcpu, VMCS_host_SS_selector, read_segreg_ss());
	vmcs_vmwrite(vcpu, VMCS_host_TR_selector, read_tr_sel());
	vmcs_vmwrite(vcpu, VMCS_host_GDTR_base, (u64)(hva_t)x_gdt_start);
	vmcs_vmwrite(vcpu, VMCS_host_IDTR_base,
					(u64)(hva_t)xmhf_xcphandler_get_idt_start());
	vmcs_vmwrite(vcpu, VMCS_host_TR_base, (u64)(hva_t)g_runtime_TSS);
	vmcs_vmwrite(vcpu, VMCS_host_RIP, (u64)(hva_t)vmexit_asm);

	//store vcpu at TOS
#ifdef __AMD64__
	vcpu->rsp = vcpu->rsp - sizeof(hva_t);
	*(hva_t *)vcpu->rsp = (hva_t)vcpu;
	vmcs_vmwrite(vcpu, VMCS_host_RSP, (u64)vcpu->rsp);
#elif defined(__I386__)
	vcpu->esp = vcpu->esp - sizeof(hva_t);
	*(hva_t *)vcpu->esp = (hva_t)vcpu;
	vmcs_vmwrite(vcpu, VMCS_host_RSP, (u64)vcpu->esp);
#else /* !defined(__I386__) && !defined(__AMD64__) */
    #error "Unsupported Arch"
#endif /* !defined(__I386__) && !defined(__AMD64__) */

	vmcs_vmwrite(vcpu, VMCS_host_SYSENTER_CS, rdmsr64(IA32_SYSENTER_CS_MSR));
	vmcs_vmwrite(vcpu, VMCS_host_SYSENTER_ESP, rdmsr64(IA32_SYSENTER_ESP_MSR));
	vmcs_vmwrite(vcpu, VMCS_host_SYSENTER_EIP, rdmsr64(IA32_SYSENTER_EIP_MSR));
	vmcs_vmwrite(vcpu, VMCS_host_FS_base, rdmsr64(IA32_MSR_FS_BASE));
	vmcs_vmwrite(vcpu, VMCS_host_GS_base, rdmsr64(IA32_MSR_GS_BASE));

	//setup default VMX controls
	vmcs_vmwrite(vcpu, VMCS_control_VMX_pin_based,
				vcpu->vmx_msrs[INDEX_IA32_VMX_PINBASED_CTLS_MSR]);
	//activate secondary processor controls
	vmcs_vmwrite(vcpu, VMCS_control_VMX_cpu_based,
				vcpu->vmx_msrs[INDEX_IA32_VMX_PROCBASED_CTLS_MSR] | (1 << 31));
	vmcs_vmwrite(vcpu, VMCS_control_VM_exit_controls,
				vcpu->vmx_msrs[INDEX_IA32_VMX_EXIT_CTLS_MSR]);
	vmcs_vmwrite(vcpu, VMCS_control_VM_entry_controls,
				vcpu->vmx_msrs[INDEX_IA32_VMX_ENTRY_CTLS_MSR]);
	//TODO: enable unrestricted guest using ` | (1 << 7)`
	vmcs_vmwrite(vcpu, VMCS_control_VMX_seccpu_based,
				vcpu->vmx_msrs[INDEX_IA32_VMX_PROCBASED_CTLS2_MSR]);

#ifdef __AMD64__
	/*
	 * For amd64, set the Host address-space size (bit 9) in
	 * control_VM_exit_controls. First check whether setting this bit is
	 * allowed through bit (9 + 32) in the MSR.
	 */
	HALT_ON_ERRORCOND(vcpu->vmx_msrs[INDEX_IA32_VMX_EXIT_CTLS_MSR] & (1UL << (9 + 32)));
	{
		u64 control_VM_exit_controls = vmcs_vmread(vcpu, VMCS_control_VM_exit_controls);
		control_VM_exit_controls |= (1UL << 9);
		vmcs_vmwrite(vcpu, VMCS_control_VM_exit_controls, control_VM_exit_controls);
	}
#elif !defined(__I386__)
    #error "Unsupported Arch"
#endif /* !defined(__I386__) */

	//Critical MSR load/store
	vmcs_vmwrite(vcpu, VMCS_control_VM_exit_MSR_load_count, 0);
	vmcs_vmwrite(vcpu, VMCS_control_VM_entry_MSR_load_count, 0);
	vmcs_vmwrite(vcpu, VMCS_control_VM_exit_MSR_store_count, 0);

	vmcs_vmwrite(vcpu, VMCS_control_pagefault_errorcode_mask, 0);
	vmcs_vmwrite(vcpu, VMCS_control_pagefault_errorcode_match, 0);
	vmcs_vmwrite(vcpu, VMCS_control_exception_bitmap, 0);
	vmcs_vmwrite(vcpu, VMCS_control_CR3_target_count, 0);

	//setup guest state
	//CR0, real-mode, PE and PG bits cleared, set ET bit
	{
		ulong_t cr0 = vcpu->vmx_msrs[INDEX_IA32_VMX_CR0_FIXED0_MSR];
		cr0 |= CR0_ET;
		vmcs_vmwrite(vcpu, VMCS_guest_CR0, cr0);
	}
	//CR4, required bits set (usually VMX enabled bit)
	{
		ulong_t cr4 = vcpu->vmx_msrs[INDEX_IA32_VMX_CR4_FIXED0_MSR];
		cr4 |= CR4_PAE;
		vmcs_vmwrite(vcpu, VMCS_guest_CR4, cr4);
	}
	//CR3 set to 0, does not matter
	vmcs_vmwrite(vcpu, VMCS_guest_CR3, read_cr3());
	//IDTR
	vmcs_vmwrite(vcpu, VMCS_guest_IDTR_base,
				vmcs_vmread(vcpu, VMCS_host_IDTR_base));
	vmcs_vmwrite(vcpu, VMCS_guest_IDTR_limit, 0xffff);
	//GDTR
	vmcs_vmwrite(vcpu, VMCS_guest_GDTR_base,
				vmcs_vmread(vcpu, VMCS_host_GDTR_base));
	vmcs_vmwrite(vcpu, VMCS_guest_GDTR_limit, 0xffff);
	//LDTR, unusable
	vmcs_vmwrite(vcpu, VMCS_guest_LDTR_base, 0);
	vmcs_vmwrite(vcpu, VMCS_guest_LDTR_limit, 0x0);
	vmcs_vmwrite(vcpu, VMCS_guest_LDTR_selector, 0);
	vmcs_vmwrite(vcpu, VMCS_guest_LDTR_access_rights, 0x10000);
	// TR, should be usable for VMX to work, but not used by guest
	vmcs_vmwrite(vcpu, VMCS_guest_TR_base, 0);	// TODO
	vmcs_vmwrite(vcpu, VMCS_guest_TR_limit, 0);
	vmcs_vmwrite(vcpu, VMCS_guest_TR_selector, __TRSEL);
	vmcs_vmwrite(vcpu, VMCS_guest_TR_access_rights, 0x8b);
	//DR7
	vmcs_vmwrite(vcpu, VMCS_guest_DR7, 0x400);
	//RSP
	vmcs_vmwrite(vcpu, VMCS_guest_RSP, 0x0);
	//RIP
	vmcs_vmwrite(vcpu, VMCS_guest_CS_selector, __CS);
	vmcs_vmwrite(vcpu, VMCS_guest_CS_base, 0);
	vmcs_vmwrite(vcpu, VMCS_guest_RIP, (u64)(ulong_t)lhv_guest_entry);
	vmcs_vmwrite(vcpu, VMCS_guest_RFLAGS, (1 << 1));	// TODO
	//CS, DS, ES, FS, GS and SS segments
	vmcs_vmwrite(vcpu, VMCS_guest_CS_limit, 0xFFFF);
	vmcs_vmwrite(vcpu, VMCS_guest_CS_access_rights, 0x9b);
	vmcs_vmwrite(vcpu, VMCS_guest_DS_selector, __DS);
	vmcs_vmwrite(vcpu, VMCS_guest_DS_base, 0);
	vmcs_vmwrite(vcpu, VMCS_guest_DS_limit, 0xFFFF);
	vmcs_vmwrite(vcpu, VMCS_guest_DS_access_rights, 0x93);
	vmcs_vmwrite(vcpu, VMCS_guest_ES_selector, __DS);
	vmcs_vmwrite(vcpu, VMCS_guest_ES_base, 0);
	vmcs_vmwrite(vcpu, VMCS_guest_ES_limit, 0xFFFF);
	vmcs_vmwrite(vcpu, VMCS_guest_ES_access_rights, 0x93);
	vmcs_vmwrite(vcpu, VMCS_guest_FS_selector, __DS);
	vmcs_vmwrite(vcpu, VMCS_guest_FS_base, 0);
	vmcs_vmwrite(vcpu, VMCS_guest_FS_limit, 0xFFFF);
	vmcs_vmwrite(vcpu, VMCS_guest_FS_access_rights, 0x93);
	vmcs_vmwrite(vcpu, VMCS_guest_GS_selector, __DS);
	vmcs_vmwrite(vcpu, VMCS_guest_GS_base, 0);
	vmcs_vmwrite(vcpu, VMCS_guest_GS_limit, 0xFFFF);
	vmcs_vmwrite(vcpu, VMCS_guest_GS_access_rights, 0x93);
	vmcs_vmwrite(vcpu, VMCS_guest_SS_selector, __DS);
	vmcs_vmwrite(vcpu, VMCS_guest_SS_base, 0);
	vmcs_vmwrite(vcpu, VMCS_guest_SS_limit, 0xFFFF);
	vmcs_vmwrite(vcpu, VMCS_guest_SS_access_rights, 0x93);

	//setup VMCS link pointer
#ifdef __AMD64__
	vmcs_vmwrite(vcpu, VMCS_guest_VMCS_link_pointer, (u64)0xFFFFFFFFFFFFFFFFULL);
#elif defined(__I386__)
	vmcs_vmwrite(vcpu, VMCS_guest_VMCS_link_pointer_full, 0xFFFFFFFFUL);
	vmcs_vmwrite(vcpu, VMCS_guest_VMCS_link_pointer_high, 0xFFFFFFFFUL);
#else /* !defined(__I386__) && !defined(__AMD64__) */
    #error "Unsupported Arch"
#endif /* !defined(__I386__) && !defined(__AMD64__) */

	//trap access to CR0 fixed 1-bits
	{
		ulong_t cr0_mask = vcpu->vmx_msrs[INDEX_IA32_VMX_CR0_FIXED0_MSR];
		cr0_mask &= ~(CR0_PE);
		cr0_mask &= ~(CR0_PG);
		cr0_mask |= CR0_CD;
		cr0_mask |= CR0_NW;
		vmcs_vmwrite(vcpu, VMCS_control_CR0_mask, cr0_mask);
	}
	vmcs_vmwrite(vcpu, VMCS_control_CR0_shadow, CR0_ET);

	//trap access to CR4 fixed bits (this includes the VMXE bit)
	vmcs_vmwrite(vcpu, VMCS_control_CR4_mask,
				vcpu->vmx_msrs[INDEX_IA32_VMX_CR4_FIXED0_MSR]);
	vmcs_vmwrite(vcpu, VMCS_control_CR0_shadow, 0);
}

void lhv_vmx_main(VCPU *vcpu)
{
	u32 vmcs_revision_identifier;

	/* Make sure this is Intel CPU */
	HALT_ON_ERRORCOND(get_cpu_vendor_or_die() == CPU_VENDOR_INTEL);

	/* Save contents of MSRs (from _vmx_initVT) */
	{
		u32 i;
		for(i = 0; i < IA32_VMX_MSRCOUNT; i++) {
			vcpu->vmx_msrs[i] = rdmsr64(IA32_VMX_BASIC_MSR + i);
		}
		vcpu->vmx_msr_efer = rdmsr64(MSR_EFER);
		vcpu->vmx_msr_efcr = rdmsr64(MSR_EFCR);
		if((u32)((u64)vcpu->vmx_msrs[IA32_VMX_MSRCOUNT-1] >> 32) & 0x80) {
			vcpu->vmx_guest_unrestricted = 1;
		} else {
			vcpu->vmx_guest_unrestricted = 0;
		}
		HALT_ON_ERRORCOND(vcpu->vmx_guest_unrestricted);
	}

	/* Discover support for VMX (22.6 DISCOVERING SUPPORT FOR VMX) */
	{
		u32 eax, ebx, ecx, edx;
		cpuid(1, &eax, &ebx, &ecx, &edx);
		HALT_ON_ERRORCOND(ecx & (1U << 5));
	}

	/* Allocate VM (23.11.5 VMXON Region) */
	{
		u64 basic_msr = vcpu->vmx_msrs[INDEX_IA32_VMX_BASIC_MSR];
		vmcs_revision_identifier = (u32) basic_msr & 0x7fffffffU;
		vcpu->vmxon_region = (void *) all_vmxon_region[vcpu->idx];
		*((u32 *) vcpu->vmxon_region) = vmcs_revision_identifier;
	}

	/* Set CR4.VMXE (22.7 ENABLING AND ENTERING VMX OPERATION) */
	{
		ulong_t cr4 = read_cr4();
		HALT_ON_ERRORCOND((cr4 & CR4_VMXE) == 0);
		write_cr4(cr4 | CR4_VMXE);
	}

	/* Check IA32_FEATURE_CONTROL (22.7 ENABLING AND ENTERING VMX OPERATION) */
	{
		printf("\nrdmsr64(MSR_EFCR) = 0x%016x", vcpu->vmx_msr_efcr);
		HALT_ON_ERRORCOND(vcpu->vmx_msr_efcr & 1);
		HALT_ON_ERRORCOND(vcpu->vmx_msr_efcr & 4);
	}

	/* VMXON */
	{
		HALT_ON_ERRORCOND(__vmx_vmxon(hva2spa(vcpu->vmxon_region)));
	}

	/* VMCLEAR, VMPTRLD */
	{
		vcpu->my_vmcs = all_vmcs[vcpu->idx][0];
		HALT_ON_ERRORCOND(__vmx_vmclear(hva2spa(vcpu->my_vmcs)));
		*((u32 *) vcpu->my_vmcs) = vmcs_revision_identifier;
		HALT_ON_ERRORCOND(__vmx_vmptrld(hva2spa(vcpu->my_vmcs)));
	}

	/* Modify VMCS */
	lhv_vmx_vmcs_init(vcpu);

	vmcs_dump(vcpu);

//	asm volatile ("cli");	// TODO: tmp

	/* VMLAUNCH */
	{
		struct regs r;
		memset(&r, 0, sizeof(r));
		vmlaunch_asm(&r);
	}

	HALT_ON_ERRORCOND(0 && "vmlaunch_asm() should never return");
}

void vmexit_handler(VCPU *vcpu, struct regs *r)
{
	ulong_t vmexit_reason = vmcs_vmread(vcpu, VMCS_info_vmexit_reason);
	printf("\nCPU(0x%02x): vmexit_reason = 0x%lx", vcpu->id, vmexit_reason);
	printf("\nCPU(0x%02x): r->eax = 0x%x", vcpu->id, r->eax);
	vmcs_dump(vcpu);
	HALT_ON_ERRORCOND(0);
}

void vmentry_error(ulong_t is_resume, ulong_t valid)
{
	VCPU *vcpu = _svm_and_vmx_getvcpu();
	/* 29.4 VM INSTRUCTION ERROR NUMBERS */
	ulong_t vminstr_error = vmcs_vmread(vcpu, VMCS_info_vminstr_error);
	printf("\nCPU(0x%02x): is_resume = %ld, valid = %ld, err = %ld",
			vcpu->id, is_resume, valid, vminstr_error);
	HALT_ON_ERRORCOND(is_resume && valid && 0);
	HALT_ON_ERRORCOND(0);
}
