#include <xmhf.h>
#include <lhv.h>

/* Test whether MSR load / store during VMEXIT / VMENTRY are effective */
static void lhv_guest_test_msr_ls_vmexit_handler(VCPU *vcpu, struct regs *r,
												 vmexit_info_t *info)
{
	if (info->vmexit_reason != VMX_VMEXIT_VMCALL) {
		return;
	}
	switch (r->eax) {
	case 12:
		/* Set experiment */
		vmcs_vmwrite(vcpu, VMCS_control_VM_exit_MSR_store_count, 3);
		vmcs_vmwrite(vcpu, VMCS_control_VM_exit_MSR_load_count, 3);
		vmcs_vmwrite(vcpu, VMCS_control_VM_entry_MSR_load_count, 3);
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
		break;
	case 16:
		/* Check effects */
		HALT_ON_ERRORCOND(vcpu->my_vmexit_msrstore[0].data ==
						  0x00000000aaaaa000ULL);
		HALT_ON_ERRORCOND(vcpu->my_vmexit_msrstore[1].data ==
						  rdmsr64(MSR_EFER));
		HALT_ON_ERRORCOND(vcpu->my_vmexit_msrstore[2].data ==
						  0x2222222222222222ULL);
		HALT_ON_ERRORCOND(rdmsr64(0x20bU) == 0x00000000bbbbb000ULL);
		HALT_ON_ERRORCOND(rdmsr64(MSR_IA32_PAT) == 0x0007060400070604ULL);
		HALT_ON_ERRORCOND(rdmsr64(0x403U) == 0x3333333333333333ULL);
		HALT_ON_ERRORCOND(rdmsr64(0x20cU) == 0x00000000ccccc000ULL);
		HALT_ON_ERRORCOND(rdmsr64(0xc0000081U) == 0x0000000011111000ULL);
		HALT_ON_ERRORCOND(rdmsr64(0x406U) == 0x6666666666666666ULL);
		/* Reset state */
		vmcs_vmwrite(vcpu, VMCS_control_VM_exit_MSR_store_count, 0);
		vmcs_vmwrite(vcpu, VMCS_control_VM_exit_MSR_load_count, 0);
		vmcs_vmwrite(vcpu, VMCS_control_VM_entry_MSR_load_count, 0);
		wrmsr64(MSR_IA32_PAT, 0x0007040600070406ULL);
		break;
	default:
		HALT_ON_ERRORCOND(0 && "Unknown argument");
		break;
	}
	vmcs_vmwrite(vcpu, VMCS_guest_RIP, info->guest_rip + info->inst_len);
	vmresume_asm(r);
}

static void lhv_guest_test_msr_ls(void)
{
	HALT_ON_ERRORCOND(__LHV_OPT__ & LHV_USE_MSR_LOAD);
	vmexit_handler_override = lhv_guest_test_msr_ls_vmexit_handler;
	asm volatile ("vmcall" : : "a"(12));
	asm volatile ("vmcall" : : "a"(16));
	vmexit_handler_override = NULL;
}

/* Test whether EPT VMEXITs happen as expected */
static void lhv_guest_test_ept_vmexit_handler(VCPU *vcpu, struct regs *r,
											  vmexit_info_t *info)
{
	if (info->vmexit_reason != VMX_VMEXIT_EPT_VIOLATION) {
		return;
	}
	{
		ulong_t q = vmcs_vmread(vcpu, VMCS_info_exit_qualification);
		u64 paddr = vmcs_vmread64(vcpu, VMCS_guest_paddr);
		ulong_t vaddr = vmcs_vmread(vcpu, VMCS_info_guest_linear_address);
		if (paddr != 0x12340000 || vaddr != 0x12340000) {
			/* Let the default handler report the error */
			return;
		}
		HALT_ON_ERRORCOND(q == 0x181);
		HALT_ON_ERRORCOND(r->eax == 0xdeadbeef);
		HALT_ON_ERRORCOND(r->ebx == 0x12340000);
		r->eax = 0xfee1c0de;
		vcpu->ept_exit_count++;
	}
	vmcs_vmwrite(vcpu, VMCS_guest_RIP, info->guest_rip + info->inst_len);
	vmresume_asm(r);
}

