# Support guest hypervisor setting EPT

## Scope
* All subarchs
* Git branch `xmhf64-nest` commit `acad9ef02`
* Git branch `lhv` commit `d6eea8116`

## Behavior
Currently for the guest hypervisor running in XMHF, EPT is not supported.
This need to be changed in order to run unrestricted nested guests.

## Debugging

First try to remove bss from XMHF's compressed image, at least in build option.
In lhv `6ff70ad37`, xmhf64-nest `2e2a3c300`, the build and run time is
decreased.

First use EPT in lhv.

Using the hpt library to construct EPT automatically. When compiling LHV,
encountered strange error during linking. In xmhf64 `8b5de43bf`, removed
`hpt_internal.c` (duplicate content compared to `hpt.c`).

In lhv `7128d975d`, EPT looks like

```
(gdb) x/gx ept_root
0x8bb4000:	0x0000000008bbc007
(gdb) ...
0x8bbc000:	0x0000000008bbd007
(gdb) ...
0x8bbd040:	0x0000000000000000	0x0000000008bbe007
(gdb) ...
0x8bbe1a0:	0x0000000001234007	0x0000000000000000
(gdb) 
```

My code is following TrustVisor's `scode.c`. So this should also be
TrustVisor's behavior. However, note that the EPT's memory type is set to UC
(uncached), which is not good.

Now we debug TrustVisor.

Break at `scode_register()`, and trigger `pal_demo` in Debian.

Can see that before entering TrustVisor, memory type is WB (6).
```
(gdb) p/x vcpu->vmcs.control_EPT_pointer 
$1 = 0x1467401e
(gdb) x/2gx 0x14674000
0x14674000 <g_vmx_ept_pml4_table_buffers>:	0x000000001467c007	0x0000000000000000
(gdb) x/2gx 0x000000001467c000
0x1467c000 <g_vmx_ept_pdp_table_buffers>:	0x0000000014684007	0x0000000014685007
(gdb) x/2gx 0x0000000014684000
0x14684000 <g_vmx_ept_pd_table_buffers>:	0x00000000146a4007	0x00000000146a5007
(gdb) x/2gx 0x00000000146a4000
0x146a4000 <g_vmx_ept_p_table_buffers>:	0x0000000000000037	0x0000000000001037
(gdb) 
```

Then break at `hpt_scode_switch_scode()` and wait until this function ends.
However, the memory types are correct (WB, 6).

```
(gdb) p/x vcpu->vmcs.control_EPT_pointer 
$10 = 0x1030e01e
(gdb) x/2gx 0x1030e000
0x1030e000:	0x000000001030f007	0x0000000000000000
(gdb) ...
0x1030f000:	0x0000000010310007	0x0000000000000000
...
0x10310250:	0x0000000010313007	0x0000000000000000
...
0x10313850:	0x000000000950a035	0x0000000000000000
...
0x10313930:	0x0000000009526033	0x0000000000000000
0x10313940:	0x0000000000000000	0x0000000009529033
(gdb) 
```

I guess the EPT pages are copied from the large EPT, so the memory type bits
are not changed. I also realized that `hpt_pme_set_pmt()` and
`hpt_pmeo_setcache()` can be used to set the memory type to WB.

Now the memory type is correct:

```
0x8bbe1a0:	0x0000000001234037	0x0000000000000000
```

Also encountered triple fault after enabling EPT in i386 lhv. The cause is the
same as in `bug_079`. Since i386 LHV uses PAE, the PDPTE needs to be populated.

In `5eb712013`, LHV contains non-trivial use of EPT.

### Understand TrustVisor

I don't recall how TrustVisor makes sure that when CPU 1 is running PAL, CPU
2's EPT is set correctly so that CPU 2 cannot access the PAL.

Actually, this problem is solved in `did_change_root_mappings` in `scode.c`.
When TrustVisor is first started, all CPUs are changed to use the same EPT.
Then when changing EPT entry for one CPU will automatically apply to other
CPUs.

### Planning EPT support

