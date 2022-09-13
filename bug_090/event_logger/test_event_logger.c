#include "event_logger.h"

int main() {
	VCPU vcpu = { .id = 0, .idx = 0 };
	xmhf_dbg_log_event(&vcpu, XMHF_DBG_EVENTLOG_vmexit_cpuid);
	xmhf_dbg_log_event(&vcpu, XMHF_DBG_EVENTLOG_vmexit_cpuid);
	xmhf_dbg_log_event(&vcpu, XMHF_DBG_EVENTLOG_vmexit_wrmsr);
	xmhf_dbg_log_event(&vcpu, XMHF_DBG_EVENTLOG_vmexit_cpuid);
	xmhf_dbg_log_event(&vcpu, XMHF_DBG_EVENTLOG_vmexit_wrmsr);
	xmhf_dbg_log_event(&vcpu, XMHF_DBG_EVENTLOG_vmexit_rdmsr);
	xmhf_dbg_log_event(&vcpu, XMHF_DBG_EVENTLOG_vmexit_cpuid);
	xmhf_dbg_log_event(&vcpu, XMHF_DBG_EVENTLOG_vmexit_wrmsr);
	xmhf_dbg_log_event(&vcpu, XMHF_DBG_EVENTLOG_vmexit_rdmsr);
}

