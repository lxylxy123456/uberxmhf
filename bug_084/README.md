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

TODO: discuss TLB shootdown and security problem in TrustVisor
TODO: implement EPT
TODO: study KVM code, maybe use older version

