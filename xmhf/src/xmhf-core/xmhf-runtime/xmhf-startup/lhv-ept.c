#include <xmhf.h>
#include <lhv.h>

// static u64 TODO[MAX_VCPU_ENTRIES];

u64 lhv_build_ept(VCPU *vcpu)
{
	u64 low = rpb->XtVmmRuntimePhysBase;
	u64 high = low + rpb->XtVmmRuntimeSize;
	(void) low;
	(void) high;
	(void) vcpu;
	HALT_ON_ERRORCOND((void *)hptw_insert_pmeo != (void *)hptw_get_pmo_alloc);
	return 0xdead0000;
}

