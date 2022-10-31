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

### Pebbles memory management code bug

TODO: test lhv in Dell and HP 2540p
TODO: report bug in pebbles kernel `lmm_add_free()`

