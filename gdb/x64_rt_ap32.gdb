# Stop at AP running 32-bit code (_ap_clear_pipe)

source gdb/x64_rt_pre.gdb

b xmhf_baseplatform_arch_x86_wakeupAPs
c
d
b *0x10000 + (uintptr_t)&_ap_clear_pipe - (uintptr_t)&_ap_bootstrap_start
c

