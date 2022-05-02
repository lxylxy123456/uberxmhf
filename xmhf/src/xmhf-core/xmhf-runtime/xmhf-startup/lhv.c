#include <xmhf.h>

void lhv_main(VCPU *vcpu)
{
	printf("\nCPU(0x%02x): not implemented", vcpu->id);
	HALT_ON_ERRORCOND(0);
}

