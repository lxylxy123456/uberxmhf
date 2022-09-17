#include <xmhf.h>
#include <lhv.h>

/*
 * From peh-x86-safemsr.c
 * Perform RDMSR instruction.
 * If successful, return 0. If RDMSR causes #GP, return 1.
 * Implementation similar to Linux's native_read_msr_safe().
 */
static u32 _rdmsr_safe(u32 index, u64 *value) {
    u32 result;
    u32 eax, edx;
    asm volatile ("1:\r\n"
                  "rdmsr\r\n"
                  "xor %%ebx, %%ebx\r\n"
                  "jmp 3f\r\n"
                  "2:\r\n"
                  "movl $1, %%ebx\r\n"
                  "jmp 3f\r\n"
                  ".section .xcph_table\r\n"
#ifdef __AMD64__
                  ".quad 0xd\r\n"
                  ".quad 1b\r\n"
                  ".quad 2b\r\n"
#elif defined(__I386__)
                  ".long 0xd\r\n"
                  ".long 1b\r\n"
                  ".long 2b\r\n"
#else /* !defined(__I386__) && !defined(__AMD64__) */
    #error "Unsupported Arch"
#endif /* !defined(__I386__) && !defined(__AMD64__) */
                  ".previous\r\n"
                  "3:\r\n"
                  : "=a"(eax), "=d"(edx), "=b"(result)
                  : "c" (index));
	if (result == 0) {
		*value = ((u64) edx << 32) | eax;
	}
    return result;
}

/*
 * Perform WRMSR instruction.
 * If successful, return 0. If WRMSR causes #GP, return 1.
 */
static u32 _wrmsr_safe(u32 index, u64 value) {
    u32 result;
    u32 eax = value, edx = value >> 32;
    asm volatile ("1:\r\n"
                  "wrmsr\r\n"
                  "xor %%ebx, %%ebx\r\n"
                  "jmp 3f\r\n"
                  "2:\r\n"
                  "movl $1, %%ebx\r\n"
                  "jmp 3f\r\n"
                  ".section .xcph_table\r\n"
#ifdef __AMD64__
                  ".quad 0xd\r\n"
                  ".quad 1b\r\n"
                  ".quad 2b\r\n"
#elif defined(__I386__)
                  ".long 0xd\r\n"
                  ".long 1b\r\n"
                  ".long 2b\r\n"
#else /* !defined(__I386__) && !defined(__AMD64__) */
    #error "Unsupported Arch"
#endif /* !defined(__I386__) && !defined(__AMD64__) */
                  ".previous\r\n"
                  "3:\r\n"
                  : "=b"(result)
                  : "c" (index), "a"(eax), "d"(edx));
    return result;
}

/* Return whether MSR is writable with a wild value. */
static bool msr_writable(u32 index) {
	u64 old_val;
	u64 new_val;
	if (_rdmsr_safe(index, &old_val)) {
		return false;
	}
	new_val = old_val ^ 0x1234abcdbeef6789ULL;
	if (_wrmsr_safe(index, new_val)) {
		return false;
	}
	HALT_ON_ERRORCOND(_wrmsr_safe(index, old_val) == 0);
	return true;
}

/* Test whether MSR load / store during VMEXIT / VMENTRY are effective */
typedef struct lhv_guest_test_msr_ls_data {
	bool skip_mc_msrs;
	u32 mc_msrs[3];
	u64 old_vals[3][3];
} lhv_guest_test_msr_ls_data_t;

