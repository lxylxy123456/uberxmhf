#include <xmhf.h>
#include <lhv.h>

#define MAX_GUESTS 4

static u8 all_vmxon_region[MAX_VCPU_ENTRIES][PAGE_SIZE_4K]
__attribute__(( section(".bss.palign_data") ));

static u8 all_vmcs[MAX_VCPU_ENTRIES][MAX_GUESTS][PAGE_SIZE_4K]
__attribute__(( section(".bss.palign_data") ));

static void lhv_vmx_vmcs_init(VCPU *vcpu)
{
	HALT_ON_ERRORCOND(vcpu == NULL);
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

	// TODO: modify VMCS
	lhv_vmx_vmcs_init(vcpu);

//	asm volatile ("cli");	// TODO: tmp

	/* VMLAUNCH */
	{
		struct regs r;
		memset(&r, 0, sizeof(r));
		vmlaunch_asm(&r);
	}

	HALT_ON_ERRORCOND(0 && "vmlaunch_asm() should never return");
}

void vmentry_error(ulong_t is_resume, ulong_t valid)
{
	VCPU *vcpu = _svm_and_vmx_getvcpu();
	printf("\nCPU(0x%02x): is_resume = %ld, valid = %ld", vcpu->id,
			is_resume, valid);
	HALT_ON_ERRORCOND(is_resume && valid && 0);
	HALT_ON_ERRORCOND(0);
}
