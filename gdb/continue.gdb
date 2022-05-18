# Jump out of hard-coded GDB break point
# C: asm volatile("xor %%al,%%al; 1: test %%al,%%al; jz 1b;" ::: "%eax", "cc");

p $al = 1

