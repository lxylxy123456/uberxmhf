# Support waking APs in DRT

## Scope
* HP, x64 XMHF, DRT
* Good: x86 XMHF
* Good: no DRT
* Git `92a9e94dc`

## Behavior
After fixing `bug_042`, DRT mode can enter runtime, but when BSP wakes up AP,
no AP shows up and BSP waits forever. Serial ends with following.
```
xmhf_xcphandler_arch_initialize: setting up runtime IDT...
xmhf_xcphandler_arch_initialize: IDT setup done.
Runtime: Re-initializing DMA protection...
Runtime: Protected SL+Runtime (10000000-1da07000) from DMA.
BSP: _mle_join_start = 0x1020bec0, _ap_bootstrap_start = 0x1020be70
BSP: joining RLPs to MLE with MONITOR wakeup
BSP: rlp_wakeup_addr = 0x0
Relinquishing BSP thread and moving to common...
BSP rallying APs...
BSP(0x00): My RSP is 0x000000001d987000
```

## Debugging

Note that when no DRT, BSP uses APIC to wake up APs. But now BSP uses something
else.

The difference can be seen in `xmhf_baseplatform_arch_x86_64vmx_wakeupAPs()`.
We need to compare behavior of x86 and x64 XMHF with DRT.

Can see that in x86 `rlp_wakeup_addr` should be `0xbb701d20`, but in x64 it is
`0x0`. We need to fix this.

Add more print statements in git `d3b19466a`, x64 DRT serial `20220302231320`.
Same flags added in x86 in git `2aba8feba`, x86 DRT serial `20220302231449`.
Can see that most other values make sense.

`sinit_mle_data` are all `0xbb7301c0`, but `sinit_mle_data->rlp_wakeup_addr`
are different. In GDB, can see that `p sizeof(sinit_mle_data_t)` is different.
In x86 is 148, in x64 is 152. This is likely caused by alignment issues.

`sinit_mle_data_t` is a structure defined in TXT MLE developer's guide section
"C.4 SINIT to MLE Data Format". So we add `__attribute__((packed))` and problem
solved.

Git `e187be88d`, serial `20220302232918`. This time see less output. This is
because machine is killed by some triple fault and serial output has not
reached debugger machine yet. We add some sleeps.

Git `026bddfd6`, serial `20220302233758`. Our guess is correct. Remove some
printf in git `7ba6af9ab`, serial `20220302234730`. Can see that write to
`rlp_wakeup_addr` causes the problem. Likely APs are up at this point.

Note that in above situations, after the error the machine reboots
automatically. APs are likely to start in `bplt-x86_64-smptrampoline.S`'s
`_ap_bootstrap_start()`. If we add an infinite loop there, may find something
useful. Another possibility is that the AP's start address of 0x10000 is not
passed correctly to TXT. But I think this is unlikely. See TXT MLE dev guide
"2.1 MLE Architecture Overview" -> "Table 1. MLE Header structure" and
`mle_hdr_t` in XMHF.

We can see that `mle_join` in `xmhf_baseplatform_arch_x86_64vmx_wakeupAPs()`
is defined in `_mle_join_start()` (assembly code). The assembly code points to
`_ap_clear_pipe()`. However adding a infinite loop at `_ap_clear_pipe()` does
not solve the problem.

Note that at this point BSP already has IDT set up, so I think the triple fault
likely comes from AP.

### Bochs

Debugging triple fault on HP is very difficult. Can we get a CPU debugger?

Looks like SENTER instructions are called "SMX instruction set", which is
related to Intel TXT. The documentation for SMX should be intel volume 2
"CHAPTER 6 SAFER MODE EXTENSIONS REFERENCE"
* Ref: <http://linasm.sourceforge.net/docs/instructions/smx.php>

Looks like Bochs supports SMX instructions, but not sure whether it supports
TXT. If it suppors, we can try Bochs because what we need to debug is low level
(e.g. the guest OS does not need to be up)
* Ref: <https://bochs.sourceforge.io/cgi-bin/lxr/source/docs-html/cpu_configurability.txt>

