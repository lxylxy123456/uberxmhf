#include <xmhf.h>
#include <lhv.h>

void lhv_vmx_main(VCPU *vcpu)
{
	/* Discover support for VMX (22.6 DISCOVERING SUPPORT FOR VMX) */
	{
		u32 eax, ebx, ecx, edx;
		cpuid(1, &eax, &ebx, &ecx, &edx);
		HALT_ON_ERRORCOND(ecx & (1U << 5));
	}

	/* Allocate VM (23.11.5 VMXON Region) */

	printf("\nCPU(0x%02x): Not implemented", vcpu->id);
}
