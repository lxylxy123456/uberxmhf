# The Mysterious Exception 0x27 in LHV

## Scope
* Dell
* Bochs
* All subarchs
* `lhv ce69c270c`
* `lhv-nmi fa93291f4`

## Behavior
When running LHV or lhv-nmi in Dell, see "The Mysterious Exception 0x27". Even
after handling this exception, the exception still appears.

This problem is first observed when running lhv-nmi in Bochs, see `bug_090`.

## Debugging

### EPT exit qualification

When running LHV on Dell, see a new assertion error. The usual qualification is
0x181, but now it becomes 0x581. This can be explained by reading Intel v3
"Table 26-7. Exit Qualification for EPT Violations". The additional bits are
set due to support to "advanced VM-exit information for EPT violations". I
changed LHV in `lhv de25814dc` to ignore these bits.

### The Mysterious Exception 0x27

First confirm scope of this bug. Build with following and running on Dell
* `./build.sh i386 --lhv-opt 0x5fd && gr`: reproducible (many IRQ 7's)
* `./build.sh i386 --lhv-opt 0x7fd && gr`: still reproducible, but only have 1
  mysterious IRQ 7 in host mode
* `./build.sh i386 --lhv-opt 0 && gr`: reproducible (many IRQ 7's)

If we replace the use of timer in `xmhf_baseplatform_arch_x86_udelay()` and
`udelay()` with RDTSC (`lhv-dev 79ddd3db5`), the problem still happens:
```c
	if (1) {
		u64 start = rdtsc64();
		printf("Replace sleep with RDTSC\n");
		while (rdtsc64() - start < (u64)usecs * 3500) {
			xmhf_cpu_relax();
		}
		return;
	}
```

When trying to run pebbles kernel, see panic in memory calculation. This also
happens on my own kernel.

![pebbles1.png](bug_105/pebbles1.png)

This happens because `lmm_add_free()` is called with a very small `size` such
that `assert(max >= min);` fails. I tried to the function body if the assertion
fails, but for some reason looks like later malloc will fail.

In the future, should report this problem to 410 course staff. But for now,
looks like we can workaround this problem by removing the modules loaded by
GRUB multiboot. After that, p2 can run well. However, p3 receives #GP(0x13b).
Note that 0x13b = 39 * 8 + 3, which looks like the mysterious IRQ 7. See my P4
code `sys_gp()` (closed source).

This means that my p3 timer driver is probably wrong. Now future directions
include:
* Review my p1 / p3 implementation
* Reverse engineer provided p2 kernel (use XMHF)
* Look for other implementations of p1 / p3

### Debugging using XMHF

Currently it looks like p2 is good, but p3 is bad. We modify XMHF a little bit
to print I/O and interrupt events. Start work on `xmhf64-dev 75bebf67f`.

In `xmhf64-dev b47cd27cc`, perform the following
* emulate IN and OUT instructions for port < 0x100
* emulate interrupts (may incorrectly reorder interrupts)
* print exceptions other than #PF and halt

Running the p2 provided kernel is serial `20221030195815`, running the p3 my
kernel is serial `20221030195818`. The observations are
* Before booting the kernel (in GRUB), interrupts come in pair: 0x80000008,
  0x8000000f. This should be the interrupt vector before remapping the vectors.
* In my kernel, two interrupts are received: 0x80000020, 0x80000027. Then while
  injecting the latter interrupt, `#GP(0x13b)` happens due to IDT entry for
  interrupt 0x27 not present.
* In the p2 provided kernel, interrupts still come in pair: 0x80000020,
  0x80000027. So I think the pathos kernel still receives the spurious
  interrupts, but the interrupts are handled silently.

In pathos kernel, the timer interrupt cycle looks like:
```
Interrupt 0x80000020		XMHF gets timer interrupt
Inject interrupt			Inject interrupt to guest
Interrupt 0x80000027		XMHF gets spurious interrupt
outb port=0x0020 al=0x20	Guest sends EOI to PIC
Inject interrupt			Inject interrupt to guest
outb port=0x0020 al=0x20	Guest sends EOI to PIC
```

At this point, I think that the mysterious IRQ 7 should be considered normal.
It should be handled, probably following OSdev wiki
<https://wiki.osdev.org/8259_PIC#Spurious_IRQs>. The Pathos kernel may be
handling it incorrectly.

Currently the logic to handle IRQ 7 is already implemented, but there is a
deadlock:
* CPU 0 is already printing something, holding printf lock
* Interrupt 0x27 comes
* CPU 0 executes `printf("CPU(0x%02x): Warning: Mysterious IRQ 7 in ...`
* CPU 0 tries to acquire printf lock

I realized that the deadlock happens because the serial output stucks at:
```
...
CPU(0x00): LHV test iter 70
CPU(0x00): Warning: Mysterious IRQ 7 in guest mode
CPU(0x00): can enter user mode
CPU(0x00): Warning: Mysterious IRQ 7 in host mode
CPU(0x00): LHV test iter 71
CPU(0x00): Warning: Mysterious IRQ 7 in guest mode
CPU(0x00): can enter user mode
CPU(0x00): Warning: Mysterious IRQ 7 in host mode
CPU(0x00): LHV 
```

To fix LHV, we simply remove the warning about IRQ 7. Fixed in `lhv aa934304c`.
After that, LHV with `--lhv-opt=0x5fd` runs well on Dell. Running LHV in
`xmhf-nest` also works well (all amd64).

Tried and untried ideas
* Run pebbels kernel, see what happens
* Review tboot `calibrate_tsc()`
* See `bug_090`
* Is pebbles kernel computing the wrong time?

### lhv-nmi bug

The fix on `lhv` is manually cherry picked to `lhv-nmi 1c7692ae3`.

lhv-nmi has another bug. It is assuming that the first AP has APIC number 1,
but on Dell it is 2. There was a fix `lhv-nmi fa93291f4` but is incomplete.
Another fix is `lhv-nmi 6b44e3a46`. After that, lhv-nmi can run well on Dell.
xmhf64 (amd64) and lhv-nmi (i386) also runs well.

### Merge xmhf64 to lhv

Observed that `xmhf_baseplatform_arch_x86_udelay()` and `udelay()` are very
similar. In `abcabc04f..28ce646f2`, remove the latter and use the former.

In `lhv aa934304c..83723a519`, merged a lot of commits from `xmhf64` to `lhv`.
Fixed a few regressions due to the merge.

Testing LHV
* x86 lhv: good
* x64 lhv: good
* x86 xmhf, x86 lhv: good
* x64 xmhf, x86 lhv: good
* x64 xmhf, x64 lhv: good

### Easier way to boot kernel in QEMU

Looks like there are much easier ways to boot a kernel that supports multiboot
in QEMU. To boot XMHF, use the following (however, cannot load the guest GRUB):
```sh
gunzip -k hypervisor-x86-i386.bin.gz
qemu-system-x86_64 -cpu Haswell,vmx=yes -enable-kvm -m 1G -kernel init-x86-i386.bin -serial stdio -initrd hypervisor-x86-i386.bin,/dev/null
```

To boot pebbles kernel, use
```sh
qemu-system-x86_64 -kernel kernel.strip
```

### Pebbles memory management code bug

We start debugging lmm code for pebbles kernel. By setting a break point at
`lmm_add_free()`, we can see the call stack during boot

```
#0  lmm_add_free (lmm=0x3d94e4 <malloc_lmm>, block=0x500, size=6912) at 410kern/lmm/lmm_add_free.c:30
#1  0x00109c05 in mb_util_lmm (mbi=0x2a020, lmm=0x3d94e4 <malloc_lmm>) at 410kern/boot/util_lmm.c:139
#2  0x00100aff in mb_entry (info=0x2a020, istack=0x3d807c <cursor_visible>) at 410kern/entry.c:40
#3  0x00100a3b in _start () at 410kern/boot/head.S:120
```

By editing GRUB configurations, looks like this bug is reproducible when 2 or
more multiboot modules are loaded. For example:
```
menuentry "XMHF-build32lhv-2" {
	set root='(hd0,msdos5)'         # point to file system for /
	set kernel='/boot/xmhf/build32lhv/init.bin'
	echo "Loading ${kernel}..."
	multiboot ${kernel} serial=115200,8n1,0xf0a0
	module --nounzip (hd0)+1        # where grub is installed
	module --nounzip (hd0)+1        # where grub is installed
}
```

Add the following to `int putbyte(char ch)` to print to serial port, very
convenient
```
// #define PORT 0x3F8
#define PORT 0xf0a0
	// https://wiki.osdev.org/Serial_Ports
	while ((inb(PORT + 5) & 0x20) == 0);
	outb(PORT, ch);
```

See `util_lmm2.diff` for patch to `util_lmm.c`. Serial `20221031123823`. Can
see that the bug happens because:
* `m[0].string = 0x000100c8`, `strlen(m[0].string) = 0`,
  `m[1].string = 0x000100cc`, leaving a small hole within an aligned 8 bytes.
* `lmm_add_free(&malloc_lmm, (void *) min, max - min);` is called with
  `min = 0x000100c9`, `max = 0x000100cc`
* In `lmm_add_free()`, `min` is aligned up (becomes `0x000100d0`) and `max` is
  aligned down (becomes `0x000100c8`)
* `assert(max >= min);` fails

This bug should be reproducible on QEMU (or even Simics). We just need to add
a lot of modules with short arguments. Indeed, this bug is reproduced using
XMHF's grub generation program and repeating `module --nounzip (hd0)+1` 4
times. The p2 reference kernel also crashes.

To edit GRUB in 15410's environment, edit `410kern/menu.lst`. This is GRUB 1
syntax, <https://uberxmhf.org/docs/pc-intel-x86_32/installing.html> may be
helpful.

However, looks like this bug cannot be easily reproduced in GRUB 1. After using
`modulenounzip (fd0)+1`, all strings are put together without gap in between.
The same happens when using QEMU `-kernel kernel -initrd 'a,a ,a'`.

To let other people reproduce this bug, it looks like we need to use GRUB2 I
have. I can use existing `c.img`, then split it into the 1M MBR header and an
ext4 file system. Then use Linux to modify the file system.
```sh
dd if=c.img of=a.img count=1M iflag=count_bytes
dd if=c.img of=b.img skip=1M iflag=skip_bytes
mkdir -p mnt
mount b.img mnt
sudo cp .../kernel.strip mnt/boot/
umount mnt/
dd if=b.img of=c.img seek=1M oflag=seek_bytes
```

### Pathos IRQ 7 handling bug

We try to reverse engineer Pathos kernel and see how EOI is called when
handling the IRQ 7.

Using the Pathos kernel provided for F22,
* `IRQ[0x20].base = 0x101c48 <KORlKYfQujCzh>` is timer interrupt
* `IRQ[0x27].base = 0x101cd8 <JcqPZpQmVMFE>` is the IRQ 7

Using `objdump -d kernel | grep out`, we see that `0x10d9db <PEpE+9>` is the
OUT instruction.

Using GDB, to set breakpoints at `KORlKYfQujCzh` and `PEpE+9`, we see how EOI
is called during normal timer interrupt.
```
(gdb) bt
#0  0x0010d9db in PEpE ()
#1  0x0010f5c4 in cZmYwuzgoCnOdWkwwMHfsJaVhN ()
#2  0x001017db in cPHavRDPp ()
#3  0x00101c75 in KORlKYfQujCzh ()
#4  0x0054ef70 in ?? ()
#5  0x00102c8c in hEo ()
#6  0x00000020 in ?? ()
#7  0x00000000 in ?? ()
(gdb) 
```

At `KORlKYfQujCzh`, we modify `EIP` to `JcqPZpQmVMFE` using GDB. Then we see
how EOI is called in the same way:
```
(gdb) c
Continuing.

Breakpoint 1, 0x00101c48 in KORlKYfQujCzh ()
(gdb) p $eip = &JcqPZpQmVMFE
$4 = (void (*)()) 0x101cd8 <JcqPZpQmVMFE>
(gdb) c
Continuing.

Breakpoint 2, 0x0010d9db in PEpE ()
(gdb) bt
#0  0x0010d9db in PEpE ()
#1  0x0010f5c4 in cZmYwuzgoCnOdWkwwMHfsJaVhN ()
#2  0x001015dc in OrFbdlCA ()
#3  0x00101d05 in JcqPZpQmVMFE ()
#4  0x0054ef70 in ?? ()
#5  0x00102c8c in hEo ()
#6  0x00000027 in ?? ()
#7  0x00000000 in ?? ()
(gdb) 
```

On Nov 1, the lmm bug and IRQ 7 possible bug are reported to staff-410. Will
track updates in `bug_076`.

## Fix

`lhv ce69c270c..83723a519`
* Update EPT VMEXIT test for new hardware
* Handle IRQ 7 correctly
* Merge updates from `xmhf64`

`lhv-nmi fa93291f4..a733de787`
* Handle IRQ 7
* Remove assumption about APIC ID of first AP = 1

