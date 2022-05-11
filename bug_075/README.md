# lhv SMP cannot access console

## Scope
* All subarchs
* lhv running in XMHF
* SMP

## Behavior
When running LHV in XMHF in QEMU with multiple CPUs, LHV cannot access VGA
memory mapped IO (address 0xb8000). The error is
```
BSP rallying APs...
BSP(0x00): My ESP is 0x08b58000
APs all awake...Setting them free...
AP(0x01): My ESP is 0x08b5c000, proceeding...
Fatal: Halting! Condition 'console_get_char(&vc, i, j) == ' '' failed, line 21, file lhv.c

Fatal: Halting! Condition 'console_get_char(&vc, i, j) == ' '' failed, line 21, file lhv.c
```

The direct cause of the assertion error above is that after clearing memory,
everything should be ' ' (space). However, reading from the memory gives '\0'.

From GDB, looks like the memory space becomes full of 0s.

When there is only 1 CPU, things work well.

## Debugging

By intuition, I think maybe something bad happens when virtualizing APIC (as
the guest's INIT-SIPI-SIPI are forwarded).

When setting up EPT in `_vmx_setupEPT()`, things looks good. 0xb8000 is UC as
expected.

### GDB printing memory

When not running virtual machines, GDB can print the screen's content.
```
(gdb) p (*(char*)0xb8000)@80
$6 = " \a \aB\ao\ao\at\ai\an\ag\a \a`\aX\aM\aH\aF\a-\ai\a3\a8\a6\a'\a \a \a \a \a \a \a \a \a \a \a \a \a \a \a \a \a \a \a \a"
(gdb) 
```

This works the same when UP LHV in XMHF. But in SMP LHV in XMHF, it becomes
```
(gdb) p (*(char*)0xb8000)@80
$1 = '\000' <repeats 79 times>
(gdb) t 2
```

At this time, the screen contains "Booting 'XMHF-i386'...". So probably guest
virtual address at 0xb8000 is not mapped to host physical address 0xb8000.

This bug is strange because Linux works well. When removing `quiet` and adding
`console=tty` in Linux's boot line, Linux can print to console correctly after
waking up APs.

```
(gdb) p (*(char*)0xb8000)@80
Cannot access memory at address 0xb8000
(gdb) p (*(char*)0xc00b8000)@80
$1 = "[\a \a \a \a \a0\a.\a5\a9\a5\a0\a1\a4\a]\a \ax\a8\a6\a:\a \aB\ao\ao\at\ai\an\ag\a \aS\aM\aP\a \ac\ao\an\af\ai\ag\au\ar\a"
(gdb) info th
  Id   Target Id                    Frame 
* 1    Thread 1.1 (CPU#0 [halted ]) 0x00000000c17f9c56 in ?? ()
  2    Thread 1.2 (CPU#1 [halted ]) 0x00000000c17f9c56 in ?? ()
  3    Thread 1.3 (CPU#2 [running]) 0x00000000c143058d in ?? ()
  4    Thread 1.4 (CPU#3 [halted ]) 0x00000000c17f9c56 in ?? ()
(gdb) 
```

Added a new branch `lhv-dev` that contains some temporary commits.

TODO: check whether `xmhf_baseplatform_arch_x86vmx_wakeupAPs()` breaks things
TODO: test on real hardware
TODO: check guest page table and EPT (in LHV one CPU performs VMCALL, then XMHF walks page table)
TODO: try to flush TLB / EPT's TLB / cache
TODO: what happens if LHV's BSP does not wake up APs?
TODO: maybe we can debug QEMU when using GDB to dump memory