It may become a challenge to support EPT shadowing and changing EPT on the fly
efficiently.

Re-read the Turtles Project paper. Note that the EPT02 is built on the fly,
driven by EPT exits. The paper also says that the memory EPT12 uses need to be
set to read only in EPT01.

A blog that is possibly helpful:
<https://royhunter.github.io/2014/06/20/NESTED-EPT/>

Possible features to be supported
* Different EPT01 between CPUs
	* This means that when EPT01 is changed, need to notify other CPUs to
	  update their EPT01. Currently it is supported by XMHF, but TrustVisor
	  uses the same EPT01 between CPUs.
* Allow invalid entries in EPT12 but not accessed
	* This means that we need shadow EPT
* Modifying EPT12 in L1 guest after VMENTRY
	* If cacheing (see below), may need to invalidate cache
* Modifying EPT12 in L2 guest
	* Should be able to assume that this never exists, or there is a security
	  problem for L1. Should also assume that paddr of EPT12 does not appear in
	  EPT12.
* Modifying EPT12 in L1 guest in another CPU
	* A good hypervisor should synchronize one CPU's use of EPT and another
	  CPU's access to EPT. So probably CPU 1 is not using the EPT when CPU 2
	  is modifying it.
* Support dirty and accessed flags in EPT12
	* It is possible for this feature to be unavailable by setting
	  `IA32_VMX_EPT_VPID_CAP`

Suppose EPT01 of CPU 1 is different from CPU 2. Then during a VMENTRY or EPT
exit in CPU 1, CPU 2's access to CPU 1's EPT12 need to be removed.

The key idea of translation is
* EPT02(x) = EPT01(EPT12(x))
* EPT01(x) = ...
	* ... invalid if x is memory used by EPT12
	* ... x else

Mapping from entity to events may occur are:
* L0
	* Change EPT01 (e.g. TrustVisor)
* L1
	* Change EPT12
* L2
	* Change EPT12 (if support)
	* Change EPT02 (accessed bit, dirty bit)

Possible ways to implement
* Translate all mappings / translate only EPT exit entries
* Cache EPT02 between L2 -> L1 -> L2 / do not cache EPT02

Suppose we are first implementing only translate EPT exit entries and without
cache. We track what need to be done after cache.

Mapping from events to other events they cause are:
* Add entry to EPT01
	* N/A (will update EPT02 when EPT exit)
* Remove entry in EPT01
	* Remove all relevant entry in EPT02 (need to search), or invalidate EPT02
* Add entry in EPT12
	* N/A (will update EPT02 when EPT exit)
	* Remove address of EPT12 from EPT01
* Remove entry in EPT12
	* Remove entry in EPT02
* Change EPT02 accessed / dirty bit
	* Update EPT12 relevant bits, or make EPT12 not readable in EPT01

So current implementation plan is:
* No caching on EPT02
* When L1 calls VMENTRY, enter L2 with empty EPT
* L2 -> L0 due to EPT, populate EPT and L0 -> L2
* When L2 -> L0 -> L1, invalidate EPT02 and update accessed / dirty for EPT12

### Another possible TrustVisor security problem

TrustVisor synchronizes EPT by using the same EPT across different CPUs.
However, it does not flush the EPT TLB using `__vmx_invept()` on all CPUs. This
may allow CPU 2 to access memory of PAL on CPU 1.

To test, we need to be able to run TrustVisor in a controlled environment.
Probably LHV is the best choice.

In lhv-dev `94275e47a` and xmhf64-dev `b4d6e3435`, can run PAL in LHV.

Wrote the test script in `ba6290f05`. However, it does not work in QEMU KVM.
I think maybe KVM's nested virtualization has different EPT TLB behavior
compared to real hardware. We try Bochs.

### Bochs

After the GRUB boot image work, booting on Bochs becomes easy.

To boot LHV (no VMX), copy `/usr/share/doc/bochs/bochsrc-sample.txt` to
`bochsrc` and modify the following.

