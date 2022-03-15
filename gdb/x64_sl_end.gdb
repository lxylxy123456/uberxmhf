# Stop at end of SL (after translating address)

source gdb/x64_sl_beg.gdb

# Local label skip_jump in xmhf_sl_arch_x86_invoke_runtime_entrypoint
# Assume `jmpq    *%rax` is 2 bytes
b *(skip_jump - 2)
c
d
symbol-file
si
symbol-file -o 0x10000000 xmhf/src/xmhf-core/xmhf-secureloader/sl_syms.exe

