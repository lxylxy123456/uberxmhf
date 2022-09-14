#include "xmhf.h"
#include "nested-x86vmx-lru.h"

typedef enum xmhf_dbg_eventlog_t {
#define DEFINE_EVENT_FIELD(name, ...) \
	XMHF_DBG_EVENTLOG_##name,
#include "event_logger_fields.h"
	XMHF_DBG_EVENTLOG_SIZE,
} xmhf_dbg_eventlog_t;

void xmhf_dbg_log_event(VCPU *vcpu, xmhf_dbg_eventlog_t event, void *key);