```diff
759c759,760
< ata0-master: type=disk, mode=flat, path="30M.sample"
---
> #ata0-master: type=disk, mode=flat, path="30M.sample"
> ata0-master: type=disk, mode=flat, path="c.img"
901a903
> com1: enabled=1, mode=file, dev=/dev/stdout
955c957
< sound: driver=default, waveout=/dev/dsp. wavein=, midiout=
---
> #sound: driver=default, waveout=/dev/dsp. wavein=, midiout=
```

The modified configuration looks like
```
cpu: model=core2_penryn_t9600, count=1, ips=50000000, reset_on_triple_fault=1, ignore_bad_msrs=1, msrs="msrs.def"
memory: guest=512, host=256
romimage: file=$BXSHARE/BIOS-bochs-latest, options=fastboot
vgaromimage: file=$BXSHARE/VGABIOS-lgpl-latest
ata0: enabled=1, ioaddr1=0x1f0, ioaddr2=0x3f0, irq=14
ata1: enabled=1, ioaddr1=0x170, ioaddr2=0x370, irq=15
ata0-master: type=disk, mode=flat, path="d.img"
boot: disk
log: bochsout.txt
panic: action=ask
error: action=report
info: action=report
debug: action=ignore, pci=report # report BX_DEBUG from module 'pci'
debugger_log: -
com1: enabled=1, mode=file, dev=/dev/stdout
```

To run VMX, need to re-compile Bochs. Trying different configurations

```
./configure \
            --enable-all-optimizations \
            --enable-cpu-level=6 \
            --enable-x86-64 \
            --enable-vmx=2 \
            --enable-clgd54xx \
            --enable-busmouse \
            --enable-show-ips \
            \
            --enable-smp \
            --disable-docbook \
            --with-x --with-x11 --with-term --with-sdl2 \
            \
            --prefix=USR_LOCAL
```

Using CPU `model=corei7_sandy_bridge_2600k` seems working. Note that
`IA32_VMX_VMFUNC_MSR` is not supported, so need to decrease `IA32_VMX_MSRCOUNT`
if using LHV.

To boot XMHF and then LHV, save XMHF as `c.img` and LHV as `d.img`, then use
configuration

```
cpu: model=corei7_sandy_bridge_2600k, count=2, ips=50000000, reset_on_triple_fault=1, ignore_bad_msrs=1, msrs="msrs.def"
memory: guest=512, host=256
romimage: file=$BXSHARE/BIOS-bochs-latest, options=fastboot
vgaromimage: file=$BXSHARE/VGABIOS-lgpl-latest
ata0: enabled=1, ioaddr1=0x1f0, ioaddr2=0x3f0, irq=14
ata1: enabled=1, ioaddr1=0x170, ioaddr2=0x370, irq=15
ata0-master: type=disk, mode=flat, path="c.img"
ata1-master: type=disk, mode=flat, path="d.img"
boot: disk
log: bochsout.txt
panic: action=ask
error: action=report
info: action=report
debug: action=ignore, pci=report # report BX_DEBUG from module 'pci'
debugger_log: -
com1: enabled=1, mode=file, dev=/dev/stdout
```

However, the possible security problem cannot be reproduced in Bochs. Either
Bochs does not fully implement the EPT TLB (i.e. will be reproducible on real
hardware), or the Intel manual leaves the possibility of the bug, but real
hardware implementation does not have the bug (i.e. will not be reproducible
on real hardware). Of course, maybe I am wrong.

Looks like this is the question I want to ask:
* <https://stackoverflow.com/questions/70179745/>

From Intel manual, it looks like TLB shootdown still need to be performed
manually for EPT.

### Experimenting EPT TLB using LHV

In lhv-dev `4f8ccfcf6`, perform the following experiment
1. CPU 0 constructs EPT for all CPUs
2. CPU 1 enters guest mode and access a memory region called `lxy` forever
3. CPU 0 removes `lxy` from EPT
4. CPU 0 accesses `lxy`

