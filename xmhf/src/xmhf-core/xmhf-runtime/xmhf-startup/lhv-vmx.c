#include <xmhf.h>
#include <lhv.h>

#define MAX_GUESTS 4

static u8 all_vmxon_region[MAX_VCPU_ENTRIES][PAGE_SIZE_4K]
__attribute__(( section(".bss.palign_data") ));

static u8 all_vmcs[MAX_VCPU_ENTRIES][MAX_GUESTS][PAGE_SIZE_4K]
__attribute__(( section(".bss.palign_data") ));

void lhv_vmx_main(VCPU *vcpu)
{
	u32 vmcs_revision_identifier;

	/* Discover support for VMX (22.6 DISCOVERING SUPPORT FOR VMX) */
	{
		u32 eax, ebx, ecx, edx;
		cpuid(1, &eax, &ebx, &ecx, &edx);
		HALT_ON_ERRORCOND(ecx & (1U << 5));
	}

	/* Allocate VM (23.11.5 VMXON Region) */
	{
		u32 eax, edx;
		rdmsr(IA32_VMX_BASIC_MSR, &eax, &edx);
		vmcs_revision_identifier = eax & 0x7fffffffU;
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
		u64 msr_efcr = rdmsr64(MSR_EFCR);
		printf("\nrdmsr64(MSR_EFCR) = 0x%016x", msr_efcr);
		HALT_ON_ERRORCOND(msr_efcr & 1);
		HALT_ON_ERRORCOND(msr_efcr & 4);
	}

	/* VMXON */
	{
		__vmx_vmxon(hva2spa(vcpu->vmxon_region));
	}

	/* VMCLEAR, VMPTRLD */
	{
		vcpu->my_vmcs = all_vmcs[vcpu->idx][0];
		HALT_ON_ERRORCOND(__vmx_vmclear(hva2spa(vcpu->my_vmcs)));
		*((u32 *) vcpu->my_vmcs) = vmcs_revision_identifier;
		HALT_ON_ERRORCOND(__vmx_vmptrld(hva2spa(vcpu->my_vmcs)));
	}

	printf("\nCPU(0x%02x): Not implemented", vcpu->id);
}