This can be a future direction.

### Review struct size

Git `e211ec67a`, we add a use of all structs of `_txt*.h` in the XMHF runtime
code. This way GDB recognizes the symbol. Then we use `txt1.gdb` to print
all their size. Find 2 mismatches, all caused by alignment problems
* `bios_data_t`: 36 (x86) vs 40 (x64)
	* Intel's specification is not aligned ("C.1 BIOS Data Format")
* `os_sinit_data_t`: 92 (x86) vs 96 (x64)
	* Intel's specification is not aligned ("C.3 OS to SINIT Data Format")

There is also another unaffected struct `sinit_mdr_t` in `_txt_heap.h`, but
the structure is defined by Intel, so we still add alignment
("Table 22. SINIT Memory Descriptor Record")

So we add alginment on these fields. Git `a08f148a1`, alginment problems fixed.
However, still see `BSP: rlp_wakeup_addr = 0xbb701d20` and system reboots.

### Merging x86 and x64
At this point, `bug_049` was done, so a lot of syntactic change to the code
base is done.

After the change, did some tests.

Git `966a26747`, x86 DRT serial `20220315124017`, x64 serial `20220315131234`.

### Error code
Can see that `TXT.ERRORCODE` printed is related to status of previous boot

### Try `GETSEC[WAKEUP]`
Git `cfeb01982`, x64 serial `20220315132349`. In this commit, use
`__getsec_wakeup()` instead of `sinit_mle_data->rlp_wakeup_addr`. However,
looks like APs are not woken up.

### Literature review
In tboot code, see (`tboot-1.10.4/docs/tboot_flow.md`)
> There a few requirements for platform state before GETSEC[SENTER] can be
> called:
> CPU has to be in protected mode

In Intel TXT guide "2.2 MLE Launch":
> The new MP startup algorithm would not
> require the RLPs to leave protected mode with paging on.

The above is the only mention of "protected mode" in Intel guide.

An answer in <https://stackoverflow.com/questions/32223339/how-does-uefi-work>
says:
> GETSEC can only be executed in protected mode

However, I think the answer means `GETSEC[EXITAC]` can only be executed in
protected mode.

In Intel v2 "CHAPTER 6 SAFER MODE EXTENSIONS REFERENCE", can see that some
instructions are only supported in protected mode, some support long mode.

In tboot and XMHF code, can see `/* must be in protected mode */`. However,
this check only checks CR0.PE. So long mode will also pass this check.

tboot's configure file seems to support x64. Need to look into tboot's build
process.

Not sure whether Intel TXT supports long mode, because the documentation is
vague.

### Building tboot from source

Running `make` in `tboot-1.10.4` will build automatically. Some files are
compiled with `-m32` and some with `-m64`.

The deciding comment is in `tboot-1.10.4/tboot/Config.mk`:
> `# if target arch is 64b, then convert -m64 to -m32 (tboot is always 32b)`

Can also verify by seeing whether the `.o` files are ELF 32-bit or ELF 64-bit.
Can see that the object files are 32-bit iff the file is located in `tboot/`.

So now should consider porting sl back to x86

Not tested ideas
* compare x86 and x64 logs (why is there `TXT.ERRORCODE=80000000`?)
* does join data structure need to be 64 bits for x64?
* try to use x86 version of `_ap_bootstrap_start()`
* try `__getsec_wakeup()` (do not use `rlp_wake_monitor`), or use ACPI
* how is BSP loaded? According to TXT docs AP should be similar
* read TXT docs
* try Bochs
* how is `g_sinit_module_ptr` used?
* study tboot
* maybe write a TXT program from scratch
* in an extreme case, set up tboot for x64 and rewrite it to xmhf

### XMHF structure change plan

The structure of tboot is something like:
* BSP is secure booted
* BSP uses GETSEC to boot APs
* BSP wait until all APs to enter wait-for-sipi, then simulate an SIPI
  (related: `ap_wake_addr`, `cpu_wakeup()` calls `_prot_to_real()`)

