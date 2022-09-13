#ifndef DEFINE_EVENT_FIELD
#error "DEFINE_EVENT_FIELD must be defined"
#endif

DEFINE_EVENT_FIELD(vmexit_cpuid)
DEFINE_EVENT_FIELD(vmexit_rdmsr)
DEFINE_EVENT_FIELD(vmexit_wrmsr)

#undef DEFINE_EVENT_FIELD
