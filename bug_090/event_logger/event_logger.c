#include "event_logger.h"

typedef struct event_log_t {
	u32 total_count;
#define DEFINE_EVENT_FIELD(name, ...) \
	u32 count_##name;
#include "event_logger_fields.h"
} event_log_t;

event_log_t global_event_log[MAX_VCPU_ENTRIES];

void xmhf_dbg_log_event(VCPU *vcpu, xmhf_dbg_eventlog_t event) {
	/* Get event log */
	event_log_t *event_log = &global_event_log[vcpu->idx];
	/* Increase count */
	event_log->total_count++;
	switch (event) {
#define DEFINE_EVENT_FIELD(name, ...) \
	case XMHF_DBG_EVENTLOG_##name: \
		event_log->count_##name++; \
		break;
#include "event_logger_fields.h"
	default:
		HALT_ON_ERRORCOND(0 && "Unknown event");
	}
	/* Print result */
	if (event == XMHF_DBG_EVENTLOG_vmexit_rdmsr) {
		printf("CPU(0x%02x): %s begin\n", vcpu->id, __func__);
		printf("CPU(0x%02x): total_count = %d\n", vcpu->id,
			   event_log->total_count);
		event_log->total_count = 0;
#define DEFINE_EVENT_FIELD(name, ...) \
		printf("CPU(0x%02x): count_" #name " = %d\n", vcpu->id, \
			   event_log->count_##name); \
		event_log->count_##name = 0;
#include "event_logger_fields.h"
		printf("CPU(0x%02x): %s end\n", vcpu->id, __func__);
	}
}

