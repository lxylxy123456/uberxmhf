# QEMU flood NMI causes Linux stuck

## Scope
* QEMU only, because HP does not support manually injecting NMIs
* bad: x64
* not tested: x86

## Behavior
See also: `bug_018` Test 4 and Failure B

After booting x64 XMHF x64 Debian 11, inject NMIs at c using:
```
python3 bug_018/qemu_inject_nmi.py 4444 0
```

After a few seconds stop injecting, and Linux fails to respond. In gdb, load
Linux's symbols and see:
```
(gdb) x/i $rip
=> 0xffffffff81a01480 <asm_exc_nmi+240>:	iretq  
(gdb) x/6gx $rsp
0xfffffe000000dfd8:	0xffffffff81a01480	0x0000000000000010
0xfffffe000000dfe8:	0x0000000000000082	0xfffffe000000dfd8
0xfffffe000000dff8:	0x0000000000000018	Cannot access memory at address 0xfffffe000000e000
(gdb) 
```

This is an infinite loop. iretq will jump back to itself.

## Debugging

In serial port, can only see `CPU(0x00): inject NMI` a lot of times. Try to
print RSP and RIP of the guest.

Modified `qemu_inject_nmi.py`, try to inject only a finite number of NMIs

500:
```
      2 CPU(0x00): inject NMI, 0xffffffff818c3eee, 0xffffffff82603eb8
      2 CPU(0x00): inject NMI, 0xffffffff8106b3e6, 0xffffc90000003f50
      4 CPU(0x00): inject NMI, 0xffffffff818c3eee, 0xffffffff82603eb8
      3 CPU(0x00): inject NMI, 0xffffffff8102efe2, 0xfffffe000000dea0
     13 CPU(0x00): inject NMI, 0xffffffff818c3eee, 0xffffffff82603eb8
     16 CPU(0x00): inject NMI, 0xffffffff814ca765, 0xfffffe000000dd80
      8 CPU(0x00): inject NMI, 0xffffffff8153b1f9, 0xffffc90000003b48
     14 CPU(0x00): inject NMI, 0xffffffff8102efe2, 0xfffffe000000dea0
     43 CPU(0x00): inject NMI, 0xffffffff81a01390, 0xfffffe000000dfd8
     47 CPU(0x00): inject NMI, 0xffffffff81a01393, 0xfffffe000000dfd8
      1 CPU(0x00): inject NMI, 0xffffffff81a01393, 0xfffffe000000dfd8
```

300:
```
      3 CPU(0x00): inject NMI, 0xffffffff818c3eee, 0xffffffff82603eb8
     12 CPU(0x00): inject NMI, 0xffffffff81a01390, 0xfffffe000000dfd8
      1 CPU(0x00): inject NMI, 0xffffffff8102efe2, 0xfffffe000000dea0
      6 CPU(0x00): inject NMI, 0xffffffff8106b316, 0xfffffe000000dcc0
      8 CPU(0x00): inject NMI, 0xffffffff8102efe2, 0xfffffe000000dea0
     33 CPU(0x00): inject NMI, 0xffffffff81a01390, 0xfffffe000000dfd8
      4 CPU(0x00): inject NMI, 0xffffffff81a01393, 0xfffffe000000dfd8
     13 CPU(0x00): inject NMI, 0xffffffff81a01390, 0xfffffe000000dfd8
      1 CPU(0x00): inject NMI, 0xffffffff81a01390, 0xfffffe000000dfd8
```

`0xffffffff81a01390` is `asm_exc_nmi`, looks like pointed by the IDT entry. So
either the hypervisor injected NMI before the guest executed IRET, or something
related to nested NMI.

The related Linux source code is `arch/x86/entry/entry_64.S`, need to read.

Also can see that `0xffffffff81a01480` is the `iretq` instruction after
`nested_nmi_out`. So we need to understand how nested NMIs work. Also found
good article:
<https://0xax.gitbooks.io/linux-insides/content/Interrupts/linux-interrupts-6.html>

After changing code to `d6ba03929`, now NMI windowing intercept will not unset
the NMI windowing bit for 10 times. Now serial output shows:
```
CPU(0x00): inject NMI, 0xffffffff818c3eee, 0xffffffff82603eb8
CPU(0x00): inject NMI, 0xffffffff818c3eee, 0xffffffff82603eb8
CPU(0x00): inject NMI, 0xffffffff818c3eee, 0xffffffff82603eb8
CPU(0x00): inject NMI, 0xffffffff818c3eee, 0xffffffff82603eb8
CPU(0x00): inject NMI, 0xffffffff818c3eee, 0xffffffff82603eb8
CPU(0x00): inject NMI, 0xffffffff818c3eee, 0xffffffff82603eb8
CPU(0x00): inject NMI, 0xffffffff818c3eee, 0xffffffff82603eb8
CPU(0x00): inject NMI, 0xffffffff818c3eee, 0xffffffff82603eb8
CPU(0x00): inject NMI, 0xffffffff818c3eee, 0xffffffff82603eb8
CPU(0x00): inject NMI, 0xffffffff818c3eee, 0xffffffff82603eb8
CPU(0x00): inject NMI, 0xffffffff818c3eee, 0xffffffff82603eb8
```

I then realized that maybe the hypervisor needs to set virtual NMI blocking.
Currently hypervisor does not set virtual NMI blocking, so the guest cannot
execute any instructions. Instead, it simply returns to the hypervisor.
* Actually this is not the case. See Intel manual.

### Debugging Linux

Now use gdb to debug Linux, do not start XMHF

```
source gdb/x64_linux_sym.gdb
b *0xffffffff81a01390
```

Labels in asm
```
asm_exc_nmi+242 = first_nmi
asm_exc_nmi+272 = repeat_nmi
asm_exc_nmi+309 = end_repeat_nmi
```

Looks like in normal NMI handling, there will be no nested NMIs at all, because
NMI handler will only execute iret when returning back to non-exception code.

### `bug_113`

This bug still presents in `xmhf64 531c11afd`. See also `bug_113`.

```sh
# Build
./build.sh amd64 fast && gr
# Also reproducible:
./build.sh amd64 fast O3 circleci && gr
# Run KVM
../tmp/bios-qemu.sh --gdb 1234 -d build64 +1 -d debian11x64 -t -qmp tcp:localhost:4444,server,wait=off -smp 1 &
# Run NMI inection script
python3 bug_018/qemu_inject_nmi.py 4444 0
```

TODO: try to inject a finite number of NMIs, likely < 10
TODO: try to modify `VMX_VMEXIT_NMI_WINDOW` to not clear NMI windowing bit
TODO: recompile Linux, reduce NMI handling code
TODO: try on Bochs, if Bochs supports injecting NMI

## Result

Low priority bug, skipped for now

