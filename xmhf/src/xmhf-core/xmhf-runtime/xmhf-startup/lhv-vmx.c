#include <xmhf.h>
#include <lhv.h>

static u8 all_vmxon_region[MAX_VCPU_ENTRIES][PAGE_SIZE_4K]
__attribute__(( section(".bss.palign_data") ));

void lhv_vmx_main(VCPU *vcpu)
{
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
		vcpu->vmxon_region = (void *) all_vmxon_region[vcpu->idx];
		*((u32 *) vcpu->vmxon_region) = (eax & 0x7fffffffU);
	}

	/* Set CR4.VMXE (22.7 ENABLING AND ENTERING VMX OPERATION) */
	{
		ulong_t cr4 = read_cr4();
		HALT_ON_ERRORCOND((cr4 & CR4_VMXE) == 0);
		write_cr4(cr4 | CR4_VMXE);
	}

	/* Check IA32_FEATURE_CONTROL */
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

	printf("\nCPU(0x%02x): Not implemented", vcpu->id);
}
