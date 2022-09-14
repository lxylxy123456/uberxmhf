#ifndef DEFINE_EVENT_FIELD
#error "DEFINE_EVENT_FIELD must be defined"
#endif

/*
 * Arguments of macro DEFINE_EVENT_FIELD:
 * name: Name of the event
 * count_type: type of the cache value (count)
 * count_fmt: format string to print the count
 * lru_size: size of the LRU cache
 * index_type: type of the cache index
 * key_type: type of the cache key
 * key_fmt: format string to print the key
 */

DEFINE_EVENT_FIELD(vmexit_cpuid, u32, "%d", 4, u16, u32, "0x%08x")
DEFINE_EVENT_FIELD(vmexit_rdmsr, u32, "%d", 4, u16, u32, "0x%08x")
DEFINE_EVENT_FIELD(vmexit_wrmsr, u32, "%d", 4, u16, u32, "0x%08x")

#undef DEFINE_EVENT_FIELD
