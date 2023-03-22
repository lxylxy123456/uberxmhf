#include <stdint.h>

#define RECORD_TYPE_REGISTER	0
#define RECORD_TYPE_MARSHALL	1
#define RECORD_TYPE_UNMARSHALL	2
#define RECORD_TYPE_UNREGISTER	3

void record_time(uint64_t ts, uint64_t te, int type);

