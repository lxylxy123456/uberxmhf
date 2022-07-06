#include <xmhf.h>
#include <lhv.h>
#include <limits.h>

#define MAX_GUESTS 4
#define MAX_MSR_LS 16	/* Max number of MSRs in MSR load / store */

static u8 all_vmxon_region[MAX_VCPU_ENTRIES][PAGE_SIZE_4K]
__attribute__(( section(".bss.palign_data") ));

static u8 all_vmcs[MAX_VCPU_ENTRIES][MAX_GUESTS][PAGE_SIZE_4K]
__attribute__(( section(".bss.palign_data") ));

static u8 all_guest_stack[MAX_VCPU_ENTRIES][MAX_GUESTS][PAGE_SIZE_4K]
__attribute__(( section(".bss.palign_data") ));

static msr_entry_t vmexit_msrstore_entries[MAX_VCPU_ENTRIES][MAX_GUESTS][MAX_MSR_LS]
__attribute__((aligned(16)));
static msr_entry_t vmexit_msrload_entries[MAX_VCPU_ENTRIES][MAX_GUESTS][MAX_MSR_LS]
__attribute__((aligned(16)));
static msr_entry_t vmentry_msrload_entries[MAX_VCPU_ENTRIES][MAX_GUESTS][MAX_MSR_LS]
__attribute__((aligned(16)));

extern u32 x_gdt_start[];