What happens on QEMU:
* At step 4, CPU 0 does not error
* CPU 0 halts by design
* Then CPU 1 gets EPT violation

Expected:
* CPU 1 never gets EPT violation

In lhv-dev `977ecf572`, make CPU 0 never halt by design. This time both CPUs
do not get EPT violation.

In lhv-dev `072022e50`, add INVEPT. Can see that after INVEPT, both CPU halts.
Without INVEPT, both CPU runs forever. However, I expect that only CPU 0 halts
if with INVEPT.

In lhv-dev `c55d7aea3`, modify code to be able to run on Bochs. On Bochs,
CPU 1 never gets EPT violation. This is the expected behavior.

According to the experiment, my answer to
<https://stackoverflow.com/questions/70179745/> is correct. i.e. INVEPT only
applies to the current CPU. There needs to be TLB shootdown to synchronize
EPT change between CPUs.

For now, likely not going to worry about reproducing the problem in TrustVisor.
Not having TLB shootdown code in TrustVisor can already be considered a
security problem.

Fixed in `99a7e5aba..aadd462c1`. The TLB shootdown code is already there. Just
request TLB shootdown by setting `g_vmx_flush_all_tlb_signal` to 1.

### KVM code

Using the following script to collect KVM code from Linux

```sh
#!/bin/bash
set -xe
LINUX_DIR=/PATH/TO/linux-5.10.84
mkdir -p arch/x86
cp -a $LINUX_DIR/arch/x86/kvm arch/x86
mkdir -p virt
cp -a $LINUX_DIR/virt/kvm virt
```

Then place the code in OpenGrok. Looks good

### EPT with limited features

We start implementing the EPT anyway. For now forget about the challenged. I
also realized that if we are driven by EPT exits and do not care about access
and dirty bits, maybe we can leave EPT12 intact. The guest should run INVEPT
when it changes EPT entries, and the guest is responsible for TLB shootdown.

In xmhf64-dev `6eb88e97b`, have basic EPT. However, the guest stuck due to not
receiving expected interrupt (probably EOI is lost).

We start by simplifying LHV. In `f75302811`, remove VMCALL and see that guest
mode also stucks. In `8fb399926`, simplify LHV more. We see that if we remove
the EPT violation in `lhv_guest_main()` to address `0x12340000`, then the
problem disappears. So we suspect that the code to forward L2 EPT exit to L1 is
incorrect.

In `599e7668a`, only enable interrupts after L1 handles EPT. Can see that
L1's handling of EPT has a problem. In `69c30a52a`, can run on Bochs. The
result is the same as QEMU. So most likely XMHF is incorrect.

Since there is only one EPT exit that goes to L1, we should be able to replace
it with VMCALL. In lhv-dev `6fc25ee9a` (and xmhf64-dev `ed588a2ba`),
implemented this and the problem is still reproducible.

In lhv-dev `23e417468`, can remove VMCALL. So the experiment in `8fb399926` may
be incorrect. In lhv-dev `4116b8b75`, removed all `L2 -> L0 -> L1` VMEXITs. Now
the behavior is
* Without XMHF, see `Halting! Condition '0' failed, line 368, file lhv-vmx.c`
  because LHV no longer handles VMEXITs
* With xmhf64-nest-dev `ed588a2ba`, LHV stucks forever.

There are 12 VMEXITs at this point:

```
pa=0x08b68208 rip=0x0820bff8 rflags=0x00010002 0  PMEO02=0x08b68037
pa=0x0820bff8 rip=0x0820bff8 rflags=0x00010002 1  PMEO02=0x0820b037
pa=0x08b93ffc rip=0x0820bff8 rflags=0x00010002 2  PMEO02=0x08b93037
pa=0x08200560 rip=0x08200560 rflags=0x00010012 3  PMEO02=0x08200037
pa=0x08b6bfb8 rip=0x082005f5 rflags=0x00010082 4  PMEO02=0x08b6b037
pa=0xfee00020 rip=0x082005f5 rflags=0x00010082 5  PMEO02=0xfee00007
pa=0x08216220 rip=0x08200625 rflags=0x00010046 6  PMEO02=0x08216037
pa=0x08213dac rip=0x0820be7b rflags=0x00010046 7  PMEO02=0x08213037
pa=0x08203ff8 rip=0x08203ff8 rflags=0x00010046 8  PMEO02=0x08203037
pa=0x08204000 rip=0x08203ffe rflags=0x00010016 9  PMEO02=0x08204037
pa=0x000b8050 rip=0x08203f68 rflags=0x00010006 10 PMEO02=0x000b8007
pa=0x082177d8 rip=0x0820bf70 rflags=0x00000246 11 PMEO02=0x08217037
```

