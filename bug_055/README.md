# GRUB 

## Scope
* HP only (QEMU works well)
* DRT only (no DRT works well)
* GRUB graphical mode only (GRUB text mode works well)
* x86 and x64 XMHF

## Behavior
When using GRUB graphics mode, GRUB does not appear the second time.

The last few lines of serial are:
```
CPU(0x00): INT 15(e820): AX=0xe820, EDX=0x534d4150, EBX=0x00000010, ECX=0x00000014, ES=0x6800, DI=0x0004
CPU(0x00): INT 15(e820): AX=0xe820, EDX=0x534d4150, EBX=0x00000011, ECX=0x00000014, ES=0x6800, DI=0x0004
CPU(0x00): INT 15(e820): AX=0xe820, EDX=0x534d4150, EBX=0x00000012, ECX=0x00000014, ES=0x6800, DI=0x0004
CPU(0x00): WRMSR (MTRR) 0x0000020c 0x00000000c0000001 (old = 0x00000000c0000001)
```

## Debugging

