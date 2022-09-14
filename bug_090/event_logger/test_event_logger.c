#include "event_logger.h"

int main() {
	VCPU vcpu = { .id = 0, .idx = 0 };
	unsigned long key = 0;
	xmhf_dbg_log_event(&vcpu, XMHF_DBG_EVENTLOG_vmexit_cpuid, &key); key = 0;
	xmhf_dbg_log_event(&vcpu, XMHF_DBG_EVENTLOG_vmexit_cpuid, &key); key = 1;
	xmhf_dbg_log_event(&vcpu, XMHF_DBG_EVENTLOG_vmexit_wrmsr, &key); key = 1;
	xmhf_dbg_log_event(&vcpu, XMHF_DBG_EVENTLOG_vmexit_cpuid, &key); key = 1;
	xmhf_dbg_log_event(&vcpu, XMHF_DBG_EVENTLOG_vmexit_wrmsr, &key); key = 1;
	xmhf_dbg_log_event(&vcpu, XMHF_DBG_EVENTLOG_vmexit_rdmsr, &key); key = 2;
	xmhf_dbg_log_event(&vcpu, XMHF_DBG_EVENTLOG_vmexit_cpuid, &key); key = 3;
	xmhf_dbg_log_event(&vcpu, XMHF_DBG_EVENTLOG_vmexit_wrmsr, &key); key = 4;
	xmhf_dbg_log_event(&vcpu, XMHF_DBG_EVENTLOG_vmexit_rdmsr, &key); key = 5;
}

