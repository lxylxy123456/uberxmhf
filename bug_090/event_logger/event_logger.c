#include "event_logger.h"

#define DEFINE_EVENT_FIELD(name, count_type, count_fmt, lru_size, index_type, \
						   key_type, key_fmt, ...) \
LRU_NEW_SET(event_log_##name##_set_t, event_log_##name##_line_t, lru_size, \
			index_type, key_type, count_type);
#include "event_logger_fields.h"

typedef struct event_log_t {
	u32 total_count;
#define DEFINE_EVENT_FIELD(name, count_type, ...) \
	count_type count_##name; \
	event_log_##name##_set_t lru_##name;
#include "event_logger_fields.h"
} event_log_t;

event_log_t global_event_log[MAX_VCPU_ENTRIES];

void xmhf_dbg_log_event(VCPU *vcpu, xmhf_dbg_eventlog_t event, void *key) {
	/* Get event log */
	event_log_t *event_log = &global_event_log[vcpu->idx];
	/* Increase count */
	event_log->total_count++;
	switch (event) {
#define DEFINE_EVENT_FIELD(name, count_type, count_fmt, lru_size, index_type, \
						   key_type, key_fmt, ...) \
	case XMHF_DBG_EVENTLOG_##name: \
		event_log->count_##name++; \
		{ \
			index_type index; \
			bool cache_hit; \
			event_log_##name##_line_t *line; \
			line = LRU_SET_FIND_EVICT(&event_log->lru_##name, \
									  *(key_type *)key, index, cache_hit); \
			if (!cache_hit) { \
				line->value = 0; \
			} \
			line->value++; \
			(void) index; \
		} \
		break;
#include "event_logger_fields.h"
	default:
		HALT_ON_ERRORCOND(0 && "Unknown event");
	}
	/* Print result */
	if (event == XMHF_DBG_EVENTLOG_vmexit_rdmsr) {
		printf("CPU(0x%02x): %s begin\n", vcpu->id, __func__);
		printf("CPU(0x%02x):  total_count = %d\n", vcpu->id,
			   event_log->total_count);
		event_log->total_count = 0;
#define DEFINE_EVENT_FIELD(name, count_type, count_fmt, lru_size, index_type, \
						   key_type, key_fmt, ...) \
		printf("CPU(0x%02x):  count_" #name " = " count_fmt "\n", vcpu->id, \
			   event_log->count_##name); \
		event_log->count_##name = 0; \
		{ \
			index_type index; \
			event_log_##name##_line_t *line; \
			LRU_FOREACH(index, line, &event_log->lru_##name) { \
				if (line->valid) { \
					printf("CPU(0x%02x):   " key_fmt " count " count_fmt "\n", \
						   vcpu->id, line->key, line->value); \
					line->valid = 0; \
				} \
			} \
		}
#include "event_logger_fields.h"
		printf("CPU(0x%02x): %s end\n", vcpu->id, __func__);
	}
	(void) key;
}