In xmhf64-nest-dev `202a35d31`, at the last EPT exit, replace EPT02 with
EPT01. The process still stucks. However, if replace earlier, no longer stucks.
By bisecting, it turns out that if replace at second last EPT exit, the problem
is gone.

Interestingly, the last EPT exit's RIP (`0x0820bf70`) is CLI instruction. pa
is `0x082177d8 <x_gdt_start+8>`. So it is likely that EPT exit indicates that
the exit happens during handling of interrupt / exception, and the hypervisor
needs to re-inject the interrupt / exception.

The related information is in Intel v3
"26.2.4 Information for VM Exits During Event Delivery". For the last ETP exit,
`info_IDT_vectoring_information = 0x80000020`. The structure is in
"Table 23-18. Format of the IDT-Vectoring Information Field" at page 921.
For others, the VMCS field is 0. `info_IDT_vectoring_error_code` should also be
checked.

The correct response should be injecting the event to the CPU, using
`control_VM_entry_interruption_information` and
`control_VM_entry_exception_errorcode`. The sturcture for the first one is in
"Table 23-15. Format of the VM-Entry Interruption-Information Field" on
page 918. We can see that these 2 tables are the same. So just need to copy
the fields.

In `xmhf64-nest-dev 23785cedb`, solve the problem. Cherry-picked to
`xmhf64-nest 54c4697bd`.

Well, actually XMHF checks this in peh:
```c
1403  	//make sure we have no nested events
1404  	if(vcpu->vmcs.info_IDT_vectoring_information & 0x80000000){
1405  		printf("CPU(0x%02x): HALT; Nested events unhandled with hwp:0x%08x\n",
1406  			vcpu->id, vcpu->vmcs.info_IDT_vectoring_information);
1407  		HALT();
1408  	}
```

Untried ideas
* check all bits during EPT exit
	* The cause of this bug would be found if checking
	  `control_VM_entry_interruption_information`
* maybe has something to do with EPT TLB? Though not likely

### Continue working on EPT

In `67bd95800`, check feature bits for EPT and disable unsupported features
(e.g. large page, access / dirty bit).

In `67bd95800..44cfb2b4c`, perform some caching on EPT to make things run fast.
Using LRU caching policy. Cache lab is a useful reference.

In lhv `8b35e400a..09f773f29`, add code to switch EPTP and EPT entries. In
XMHF, need to handle VMEXIT due to INVEPT and INVVPID.

In `xmhf64-nest 8c0f7e841`, realize that using `hptw_get_pmeo()` in
`xmhf_nested_arch_x86vmx_handle_ept02_exit()` is bad, because `hptw_get_pmeo()`
does not check access rights for all 4 levels of paging. We need to write some
new functions in `hptw.c` to achieve this functionality. In
`xmhf64-nest daafed3d0`, fixed by writing `hptw_checked_get_pmeo()` that walks
page table.

Even though we require the guest to not use VPID, there is one caveat: during
all VMEXIT and VMENTRY, the TLB needs to be flushed. See KVM's
`nested_vmx_transition_tlb_flush()`. This is fixed in
`xmhf64-nest daafed3d0..458adf795`.

In `xmhf64-nest 458adf795..3f796487d`, extract the LRU cache algorithm to a
single file. Use macros to achieve a similar effect of C++ templating. This is
similar to 15410's variable queue challenge.

