#include "event_logger.h"

typedef enum xmhf_eventlog_t {
	XMHF_EL_VMEXIT_0,
} xmhf_eventlog_t;

void xmhf_dbg_log_event(void) {
	printf("xmhf_dbg_log_event()\n");
}

