# Jump out of NOP loop
# C: asm volatile("stc; 1: pause; jc 1b; nop" : : : "cc");

p $eflags &= ~1
c