static void lhv_guest_test_ept(VCPU *vcpu)
{
	u32 expected_ept_count;
	HALT_ON_ERRORCOND(__LHV_OPT__ & LHV_USE_EPT);
	HALT_ON_ERRORCOND(vcpu->ept_exit_count == 0);
	vmexit_handler_override = lhv_guest_test_ept_vmexit_handler;
	{
		u32 a = 0xdeadbeef;
		u32 *p = (u32 *)0x12340000;
		asm volatile("movl (%1), %%eax" :
					 "+a" (a) :
					 "b" (p) :
					 "cc", "memory");
		if (0) {
			printf("CPU(0x%02x): EPT result: 0x%08x 0x%02x\n", vcpu->id, a,
				   vcpu->ept_num);
		}
		if (vcpu->ept_num == 0) {
			HALT_ON_ERRORCOND(a == 0xfee1c0de);
			expected_ept_count = 1;
		} else {
			HALT_ON_ERRORCOND((u8) a == vcpu->ept_num);
			HALT_ON_ERRORCOND((u8) (a >> 8) == vcpu->ept_num);
			HALT_ON_ERRORCOND((u8) (a >> 16) == vcpu->ept_num);
			HALT_ON_ERRORCOND((u8) (a >> 24) == vcpu->ept_num);
			expected_ept_count = 0;
		}
	}
	vmexit_handler_override = NULL;
	HALT_ON_ERRORCOND(vcpu->ept_exit_count == expected_ept_count);
	vcpu->ept_exit_count = 0;
}

/* Switch EPT */
static void lhv_guest_switch_ept_vmexit_handler(VCPU *vcpu, struct regs *r,
												vmexit_info_t *info)
{
	if (info->vmexit_reason != VMX_VMEXIT_VMCALL) {
		return;
	}
	HALT_ON_ERRORCOND(r->eax == 17);
	{
		u64 eptp;
		/* Check prerequisite */
		HALT_ON_ERRORCOND(__LHV_OPT__ & LHV_USE_EPT);
		/* Swap EPT */
		vcpu->ept_num++;
		vcpu->ept_num %= (LHV_EPT_COUNT << 4);
		eptp = lhv_build_ept(vcpu, vcpu->ept_num);
		vmcs_vmwrite64(vcpu, VMCS_control_EPT_pointer, eptp | 0x1eULL);
	}
	vmcs_vmwrite(vcpu, VMCS_guest_RIP, info->guest_rip + info->inst_len);
	vmresume_asm(r);
}

static void lhv_guest_switch_ept(void)
{
	HALT_ON_ERRORCOND(__LHV_OPT__ & LHV_USE_SWITCH_EPT);
	HALT_ON_ERRORCOND(__LHV_OPT__ & LHV_USE_EPT);
	vmexit_handler_override = lhv_guest_switch_ept_vmexit_handler;
	asm volatile ("vmcall" : : "a"(17));
	vmexit_handler_override = NULL;
}

/* Test changing VPID and whether INVVPID returns the correct error code */
static void lhv_guest_test_vpid_vmexit_handler(VCPU *vcpu, struct regs *r,
											   vmexit_info_t *info)
{
	if (info->vmexit_reason != VMX_VMEXIT_VMCALL) {
		return;
	}
	HALT_ON_ERRORCOND(r->eax == 19);
	{
		/* VPID will always be odd */
		u16 vpid = vmcs_vmread(vcpu, VMCS_control_vpid);
		HALT_ON_ERRORCOND(vpid % 2 == 1);
		/*
		 * Currently we cannot easily test the effect of INVVPID. So
		 * just make sure that the return value is correct.
		 */
		HALT_ON_ERRORCOND(__vmx_invvpid(VMX_INVVPID_INDIVIDUALADDRESS, vpid,
										0x12345678U));
#ifdef __AMD64__
		HALT_ON_ERRORCOND(!__vmx_invvpid(VMX_INVVPID_INDIVIDUALADDRESS, vpid,
										 0xf0f0f0f0f0f0f0f0ULL));
#elif !defined(__I386__)
    #error "Unsupported Arch"
#endif /* !defined(__I386__) */
		HALT_ON_ERRORCOND(!__vmx_invvpid(VMX_INVVPID_INDIVIDUALADDRESS, 0,
										 0x12345678U));
		HALT_ON_ERRORCOND(__vmx_invvpid(VMX_INVVPID_SINGLECONTEXT, vpid, 0));
		HALT_ON_ERRORCOND(!__vmx_invvpid(VMX_INVVPID_SINGLECONTEXT, 0, 0));
		HALT_ON_ERRORCOND(__vmx_invvpid(VMX_INVVPID_ALLCONTEXTS, vpid, 0));
		HALT_ON_ERRORCOND(__vmx_invvpid(VMX_INVVPID_SINGLECONTEXTGLOBAL, vpid,
										0));
		HALT_ON_ERRORCOND(!__vmx_invvpid(VMX_INVVPID_SINGLECONTEXTGLOBAL, 0,
										 0));
		/* Update VPID */
		vpid += 2;
		vmcs_vmwrite(vcpu, VMCS_control_vpid, vpid);
	}
	vmcs_vmwrite(vcpu, VMCS_guest_RIP, info->guest_rip + info->inst_len);
	vmresume_asm(r);
}