Everything above happens in protected mode.

Current structure of XMHF64 (for XMHF, just no jump to long mode):
```
                                       +----+      +----+    +------------+
long                                   | sl | jump | rt |    |     rt     |
mode                                   |    | ---> |    | -> | bsp and ap |
                                       +----+      +----+    +------------+
                                         A            |              A
                                     ASM |       (AP) |              | ASM
      (BSP)                              |            |              |
     +------+           +----+        +---+           | GETSEC   +-----+
prot | bios | multiboot | bl | GETSEC |sl |           +--------> | rt  |
mode |      | --------> |    | -----> |pre|           |          | smp |
     +------+           +----+  JMP   +---+      INIT |          +-----+
                                                 SIPI |           A
                                                 SIPI |           | ASM
                                                      V           |
                                                     +-------------+
real                                                 |     rt      |
mode                                                 |     smp     |
                                                     +-------------+
```

* BIOS jumps to bootloader (bl), already in protected mode
* bl goes to secureloader (sl)
	* When DRT, use Intel TXT / GETSEC
	* When no DRT, simply jump
* sl first runs `sl-x86-i386-entry.S`, switch to long mode if x64 XMHF
* sl jumps to runtime (rt)
* rt wakes up AP
	* When DRT, use Intel TXT / GETSEC, result in protected mode
	* When no DRT, use APIC INIT-SIPI-SIPI sequence, result in real mode
* AP switch to long mode, start running with BSP

However, when rt wakes AP up, the tboot code we have only supports x86.

Other constraints are that sl can only be compiled in x86 or x64. Runtime have
to be compiled in x64. boot loader is already in x86 and will likely stay the
same.

So the plan is to change sl to x86, and move AP wake up code into sl.

Proposed structure of new XMHF64
```
                                                                         +----+
long                                                                     | rt |
mode                                                                     |    |
                                                                         +----+
                                                                          A
                                                                          | ASM
      (BSP)                                                               |
     +------+           +----+        +----+        +------------+    +----+
prot | bios | multiboot | bl | GETSEC | sl |  BSP   |     sl     | -> | rt |
mode |      | --------> |    | -----> |    | -----> | bsp and ap | -> | pre|
     +------+           +----+  JMP   +----+ GETSEC +------------+    +----+
                                         |    (AP)   A
                          INIT-SIPI-SIPI |           | ASM
                                    (AP) V           |
                                        +-------------+
real                                    |     sl      |
mode                                    |     smp     |
                                        +-------------+
```

* BIOS jumps to bootloader (bl), already in protected mode
* bl goes to secureloader (sl)
	* When DRT, use Intel TXT / GETSEC
	* When no DRT, simply jump
* sl runs in protected mode now. It wakes up APs
	* When DRT, use Intel TXT / GETSEC, result in protected mode
	* When no DRT, use APIC INIT-SIPI-SIPI sequence, result in real mode
* If using INIT-SIPI-SIPI, AP switches to protected mode and enters sl
* All CPUs jump from sl to run time
* In the start of run time, all CPUs enter long mode
* Run time C code runs in long mode

Major changes to be made
* Wake up AP code: move from rt to sl
* AP set up code: move from rt to sl
* BSP jump to long mode code: move from sl to rt
* RT entry: add an argument to distinguish BSP and AP
* Compile sl in x86

Challenges
* sl trusted area currently has a 64KB size limit. Around 10355 bytes left.

### Implementing AP wakeup in sl

The code is rooted in `xmhf_smpguest_initialize()`.

Asm code for SL entry (not very related): `sl-x86-i386-entry.S`.
Asm code for AP born: `bplt-x86-i386-smptrampoline.S`.

TODO: still try jumping back to protected mode in RT

# tmp notes

Port sl to x86

## Fix

`78f0eef8e..3ae6989ba`, `4d6217bd5..` (a08f148a1)
* Fix type size in `xmhf_baseplatform_arch_x86_64vmx_wakeupAPs()`
* Fix `x86_64` alignment problem for `_txt_heap.h`