In `lhv 308174b5c`, can enable VPID using `__LHV_OPT__` mask `0x10`. In
`xmhf64-nest de0873065..20dad0089`, support enabling VPID in XMHF (but not
INVVPID yet).

In `lhv c41fe8b62`, use all 4 kinds of INVVPID. In
`xmhf64-nest 20dad0089..e69ca507a`, support all 4 kinds of INVVPID.

### Unrestricted guest

After implementing nested page table, we should be able to enable unrestricted
guest. However, when I try to disable paging in LHV guest in `lhv 6332af28c`, I
get a `#GP`. I suspect that maybe some VMCS settings in LHV is wrong, because
this does not happend when disabling paging in LHV hypervsior.

SDM says that when unrestricted guest is 0, MOV CR0 will `#GP` if unset CR0.PG.

Actually, after some rebuild, this problem is mysteriously solved. Maybe the
unrestricted guest bit was not set properly due to build cache problems.

However, `lhv 73a70bf6e` cannot run properly in `xmhf64-nest 2b1b23cf0`. The
guest can correctly disable paging, but when enable paging, the guest see
`#GP`.

In `xmhf64-nest-dev a3bd8576e` and `lhv-dev b699e994a`, use VMCALL in LHV to
notify XMHF to dump the VMCS. The results are in `20220706130634`. Use the
following script to compare VMCS
```sh
diff <(grep ':guest:' 20220706130634 | cut -d : -f 1-2,4-) \
	<(grep ':host :' 20220706130634 | cut -d : -f 1-2,4-)
```

In `lhv-dev 80a91b614` and `xmhf64-nest-dev e7a54bccb`, modify guest VMCS02 so
that it is almost identical to host VMCS01. Can see that the problem happens in
EPT. Since EPT changes a little bit, the behavior also changes.

The expected behavior is
```
...
CPU(0x00): (0x6c1c) :guest:host_IA32_INTERRUPT_SSP_TABLE_ADDR = unavailable
CPU(0x00): LHV guest can enable paging
```

The actual behavior is
```
CPU(0x00): (0x6c1c) :guest:host_IA32_INTERRUPT_SSP_TABLE_ADDR = unavailable
CPU(0x00): EPT: 0x08214078 0x08214000 0x08214000 RIP=0x0820c4a6
CPU(0x00): EPT: 0x08200c31 0x08200000 0x08200000 RIP=0x08200c31

Fatal: Halting! Condition 'vmcs12->control_IO_BitmapA_address == __vmx_vmread64(0x2000)' failed, line 207, file arch/x86/vmx/nested-x86vmx-vmcs12-fields.h
```

The LHV build result is compressed and saved in `lhv-80a91b614.img.xz`.
Note `LHV_OPT = 0x000000000000003c`.

The 2 EPT violations happen due to the exception. The guest is accessing IDTR
(`0x08214078`) and the first instruction of exception handler (`0x08200c31`).
Then a VMEXIT happens that should go to L1. But since VMCS02 is corrupted, the
nested virtualization code halts. In the expected behavior, there is no VMEXITs
of any kind.

Now we need to bisect the EPT in some way. In `xmhf64-nest-dev 59515e525`,
after working on EPT, looks like the problem is solved when page `0x8b69000` is
mapped. Other mapped pages are `0x08b6a000` and `0x08b95000`. The lower bits of
page table entries are all `0x037`.

Page `0x8b69000` needs read and write. That is, `0x8b69037` and `0x8b69033` can
work. Bug `0x8b69032` and `0x8b69031` cannot.

By searching in VMCS dump, it is clear that the page is
`guest_CR3 = 0x08b69000`. So the question now is that why there is no EPT
violation when accessing `guest_CR3`. We need to start suspecting that KVM has
a bug.