static void lhv_guest_test_msr_ls_vmexit_handler(VCPU *vcpu, struct regs *r,
												 vmexit_info_t *info)
{
	lhv_guest_test_msr_ls_data_t *data;
	if (info->vmexit_reason != VMX_VMEXIT_VMCALL) {
		return;
	}
#ifdef __AMD64__
	data = (void *)r->rbx;
#elif defined(__I386__)
	data = (void *)r->ebx;
#else /* !defined(__I386__) && !defined(__AMD64__) */
	#error "Unsupported Arch"
#endif /* !defined(__I386__) && !defined(__AMD64__) */
	switch (r->eax) {
	case 12:
		/*
		 * Check support of machine check MSRs.
		 * On QEMU, 0x402 and 0x403 are writable.
		 * On HP 2540P, 0x400 is writable.
		 */
		if ((rdmsr64(0x179U) & 0xff) > 1 && msr_writable(0x402)) {
			data->skip_mc_msrs = false;
			HALT_ON_ERRORCOND(msr_writable(0x403));
			HALT_ON_ERRORCOND(msr_writable(0x406));
			data->mc_msrs[0] = 0x402U;
			data->mc_msrs[1] = 0x403U;
			data->mc_msrs[2] = 0x406U;
		} else {
			data->skip_mc_msrs = true;
		}
		/* Save old MSRs */
		data->old_vals[0][0] = rdmsr64(0x20aU);
		data->old_vals[0][1] = rdmsr64(0x20bU);
		data->old_vals[0][2] = rdmsr64(0x20cU);
		data->old_vals[1][0] = rdmsr64(MSR_EFER);
		data->old_vals[1][1] = rdmsr64(0xc0000081U);
		data->old_vals[1][2] = rdmsr64(MSR_IA32_PAT);
		if (!data->skip_mc_msrs) {
			data->old_vals[2][0] = rdmsr64(data->mc_msrs[0]);
			data->old_vals[2][1] = rdmsr64(data->mc_msrs[1]);
			data->old_vals[2][2] = rdmsr64(data->mc_msrs[2]);
		}
		/* Set experiment */
		if (!data->skip_mc_msrs) {
			vmcs_vmwrite(vcpu, VMCS_control_VM_exit_MSR_store_count, 3);
			vmcs_vmwrite(vcpu, VMCS_control_VM_exit_MSR_load_count, 3);
			vmcs_vmwrite(vcpu, VMCS_control_VM_entry_MSR_load_count, 3);
		} else {
			vmcs_vmwrite(vcpu, VMCS_control_VM_exit_MSR_store_count, 2);
			vmcs_vmwrite(vcpu, VMCS_control_VM_exit_MSR_load_count, 2);
			vmcs_vmwrite(vcpu, VMCS_control_VM_entry_MSR_load_count, 2);
		}
		HALT_ON_ERRORCOND((rdmsr64(0xfeU) & 0xff) > 6);
		wrmsr64(0x20aU, 0x00000000aaaaa000ULL);
		wrmsr64(0x20bU, 0x00000000deadb000ULL);
		wrmsr64(0x20cU, 0x00000000deadc000ULL);
		if (!data->skip_mc_msrs) {
			wrmsr64(data->mc_msrs[0], 0x2222222222222222ULL);
			wrmsr64(data->mc_msrs[1], 0xdeaddeadbeefbeefULL);
			wrmsr64(data->mc_msrs[2], 0xdeaddeadbeefbeefULL);
		}
		HALT_ON_ERRORCOND(rdmsr64(MSR_IA32_PAT) == 0x0007040600070406ULL);
		vcpu->my_vmexit_msrstore[0].index = 0x20aU;
		vcpu->my_vmexit_msrstore[0].data = 0x00000000deada000ULL;
		vcpu->my_vmexit_msrstore[1].index = MSR_EFER;
		vcpu->my_vmexit_msrstore[1].data = 0x00000000deada000ULL;
		if (!data->skip_mc_msrs) {
			vcpu->my_vmexit_msrstore[2].index = data->mc_msrs[0];
			vcpu->my_vmexit_msrstore[2].data = 0xdeaddeadbeefbeefULL;
		}
		vcpu->my_vmexit_msrload[0].index = 0x20bU;
		vcpu->my_vmexit_msrload[0].data = 0x00000000bbbbb000ULL;
		vcpu->my_vmexit_msrload[1].index = 0xc0000081U;
		vcpu->my_vmexit_msrload[1].data = 0x0000000011111000ULL;
		if (!data->skip_mc_msrs) {
			vcpu->my_vmexit_msrload[2].index = data->mc_msrs[1];
			vcpu->my_vmexit_msrload[2].data = 0x3333333333333333ULL;
		}
		vcpu->my_vmentry_msrload[0].index = 0x20cU;
		vcpu->my_vmentry_msrload[0].data = 0x00000000ccccc000ULL;
		vcpu->my_vmentry_msrload[1].index = MSR_IA32_PAT;
		vcpu->my_vmentry_msrload[1].data = 0x0007060400070604ULL;
		if (!data->skip_mc_msrs) {
			vcpu->my_vmentry_msrload[2].index = data->mc_msrs[2];
			vcpu->my_vmentry_msrload[2].data = 0x6666666666666666ULL;
		}
		break;
	case 16:
		/* Check effects */
		HALT_ON_ERRORCOND(vcpu->my_vmexit_msrstore[0].data ==
						  0x00000000aaaaa000ULL);
		HALT_ON_ERRORCOND(rdmsr64(0x20bU) == 0x00000000bbbbb000ULL);
		HALT_ON_ERRORCOND(rdmsr64(0x20cU) == 0x00000000ccccc000ULL);
		HALT_ON_ERRORCOND(vcpu->my_vmexit_msrstore[1].data ==
						  rdmsr64(MSR_EFER));
		HALT_ON_ERRORCOND(rdmsr64(MSR_IA32_PAT) == 0x0007060400070604ULL);
		HALT_ON_ERRORCOND(rdmsr64(0xc0000081U) == 0x0000000011111000ULL);
		if (!data->skip_mc_msrs) {
			HALT_ON_ERRORCOND(vcpu->my_vmexit_msrstore[2].data ==
							  0x2222222222222222ULL);
			HALT_ON_ERRORCOND(rdmsr64(data->mc_msrs[1]) ==
							  0x3333333333333333ULL);
			HALT_ON_ERRORCOND(rdmsr64(data->mc_msrs[2]) ==
							  0x6666666666666666ULL);
		}
		/* Reset state */
		vmcs_vmwrite(vcpu, VMCS_control_VM_exit_MSR_store_count, 0);
		vmcs_vmwrite(vcpu, VMCS_control_VM_exit_MSR_load_count, 0);
		vmcs_vmwrite(vcpu, VMCS_control_VM_entry_MSR_load_count, 0);
		wrmsr64(MSR_IA32_PAT, 0x0007040600070406ULL);
		wrmsr64(0x20aU, data->old_vals[0][0]);
		wrmsr64(0x20bU, data->old_vals[0][1]);
		wrmsr64(0x20cU, data->old_vals[0][2]);
		wrmsr64(MSR_EFER, data->old_vals[1][0]);
		wrmsr64(0xc0000081U, data->old_vals[1][1]);
		wrmsr64(MSR_IA32_PAT, data->old_vals[1][2]);
		if (!data->skip_mc_msrs) {
			wrmsr64(data->mc_msrs[0], data->old_vals[2][0]);
			wrmsr64(data->mc_msrs[1], data->old_vals[2][1]);
			wrmsr64(data->mc_msrs[2], data->old_vals[2][2]);
		}
		break;
	default:
		HALT_ON_ERRORCOND(0 && "Unknown argument");
		break;
	}
	vmcs_vmwrite(vcpu, VMCS_guest_RIP, info->guest_rip + info->inst_len);
	vmresume_asm(r);
}

