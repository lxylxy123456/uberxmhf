#include <xmhf.h>
#include <lhv.h>

// TODO: optimize by caching

void vmcs_vmwrite(VCPU *vcpu, ulong_t encoding, ulong_t value)
{
	(void) vcpu;
	printf("\nCPU(0x%02x): vmwrite(0x%04lx, 0x%08lx)", vcpu->id, encoding, value);
	HALT_ON_ERRORCOND(__vmx_vmwrite(encoding, value));
}

ulong_t vmcs_vmread(VCPU *vcpu, ulong_t encoding)
{
	unsigned long value;
	(void) vcpu;
	HALT_ON_ERRORCOND(__vmx_vmread(encoding, &value));
	printf("\nCPU(0x%02x): 0x%08lx = vmread(0x%04lx)", vcpu->id, value, encoding);
	return value;
}