In `xmhf64-nest-dev aab39541f`, do not modify unrelated VMCS fields, so that
the program can reproduce on Bochs. On Bochs, the result is
```
CPU(0x00): (0x6c1a) :guest:host_SSP = unavailable
CPU(0x00): (0x6c1c) :guest:host_IA32_INTERRUPT_SSP_TABLE_ADDR = unavailable
CPU(0x00): EPT: 0x08b69000 0x08b69000 0x08b69000 RIP=0x0820c4a6
CPU(0x00): LHV guest can enable paging
```

This means that it is likely that KVM has a bug.

### Reproducing KVM bug in LHV

In `lhv-dev 2061cd367`, able to reproduce this problem with LHV only. This
makes testing on Bochs faster. Also, print the result on screen so that it is
possible to test on real hardware without serial port. Use `LHV_OPT = 0x3e`.

Tested on Bochs. Bochs prints "GOOD" on serial, but QEMU prints "BAD". Serial
port output in both cases look reasonable.

Tested on Thinkpad, also prints "GOOD". Tested on HP 840's KVM, also BAD (with
reasonable serial port output). So we can confirm that this is a KVM bug.

When reporting the bug, can use `lhv-dev 231a25f7f`. A copy of the compiled
image is in `lhv-231a25f7f.img.xz`.

### Workaround

In `xmhf64-nest-dev b91f26d6b..4e4a752cc`, workaround this bug by always
setting nested guest CR3 in EPT. Squash the commits to
`xmhf64-nest 2b1b23cf0..fdf1ce506`.

### Debugging KVM

Now we try to guess what is happening in KVM. By reading code, I think what
happened is:
```
handle_set_cr0()
	is_guest_mode() -> true
	kvm_set_cr0()
		load_pdptrs()
			kvm_read_guest_page_mmu() -> fail
```

`bug_078` contains knowledge about debugging KVM. I guess that using nested
virtualization and GDB to debug KVM is likely not going to work. So we should
use print debugging. See `#### Compile first` in `bug_078`.

Here is the process to compile Linux, most from `bug_078`

```
wget https://mirrors.edge.kernel.org/pub/linux/kernel/v5.x/linux-5.18.9.tar.xz
tar xaf linux-5.18.9.tar.xz
cd linux-5.18.9/
make nconfig
	# Cryptographic API: Remove key ring
make clean
time make deb-pkg -j `nproc`
	# 32 min 36.344 src, 4.2G
sudo apt-get install ../linux-image-5.18.9_5.18.9-1_amd64.deb

# Reboot using new Linux kernel
# cd to previous directory
# At this point `sudo insmod ./arch/x86/kvm/kvm.ko` etc should work

cd ..
mkdir kvm
cd kvm
tar xaf ../linux-5.18.9.tar.xz
cd linux-5.18.9/
cp ../../linux-5.18.9/.config .
make oldconfig
```

Now modify KVM source code. Print debugging looks like

```c
	printk(KERN_ERR "LXY: %s:%d: %s():", __FILE__, __LINE__, __func__);
```

Then to compile and install, use

```sh
time make modules_prepare -j `nproc`
time make M=arch/x86/kvm -j `nproc`
ls arch/x86/kvm/kvm.ko

sudo rmmod kvm_intel
sudo rmmod kvm
sudo insmod arch/x86/kvm/kvm.ko
sudo insmod arch/x86/kvm/kvm-intel.ko
lsmod | grep kvm
```

View result in dmesg

```sh
sudo dmesg -w | grep LXY
```

By print debugging, we can confirm that the guessed stack trace above is
correct (the one with `load_pdptrs()`). Now it's time to report the bug.

Looks like
<https://lore.kernel.org/all/CABdb7371mXnseJWHYwmnz6rHCzjr=gBrHy1yumRVomeDraNxxg@mail.gmail.com/T/>
is very similar to this bug. However, I think it is a different bug.

Bug reported in <https://bugzilla.kernel.org/show_bug.cgi?id=216212>. Bug
report text in `bug_216212.txt`. Bugs are tracked in `bug_076`.

### Unrestricted guest in AMD64