static void lhv_guest_test_vpid(void)
{
	HALT_ON_ERRORCOND(__LHV_OPT__ & LHV_USE_VPID);
	vmexit_handler_override = lhv_guest_test_vpid_vmexit_handler;
	asm volatile ("vmcall" : : "a"(19));
	vmexit_handler_override = NULL;
}

void lhv_guest_main(ulong_t cpu_id)
{
	VCPU *vcpu = _svm_and_vmx_getvcpu();
	console_vc_t vc;
	HALT_ON_ERRORCOND(cpu_id == vcpu->idx);
	console_get_vc(&vc, vcpu->idx, 1);
	console_clear(&vc);
	for (int i = 0; i < vc.width; i++) {
		for (int j = 0; j < 2; j++) {
			HALT_ON_ERRORCOND(console_get_char(&vc, i, j) == ' ');
			console_put_char(&vc, i, j, '0' + vcpu->id);
		}
	}
	if (!(__LHV_OPT__ & LHV_NO_EFLAGS_IF)) {
		asm volatile ("sti");
	}
	while (1) {
		if (!(__LHV_OPT__ & LHV_NO_EFLAGS_IF)) {
			asm volatile ("hlt");
		}
		if (__LHV_OPT__ & LHV_USE_MSR_LOAD) {
			lhv_guest_test_msr_ls();
		}
		if (__LHV_OPT__ & LHV_USE_EPT) {
			lhv_guest_test_ept(vcpu);
		}
		if (__LHV_OPT__ & LHV_USE_SWITCH_EPT) {
			lhv_guest_switch_ept();
		}
		if (__LHV_OPT__ & LHV_USE_VPID) {
			lhv_guest_test_vpid();
		}
		asm volatile ("vmcall");
		if (__LHV_OPT__ & LHV_USE_UNRESTRICTED_GUEST) {
#ifdef __AMD64__
			extern void lhv_disable_enable_paging(char *);
			if ("quiet") {
				lhv_disable_enable_paging("");
			} else {
				lhv_disable_enable_paging("LHV guest can disable paging\n");
			}
#elif defined(__I386__)
			ulong_t cr0 = read_cr0();
			asm volatile ("cli");
			write_cr0(cr0 & 0x7fffffffUL);
			if (0) {
				printf("CPU(0x%02x): LHV guest can disable paging\n", vcpu->id);
			}
			write_cr0(cr0);
			asm volatile ("sti");
#else /* !defined(__I386__) && !defined(__AMD64__) */
    #error "Unsupported Arch"
#endif /* !defined(__I386__) && !defined(__AMD64__) */
		}
		if (__LHV_OPT__ & LHV_USE_LARGE_PAGE) {
			HALT_ON_ERRORCOND(__LHV_OPT__ & LHV_USE_EPT);
			HALT_ON_ERRORCOND(large_pages[0][0] == 'B');
			HALT_ON_ERRORCOND(large_pages[1][0] == 'A');
		}
	}
}

void lhv_guest_xcphandler(uintptr_t vector, struct regs *r)
{
	(void) r;
	switch (vector) {
	case 0x20:
		handle_timer_interrupt(_svm_and_vmx_getvcpu(), vector, 1);
		break;
	case 0x21:
		handle_keyboard_interrupt(_svm_and_vmx_getvcpu(), vector, 1);
		break;
	case 0x22:
		handle_timer_interrupt(_svm_and_vmx_getvcpu(), vector, 1);
		break;
	default:
		printf("Guest: interrupt / exception vector %ld\n", vector);
		HALT_ON_ERRORCOND(0 && "Guest: unknown interrupt / exception!\n");
		break;
	}
}
