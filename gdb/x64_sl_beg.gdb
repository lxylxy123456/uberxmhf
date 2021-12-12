# Stop at beginning of SL (after translating back address)

source gdb/x64_sl_pre.gdb

# Assume `jmpq    *%rax` is 2 bytes
b *(_sl_skip_page_table_change - 2)
c
d
symbol-file
si
symbol-file -o 0 xmhf/src/xmhf-core/xmhf-secureloader/sl_syms.exe