At `xmhf64-nest ad5e4a7c0`, try to run unrestricted guest in AMD64. In AMD64,
need to switch CS to 32 bits and run 32-bit code, so write most functionality
in assembly. `lhv 73a70bf6e..1a35764bb` makes it possible to disable paging
in 64-bit mode. Running it in KVM looks fine.

Running `lhv 1a35764bb` in XMHF first fails because the 64-bit flag in VMENTRY
control fields may change. This is solved in
`xmhf64-nest ad5e4a7c0..ea7e59c79`.

However, now when running LHV in XMHF for a long time, see strange
`L2 -> L0 -> L1` EPT error. This is reproducible when only 1 CPU, but looks
to reproduce faster when multiple CPUs. The serial port shows:

```
...
CPU(0x00): EPT: 0x0821c130 0x0821c000 0x0821c000
CPU(0x00): EPT: 0x08220910 0x08220000 0x08220000
CPU(0x00): EPT: 0x08216110 0x08216000 0x08216000
CPU(0x00): EPT: 0x082011ad 0x08201000 0x08201000
HPT[3]:.../xmhf/src/libbaremetal/libxmhfutil/hptw.c:hptw_checked_get_pmeo:384: EU_CHK( ((access_type & hpt_pmeo_getprot(pmeo)) == access_type) && (cpl == HPTW_CPL0 || hpt_pmeo_getuser(pmeo))) failed
HPT[3]:.../xmhf/src/libbaremetal/libxmhfutil/hptw.c:hptw_checked_get_pmeo:384: req-priv:3 req-cpl:0 priv:0 user-accessible:1
CPU(0x00): nested vmexit 48
```

Then there is a deadlock between LHV hypervisor wanting to print unknown EPT
violation and the LHV guest already holding the printf lock.

Looks like the EPT violation is very strange. The `paddr` is the 0th page, and
RIP points to some benign-looking instruction.

```
(gdb) up 4
#4  0x000000000820908e in vmexit_handler (vcpu=0x8217da0 <g_vcpubuffers>, r=0x96dbf78 <g_cpustacks+65400>) at lhv-vmx.c:501
501				printf("CPU(0x%02x): ept: 0x%08lx\n", vcpu->id, q);
(gdb) x/i vcpu->vmcs.guest_RIP
   0x82011c5 <XtRtmIdtStub10+24>:	addl   $0x20,0x58(%rsp)
(gdb) p vcpu->vmcall_exit_count
$15 = 699
(gdb) p/x r->rsp
$16 = 0x96dbfd8
(gdb) p/x paddr
$17 = 0x73
(gdb) p/x vaddr
$18 = 0x73
(gdb) p/x vcpu->vmcs.guest_RSP
$19 = 0x8d9bf7c
(gdb) 
```

Still reporducible in `lhv-dev 375ffac4d` and `xmhf64-nest-dev b17ddaa0a`. We
get a VMCS dump in XMHF (L0). Notice that `guest_RSP` is not 16-bytes aligned.
Also remember that `guest_TR_base = 0` because we were lazy. Also note that we
are in `XtRtmIdtStub10`, which is the exception handler for `#TS Invalid TSS`.
So I guess that when disabling paging, an interrupt caused the problem. For
example:

```
	/* Jump to long mode */
	pushl	$(__CS)
								// Interrupt hits here
	pushl	$1f
	lretl
1:
```

I do not have a full story of how this happens (because detailed RSP value may
vary). But for now, it is better to disable interrupts in
`lhv_disable_enable_paging()`.

In `lhv-dev 375ffac4d..fa58f9b58`, implemented disabling interrupts. With
`xmhf64-nest-dev b17ddaa0a`, it looks like problem is solved.

The production versions are `xmhf64-nest ea7e59c79` and `lhv 15dd393c4`. Looks
good. Also, the KVM problem no longer happens in AMD64, so it is likely that
only PAE paging has the KVM problem. This is expected.

TODO: implement real mode guest
TODO: study KVM code, maybe use older version
TODO: study shadow page table, maybe use Xen code
TODO: encountering memory size limit: on QEMU currently runtime has to < 256M