static void lhv_guest_test_msr_ls(VCPU *vcpu)
{
	if (__LHV_OPT__ & LHV_USE_MSR_LOAD) {
		lhv_guest_test_msr_ls_data_t data;
		vcpu->vmexit_handler_override = lhv_guest_test_msr_ls_vmexit_handler;
		asm volatile ("vmcall" : : "a"(12), "b"(&data));
		asm volatile ("vmcall" : : "a"(16), "b"(&data));
		vcpu->vmexit_handler_override = NULL;
	}
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
	if (__LHV_OPT__ & LHV_USE_EPT) {
		u32 expected_ept_count;
		HALT_ON_ERRORCOND(vcpu->ept_exit_count == 0);
		vcpu->vmexit_handler_override = lhv_guest_test_ept_vmexit_handler;
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
		vcpu->vmexit_handler_override = NULL;
		HALT_ON_ERRORCOND(vcpu->ept_exit_count == expected_ept_count);
		vcpu->ept_exit_count = 0;
	}
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

static void lhv_guest_switch_ept(VCPU *vcpu)
{
	if (__LHV_OPT__ & LHV_USE_SWITCH_EPT) {
		HALT_ON_ERRORCOND(__LHV_OPT__ & LHV_USE_EPT);
		vcpu->vmexit_handler_override = lhv_guest_switch_ept_vmexit_handler;
		asm volatile ("vmcall" : : "a"(17));
		vcpu->vmexit_handler_override = NULL;
	}
}

/* Test VMCLEAR and VMXOFF */
static void lhv_guest_test_vmxoff_vmexit_handler(VCPU *vcpu, struct regs *r,
												 vmexit_info_t *info)
{
	if (info->vmexit_reason != VMX_VMEXIT_VMCALL) {
		return;
	}
	HALT_ON_ERRORCOND(r->eax == 22);
	{
		bool test_vmxoff = r->ebx;
		const bool test_modify_vmcs = true;
		spa_t vmptr;
		u32 old_es_limit;
		u32 new_es_limit = 0xa69f1c74;
		/* Back up current VMCS */
		vmcs_dump(vcpu, 0);
		/* Set ES access right */
		if (test_modify_vmcs) {
			old_es_limit = vmcs_vmread(vcpu, VMCS_guest_ES_limit);
			vmcs_vmwrite(vcpu, VMCS_guest_ES_limit, new_es_limit);
		}
		/* Test VMPTRST */
		HALT_ON_ERRORCOND(__vmx_vmptrst(&vmptr));
		HALT_ON_ERRORCOND(vmptr == hva2spa(vcpu->my_vmcs));

		/* VMCLEAR current VMCS */
		HALT_ON_ERRORCOND(__vmx_vmclear(hva2spa(vcpu->my_vmcs)));
		/* Make sure that VMWRITE fails */
		HALT_ON_ERRORCOND(!__vmx_vmwrite(0x0000, 0x0000));

		/* Run VMXOFF and VMXON */
		if (test_vmxoff) {
			u32 result;
			HALT_ON_ERRORCOND(__vmx_vmxoff());
			asm volatile ("1:\r\n"
						  "vmwrite %2, %1\r\n"
						  "xor %%ebx, %%ebx\r\n"
						  "jmp 3f\r\n"
						  "2:\r\n"
						  "movl $1, %%ebx\r\n"
						  "jmp 3f\r\n"
						  ".section .xcph_table\r\n"
#ifdef __AMD64__
						  ".quad 0x6\r\n"
						  ".quad 1b\r\n"
						  ".quad 2b\r\n"
#elif defined(__I386__)
						  ".long 0x6\r\n"
						  ".long 1b\r\n"
						  ".long 2b\r\n"
#else /* !defined(__I386__) && !defined(__AMD64__) */
	#error "Unsupported Arch"
#endif /* !defined(__I386__) && !defined(__AMD64__) */
						  ".previous\r\n"
						  "3:\r\n"
						  : "=b"(result)
						  : "r"(0UL), "rm"(0UL));

			/* Make sure that VMWRITE raises #UD exception */
			HALT_ON_ERRORCOND(result == 1);

			HALT_ON_ERRORCOND(__vmx_vmxon(hva2spa(vcpu->vmxon_region)));
		}

		/*
		 * Find ES access right in VMCS and correct it. Here we need to assume
		 * that the hardware stores the VMCS field directly in a 4 byte aligned
		 * location (i.e. no encoding, no encryption etc).
		 */
		if (test_modify_vmcs) {
			u32 i;
			u32 found = 0;
			for (i = 2; i < PAGE_SIZE_4K / sizeof(new_es_limit); i++) {
				if (((u32 *) vcpu->my_vmcs)[i] == new_es_limit) {
					((u32 *) vcpu->my_vmcs)[i] = old_es_limit;
					found++;
				}
			}
			HALT_ON_ERRORCOND(found == 1);
		}

		/* Make sure that VMWRITE still fails */
		HALT_ON_ERRORCOND(!__vmx_vmwrite(0x0000, 0x0000));

		/* Restore VMCS using VMCLEAR and VMPTRLD */
		HALT_ON_ERRORCOND(__vmx_vmclear(hva2spa(vcpu->my_vmcs)));
		{
			u64 basic_msr = vcpu->vmx_msrs[INDEX_IA32_VMX_BASIC_MSR];
			u32 vmcs_revision_identifier = basic_msr & 0x7fffffffU;
			HALT_ON_ERRORCOND(*((u32 *) vcpu->my_vmcs) ==
							  vmcs_revision_identifier);
		}
		HALT_ON_ERRORCOND(__vmx_vmptrld(hva2spa(vcpu->my_vmcs)));
		/* Make sure all VMCS fields stay the same */
		{
			struct _vmx_vmcsfields a;
			memcpy(&a, &vcpu->vmcs, sizeof(a));
			vmcs_dump(vcpu, 0);
			HALT_ON_ERRORCOND(memcmp(&a, &vcpu->vmcs, sizeof(a)) == 0);
		}
	}
	vmcs_vmwrite(vcpu, VMCS_guest_RIP, info->guest_rip + info->inst_len);
	/* Hardware thinks VMCS is not launched, so VMLAUNCH instead of VMRESUME */
	vmlaunch_asm(r);
}

static void lhv_guest_test_vmxoff(VCPU *vcpu, bool test_vmxoff)
{
	if (__LHV_OPT__ & LHV_USE_VMXOFF) {
		vcpu->vmexit_handler_override = lhv_guest_test_vmxoff_vmexit_handler;
		asm volatile ("vmcall" : : "a"(22), "b"((u32)test_vmxoff));
		vcpu->vmexit_handler_override = NULL;
	}
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

static void lhv_guest_test_vpid(VCPU *vcpu)
{
	if (__LHV_OPT__ & LHV_USE_VPID) {
		vcpu->vmexit_handler_override = lhv_guest_test_vpid_vmexit_handler;
		asm volatile ("vmcall" : : "a"(19));
		vcpu->vmexit_handler_override = NULL;
	}
}

/*
 * Wait for interrupt in hypervisor mode, nop when LHV_NO_EFLAGS_IF or
 * LHV_NO_INTERRUPT.
 */
static void lhv_guest_wait_int_vmexit_handler(VCPU *vcpu, struct regs *r,
											  vmexit_info_t *info)
{
	if (info->vmexit_reason != VMX_VMEXIT_VMCALL) {
		return;
	}
	HALT_ON_ERRORCOND(r->eax == 25);
	if (!(__LHV_OPT__ & (LHV_NO_EFLAGS_IF | LHV_NO_INTERRUPT))) {
		asm volatile ("sti; hlt; cli;");
	}
	vmcs_vmwrite(vcpu, VMCS_guest_RIP, info->guest_rip + info->inst_len);
	vmresume_asm(r);
}

static void lhv_guest_wait_int(VCPU *vcpu)
{
	vcpu->vmexit_handler_override = lhv_guest_wait_int_vmexit_handler;
	asm volatile ("vmcall" : : "a"(25));
	vcpu->vmexit_handler_override = NULL;
}

/* Test unrestricted guest by disabling paging */
static void lhv_guest_test_unrestricted_guest(VCPU *vcpu)
{
	(void)vcpu;
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
}

/* Test large page in EPT */
static void lhv_guest_test_large_page(VCPU *vcpu)
{
	(void)vcpu;
	if (__LHV_OPT__ & LHV_USE_LARGE_PAGE) {
		HALT_ON_ERRORCOND(__LHV_OPT__ & LHV_USE_EPT);
		HALT_ON_ERRORCOND(large_pages[0][0] == 'B');
		HALT_ON_ERRORCOND(large_pages[1][0] == 'A');
	}
}

/* Test running TrustVisor */
static void lhv_guest_test_user_vmexit_handler(VCPU *vcpu, struct regs *r,
											   vmexit_info_t *info)
{
	if (info->vmexit_reason != VMX_VMEXIT_VMCALL) {
		return;
	}
	HALT_ON_ERRORCOND(r->eax == 33);
	vmcs_vmwrite(vcpu, VMCS_guest_RIP, info->guest_rip + info->inst_len);
	memcpy(&vcpu->guest_regs, r, sizeof(struct regs));
	/* After user mode ends, just run vmresume_asm(&vcpu->guest_regs); */
	enter_user_mode(vcpu, 0);
	HALT_ON_ERRORCOND(0 && "Should never return");
}

static void lhv_guest_test_user(VCPU *vcpu)
{
	if (__LHV_OPT__ & LHV_USER_MODE) {
		HALT_ON_ERRORCOND(!(__LHV_OPT__ & LHV_NO_EFLAGS_IF));
		vcpu->vmexit_handler_override = lhv_guest_test_user_vmexit_handler;
		asm volatile ("vmcall" : : "a"(33));
		vcpu->vmexit_handler_override = NULL;
	}
}

/* Main logic to call subsequent tests */
void lhv_guest_main(ulong_t cpu_id)
{
	u64 iter = 0;
	bool in_xmhf = false;
	VCPU *vcpu = _svm_and_vmx_getvcpu();
	HALT_ON_ERRORCOND(cpu_id == vcpu->idx);
	{
		u32 eax, ebx, ecx, edx;
		cpuid(0x46484d58U, &eax, &ebx, &ecx, &edx);
		in_xmhf = (eax == 0x46484d58U);
	}
	{
		console_vc_t vc;
		console_get_vc(&vc, vcpu->idx, 1);
		console_clear(&vc);
		for (int i = 0; i < vc.width; i++) {
			for (int j = 0; j < 2; j++) {
#ifndef __DEBUG_VGA__
				HALT_ON_ERRORCOND(console_get_char(&vc, i, j) == ' ');
#endif /* !__DEBUG_VGA__ */
				console_put_char(&vc, i, j, '0' + vcpu->id);
			}
		}
	}
	if (!(__LHV_OPT__ & LHV_NO_EFLAGS_IF)) {
		asm volatile ("sti");
	}
	while (1) {
		/* Assume that iter never wraps around */
		HALT_ON_ERRORCOND(++iter > 0);
		if (in_xmhf) {
			printf("CPU(0x%02x): LHV in XMHF test iter %lld\n", vcpu->id, iter);
		} else {
			printf("CPU(0x%02x): LHV test iter %lld\n", vcpu->id, iter);
		}
		if (!(__LHV_OPT__ & (LHV_NO_EFLAGS_IF | LHV_NO_INTERRUPT))) {
			asm volatile ("hlt");
		}
		if (in_xmhf && (__LHV_OPT__ & LHV_USE_MSR_LOAD) &&
			(__LHV_OPT__ & LHV_USER_MODE)) {
			/*
			 * Due to the way TrustVisor is implemented, cannot change MTRR
			 * after running pal_demo. So we need to disable some tests.
			 */
			if (iter < 3) {
				lhv_guest_test_msr_ls(vcpu);
			} else if (iter == 3) {
				/* Implement a barrier and make sure all CPUs arrive */
				static u32 lock = 1;
				static volatile u32 arrived = 0;
				printf("CPU(0x%02x): enter LHV barrier\n", vcpu->id);
				spin_lock(&lock);
				arrived++;
				spin_unlock(&lock);
				while (arrived < g_midtable_numentries) {
					asm volatile ("pause");
				}
				printf("CPU(0x%02x): leave LHV barrier\n", vcpu->id);
			} else {
				lhv_guest_test_user(vcpu);
			}
		} else {
			/* Only one of the following will execute */
			lhv_guest_test_msr_ls(vcpu);
			lhv_guest_test_user(vcpu);
		}
		lhv_guest_test_ept(vcpu);
		lhv_guest_switch_ept(vcpu);
		lhv_guest_test_vpid(vcpu);
		if (iter % 5 == 0) {
			lhv_guest_test_vmxoff(vcpu, iter % 3 == 0);
		}
		lhv_guest_test_unrestricted_guest(vcpu);
		lhv_guest_test_large_page(vcpu);
		lhv_guest_wait_int(vcpu);
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
#ifdef __DEBUG_QEMU__
	case 0x27:
		/* Workaround to make LHV runnable on Bochs */
		printf("CPU(0x%02x): Warning: Mysterious IRQ 7 in guest mode\n",
			   _svm_and_vmx_getvcpu()->id);
		break;
#endif /* __DEBUG_QEMU__ */
	default:
		printf("Guest: interrupt / exception vector %ld\n", vector);
		HALT_ON_ERRORCOND(0 && "Guest: unknown interrupt / exception!\n");
		break;
	}
}