static void lhv_vmx_vmcs_init(VCPU *vcpu)
{
	// From vmx_initunrestrictedguestVMCS
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
	{
		u32 vmexit_ctls = vcpu->vmx_msrs[INDEX_IA32_VMX_EXIT_CTLS_MSR];
#ifdef __AMD64__
		vmexit_ctls |= (1UL << 9);
#elif !defined(__I386__)
    #error "Unsupported Arch"
#endif /* !defined(__I386__) */
		vmcs_vmwrite(vcpu, VMCS_control_VM_exit_controls, vmexit_ctls);
	}
	{
		u32 vmentry_ctls = vcpu->vmx_msrs[INDEX_IA32_VMX_ENTRY_CTLS_MSR];
#ifdef __AMD64__
		vmentry_ctls |= (1UL << 9);
#elif !defined(__I386__)
    #error "Unsupported Arch"
#endif /* !defined(__I386__) */
		vmcs_vmwrite(vcpu, VMCS_control_VM_entry_controls, vmentry_ctls);
	}
	//TODO: enable unrestricted guest using ` | (1 << 7)`
	vmcs_vmwrite(vcpu, VMCS_control_VMX_seccpu_based,
				vcpu->vmx_msrs[INDEX_IA32_VMX_PROCBASED_CTLS2_MSR]);

	//Critical MSR load/store
	if (__LHV_OPT__ & LHV_USE_MSR_LOAD) {
		vmcs_vmwrite(vcpu, VMCS_control_VM_exit_MSR_store_count, 3);
		vmcs_vmwrite(vcpu, VMCS_control_VM_exit_MSR_load_count, 3);
		vmcs_vmwrite(vcpu, VMCS_control_VM_entry_MSR_load_count, 3);
		vcpu->my_vmexit_msrstore = vmexit_msrstore_entries[0][vcpu->idx];
		vcpu->my_vmexit_msrload = vmexit_msrload_entries[0][vcpu->idx];
		vcpu->my_vmentry_msrload = vmentry_msrload_entries[0][vcpu->idx];
		vmcs_vmwrite64(vcpu, VMCS_control_VM_exit_MSR_store_address,
						hva2spa(vcpu->my_vmexit_msrstore));
		vmcs_vmwrite64(vcpu, VMCS_control_VM_exit_MSR_load_address,
						hva2spa(vcpu->my_vmexit_msrload));
		vmcs_vmwrite64(vcpu, VMCS_control_VM_entry_MSR_load_address,
						hva2spa(vcpu->my_vmentry_msrload));
		if (0) {
			printf("MTRR 0x%016llx\n", rdmsr64(0xfeU));
			for (u32 i = 0x200; i < 0x210; i++) {
				printf("MTRR 0x%08x 0x%016llx\n", i, rdmsr64(i));
			}
		}
		HALT_ON_ERRORCOND((rdmsr64(0xfeU) & 0xff) > 6);
		wrmsr64(0x20aU, 0x00000000aaaaa000ULL);
		wrmsr64(0x20bU, 0x00000000deadb000ULL);
		wrmsr64(0x20cU, 0x00000000deadc000ULL);
		wrmsr64(0x402U, 0x2222222222222222ULL);
		wrmsr64(0x403U, 0xdeaddeadbeefbeefULL);
		wrmsr64(0x406U, 0xdeaddeadbeefbeefULL);
		HALT_ON_ERRORCOND(rdmsr64(MSR_IA32_PAT) == 0x0007040600070406ULL);
		vcpu->my_vmexit_msrstore[0].index = 0x20aU;
		vcpu->my_vmexit_msrstore[0].data = 0x00000000deada000ULL;
		vcpu->my_vmexit_msrstore[1].index = MSR_EFER;
		vcpu->my_vmexit_msrstore[1].data = 0x00000000deada000ULL;
		vcpu->my_vmexit_msrstore[2].index = 0x402U;
		vcpu->my_vmexit_msrstore[2].data = 0xdeaddeadbeefbeefULL;
		vcpu->my_vmexit_msrload[0].index = 0x20bU;
		vcpu->my_vmexit_msrload[0].data = 0x00000000bbbbb000ULL;
		vcpu->my_vmexit_msrload[1].index = 0xc0000081U;
		vcpu->my_vmexit_msrload[1].data = 0x0000000011111000ULL;
		vcpu->my_vmexit_msrload[2].index = 0x403U;
		vcpu->my_vmexit_msrload[2].data = 0x3333333333333333ULL;
		vcpu->my_vmentry_msrload[0].index = 0x20cU;
		vcpu->my_vmentry_msrload[0].data = 0x00000000ccccc000ULL;
		vcpu->my_vmentry_msrload[1].index = MSR_IA32_PAT;
		vcpu->my_vmentry_msrload[1].data = 0x0007060400070604ULL;
		vcpu->my_vmentry_msrload[2].index = 0x406U;
		vcpu->my_vmentry_msrload[2].data = 0x6666666666666666ULL;
		printf("CPU(0x%02x): configured load/store MSR\n", vcpu->id);
	} else {
		vmcs_vmwrite(vcpu, VMCS_control_VM_exit_MSR_load_count, 0);
		vmcs_vmwrite(vcpu, VMCS_control_VM_entry_MSR_load_count, 0);
		vmcs_vmwrite(vcpu, VMCS_control_VM_exit_MSR_store_count, 0);
	}

	if (__LHV_OPT__ & LHV_USE_EPT) {
		u64 eptp = lhv_build_ept(vcpu, 0);
		u32 seccpu = vmcs_vmread(vcpu, VMCS_control_VMX_seccpu_based);
		seccpu |= (1U << VMX_SECPROCBASED_ENABLE_EPT);
		vmcs_vmwrite(vcpu, VMCS_control_VMX_seccpu_based, seccpu);
		vmcs_vmwrite64(vcpu, VMCS_control_EPT_pointer, eptp | 0x1eULL);
#ifdef __I386__
		{
			u64 *cr3 = (u64 *)read_cr3();
			vmcs_vmwrite64(vcpu, VMCS_guest_PDPTE0, cr3[0]);
			vmcs_vmwrite64(vcpu, VMCS_guest_PDPTE1, cr3[1]);
			vmcs_vmwrite64(vcpu, VMCS_guest_PDPTE2, cr3[2]);
			vmcs_vmwrite64(vcpu, VMCS_guest_PDPTE3, cr3[3]);
		}
#endif /* __I386__ */
	}

	if (__LHV_OPT__ & LHV_USE_VPID) {
		u32 seccpu = vmcs_vmread(vcpu, VMCS_control_VMX_seccpu_based);
		seccpu |= (1U << VMX_SECPROCBASED_ENABLE_VPID);
		vmcs_vmwrite(vcpu, VMCS_control_VMX_seccpu_based, seccpu);
		vmcs_vmwrite(vcpu, VMCS_control_vpid, 1);
	}

	if (__LHV_OPT__ & LHV_USE_UNRESTRICTED_GUEST) {
		u32 seccpu;
		HALT_ON_ERRORCOND(__LHV_OPT__ & LHV_USE_EPT);
		seccpu = vmcs_vmread(vcpu, VMCS_control_VMX_seccpu_based);
		seccpu |= (1U << VMX_SECPROCBASED_UNRESTRICTED_GUEST);
		vmcs_vmwrite(vcpu, VMCS_control_VMX_seccpu_based, seccpu);
	}

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
	{
		vcpu->my_stack = all_guest_stack[vcpu->idx][0];
		vmcs_vmwrite(vcpu, VMCS_guest_RSP, (u64)(ulong_t)vcpu->my_stack);
	}
	//RIP
	vmcs_vmwrite(vcpu, VMCS_guest_CS_selector, __CS);
	vmcs_vmwrite(vcpu, VMCS_guest_CS_base, 0);
	vmcs_vmwrite(vcpu, VMCS_guest_RIP, (u64)(ulong_t)lhv_guest_entry);
	vmcs_vmwrite(vcpu, VMCS_guest_RFLAGS, (1 << 1));	// TODO
	//CS, DS, ES, FS, GS and SS segments
	vmcs_vmwrite(vcpu, VMCS_guest_CS_limit, 0xffffffff);
#ifdef __AMD64__
	vmcs_vmwrite(vcpu, VMCS_guest_CS_access_rights, 0xa09b);
#elif defined(__I386__)
	vmcs_vmwrite(vcpu, VMCS_guest_CS_access_rights, 0xc09b);
#else /* !defined(__I386__) && !defined(__AMD64__) */
    #error "Unsupported Arch"
#endif /* !defined(__I386__) && !defined(__AMD64__) */
	vmcs_vmwrite(vcpu, VMCS_guest_DS_selector, __DS);
	vmcs_vmwrite(vcpu, VMCS_guest_DS_base, 0);
	vmcs_vmwrite(vcpu, VMCS_guest_DS_limit, 0xffffffff);
	vmcs_vmwrite(vcpu, VMCS_guest_DS_access_rights, 0xc093);
	vmcs_vmwrite(vcpu, VMCS_guest_ES_selector, __DS);
	vmcs_vmwrite(vcpu, VMCS_guest_ES_base, 0);
	vmcs_vmwrite(vcpu, VMCS_guest_ES_limit, 0xffffffff);
	vmcs_vmwrite(vcpu, VMCS_guest_ES_access_rights, 0xc093);
	vmcs_vmwrite(vcpu, VMCS_guest_FS_selector, __DS);
	vmcs_vmwrite(vcpu, VMCS_guest_FS_base, 0);
	vmcs_vmwrite(vcpu, VMCS_guest_FS_limit, 0xffffffff);
	vmcs_vmwrite(vcpu, VMCS_guest_FS_access_rights, 0xc093);
	vmcs_vmwrite(vcpu, VMCS_guest_GS_selector, __DS);
	vmcs_vmwrite(vcpu, VMCS_guest_GS_base, 0);
	vmcs_vmwrite(vcpu, VMCS_guest_GS_limit, 0xffffffff);
	vmcs_vmwrite(vcpu, VMCS_guest_GS_access_rights, 0xc093);
	vmcs_vmwrite(vcpu, VMCS_guest_SS_selector, __DS);
	vmcs_vmwrite(vcpu, VMCS_guest_SS_base, 0);
	vmcs_vmwrite(vcpu, VMCS_guest_SS_limit, 0xffffffff);
	vmcs_vmwrite(vcpu, VMCS_guest_SS_access_rights, 0xc093);

	//setup VMCS link pointer
	vmcs_vmwrite64(vcpu, VMCS_guest_VMCS_link_pointer, (u64)0xFFFFFFFFFFFFFFFFULL);

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
		if (_vmx_hasctl_unrestricted_guest(&vcpu->vmx_caps)) {
			vcpu->vmx_guest_unrestricted = 1;
		} else {
			vcpu->vmx_guest_unrestricted = 0;
		}
		/* At this point LHV does not require EPT and unrestricted guest yet. */
		// TODO: HALT_ON_ERRORCOND(vcpu->vmx_guest_unrestricted);
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

	write_cr0(read_cr0() | CR0_NE);

	/* Set CR4.VMXE (22.7 ENABLING AND ENTERING VMX OPERATION) */
	{
		ulong_t cr4 = read_cr4();
		HALT_ON_ERRORCOND((cr4 & CR4_VMXE) == 0);
		write_cr4(cr4 | CR4_VMXE);
	}

	/* Check IA32_FEATURE_CONTROL (22.7 ENABLING AND ENTERING VMX OPERATION) */
	{
		u64 vmx_msr_efcr = rdmsr64(MSR_EFCR);
		// printf("rdmsr64(MSR_EFCR) = 0x%016x\n", vmx_msr_efcr);
		HALT_ON_ERRORCOND(vmx_msr_efcr & 1);
		HALT_ON_ERRORCOND(vmx_msr_efcr & 4);
	}

	/* VMXON */
	{
		HALT_ON_ERRORCOND(__vmx_vmxon(hva2spa(vcpu->vmxon_region)));
	}

	/* VMCLEAR, VMPTRLD */
	{
		vcpu->my_vmcs = all_vmcs[vcpu->idx][0];
		if (!"test_vmclear" && vcpu->isbsp) {
			for (u32 i = 0; i < 0x1000 / sizeof(u32); i++) {
				((u32 *) vcpu->my_vmcs)[i] = (i << 20) | i;
			}
		}
		HALT_ON_ERRORCOND(__vmx_vmclear(hva2spa(vcpu->my_vmcs)));
		*((u32 *) vcpu->my_vmcs) = vmcs_revision_identifier;
		HALT_ON_ERRORCOND(__vmx_vmptrld(hva2spa(vcpu->my_vmcs)));
		if (!"test_vmclear" && vcpu->isbsp) {
			for (u32 i = 0; i < 0x1000 / sizeof(u32); i++) {
				printf("vmcs[0x%03x] = 0x%08x\n", i, ((u32 *)vcpu->my_vmcs)[i]);
			}
#define DECLARE_FIELD(encoding, name)							\
			{													\
				printf("vmread(0x%04x) = %08lx\n", encoding,	\
						vmcs_vmread(vcpu, encoding));			\
			}
#include <lhv-vmcs-template.h>
#undef DECLARE_FIELD
		}
	}

	/* Modify VMCS */
	lhv_vmx_vmcs_init(vcpu);
	vmcs_dump(vcpu, 0);

	/* VMLAUNCH */
	{
		struct regs r;
		memset(&r, 0, sizeof(r));
		r.edi = vcpu->id;
		vmlaunch_asm(&r);
	}

	HALT_ON_ERRORCOND(0 && "vmlaunch_asm() should never return");
}

void vmexit_handler(VCPU *vcpu, struct regs *r)
{
	ulong_t vmexit_reason = vmcs_vmread(vcpu, VMCS_info_vmexit_reason);
	ulong_t guest_rip = vmcs_vmread(vcpu, VMCS_guest_RIP);
	ulong_t inst_len = vmcs_vmread(vcpu, VMCS_info_vmexit_instruction_length);
	HALT_ON_ERRORCOND(vcpu == _svm_and_vmx_getvcpu());
	if (__LHV_OPT__ & LHV_USE_MSR_LOAD) {
		HALT_ON_ERRORCOND(vcpu->my_vmexit_msrstore[0].data == 0x00000000aaaaa000ULL);
		HALT_ON_ERRORCOND(vcpu->my_vmexit_msrstore[1].data == rdmsr64(MSR_EFER));
		HALT_ON_ERRORCOND(vcpu->my_vmexit_msrstore[2].data == 0x2222222222222222ULL);
		HALT_ON_ERRORCOND(rdmsr64(0x20bU) == 0x00000000bbbbb000ULL);
		HALT_ON_ERRORCOND(rdmsr64(MSR_IA32_PAT) == 0x0007060400070604ULL);
		HALT_ON_ERRORCOND(rdmsr64(0x403U) == 0x3333333333333333ULL);
		HALT_ON_ERRORCOND(rdmsr64(0x20cU) == 0x00000000ccccc000ULL);
		HALT_ON_ERRORCOND(rdmsr64(0xc0000081U) == 0x0000000011111000ULL);
		HALT_ON_ERRORCOND(rdmsr64(0x406U) == 0x6666666666666666ULL);
	}
	switch (vmexit_reason) {
	case VMX_VMEXIT_CPUID:
		{
			u32 old_eax = r->eax;
			asm volatile ("cpuid\r\n"
				  :"=a"(r->eax), "=b"(r->ebx), "=c"(r->ecx), "=d"(r->edx)
				  :"a"(r->eax), "c" (r->ecx));
			if (old_eax == 0x1) {
				/* Clear VMX capability */
				r->ecx &= ~(1U << 5);
			}
			vmcs_vmwrite(vcpu, VMCS_guest_RIP, guest_rip + inst_len);
			break;
		}
	case VMX_VMEXIT_RDMSR:
		{
			asm volatile ("rdmsr\r\n"
				  :"=a"(r->eax), "=d"(r->edx)
				  :"c" (r->ecx));
			vmcs_vmwrite(vcpu, VMCS_guest_RIP, guest_rip + inst_len);
			break;
		}
	case VMX_VMEXIT_VMCALL:
		if (r->eax == 0x4321) {
			u64 eptp = vmcs_vmread(vcpu, VMCS_control_EPT_pointer);
			u64 cr3 = vmcs_vmread(vcpu, VMCS_guest_CR3);
			u64 ept_t4 = (eptp & ~0xfff);
			u64 ept_e4 = *(u64*)(uintptr_t)(ept_t4 + (((cr3 >> 39) & 0x1ff) << 3));
			u64 ept_t3 = (ept_e4 & ~0xfff);
			u64 ept_e3 = *(u64*)(uintptr_t)(ept_t3 + (((cr3 >> 30) & 0x1ff) << 3));
			u64 ept_t2 = (ept_e3 & ~0xfff);
			u64 ept_e2 = *(u64*)(uintptr_t)(ept_t2 + (((cr3 >> 21) & 0x1ff) << 3));
			u64 ept_t1 = (ept_e2 & ~0xfff);
			u64 *ept_e1p = (u64*)(uintptr_t)(ept_t1 + (((cr3 >> 12) & 0x1ff) << 3));
			u64 ept_e1 = *ept_e1p;
			printf("EPT E1 = 0x%016llx\n", ept_e1);
			printf("CR3    = 0x%016llx\n", cr3);
			*ept_e1p = 0;
			__vmx_invept(VMX_INVEPT_GLOBAL, 0);
		} else {
			HALT_ON_ERRORCOND(0);
		}
		vmcs_vmwrite(vcpu, VMCS_guest_RIP, guest_rip + inst_len);
		break;
	case VMX_VMEXIT_EPT_VIOLATION:
		console_put_char(NULL, 20, 21, 'G');
		console_put_char(NULL, 21, 21, 'O');
		console_put_char(NULL, 22, 21, 'O');
		console_put_char(NULL, 23, 21, 'D');
		HALT_ON_ERRORCOND(0 && "hypervisor receives EPT (correct behavior)");
		HALT_ON_ERRORCOND(__LHV_OPT__ & LHV_USE_EPT);
		{
			ulong_t q = vmcs_vmread(vcpu, VMCS_info_exit_qualification);
			u64 paddr = vmcs_vmread64(vcpu, VMCS_guest_paddr);
			ulong_t vaddr = vmcs_vmread(vcpu, VMCS_info_guest_linear_address);
			/* Unknown EPT violation */
			printf("CPU(0x%02x): ept: 0x%08lx\n", vcpu->id, q);
			printf("CPU(0x%02x): paddr: 0x%016llx\n", vcpu->id, paddr);
			printf("CPU(0x%02x): vaddr: 0x%08lx\n", vcpu->id, vaddr);
			vmcs_dump(vcpu, 0);
			HALT_ON_ERRORCOND(0);
			break;
		}
	default:
		{
			printf("CPU(0x%02x): vmexit: 0x%lx\n", vcpu->id, vmexit_reason);
			printf("CPU(0x%02x): r->eax = 0x%x\n", vcpu->id, r->eax);
			printf("CPU(0x%02x): rip = 0x%x\n", vcpu->id, guest_rip);
			vmcs_dump(vcpu, 0);
			HALT_ON_ERRORCOND(0);
			break;
		}
	}
	vmresume_asm(r);
}

void vmentry_error(ulong_t is_resume, ulong_t valid)
{
	VCPU *vcpu = _svm_and_vmx_getvcpu();
	/* 29.4 VM INSTRUCTION ERROR NUMBERS */
	ulong_t vminstr_error = vmcs_vmread(vcpu, VMCS_info_vminstr_error);
	printf("CPU(0x%02x): is_resume = %ld, valid = %ld, err = %ld\n",
			vcpu->id, is_resume, valid, vminstr_error);
	HALT_ON_ERRORCOND(is_resume && valid && 0);
	HALT_ON_ERRORCOND(0);
}
