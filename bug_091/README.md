# Testing on 2540p

## Scope
* HP only
* Planned configurations
	* LHV
	* XMHF LHV
	* XMHF XMHF LHV
	* XMHF XMHF Debian
	* INIT-SIPI-SIPI twice exploit
	* NMI tests (`lhv-nmi` branch)
* Git revisions at start of bug
	* `lhv 93f279dae`
	* `lhv-nmi 3cee6b73a`
	* `xmhf64 983c6bc82`
	* `xmhf64-dev cc6f106f8`

## Behavior
Recently I got access to 2540P, so should test things I developed during the
summer.

## Debugging

### Testing LHV

In HP 2540P, for two CPUs, `vcpu->id != vcpu->idx`. This causes a bug in LHV.
Fixed in `93f279dae..117992a70`.

* i386, `LHV_OPT=0x0`: good
* i386, `LHV_OPT=0x1fd`: bad1
* amd64, `LHV_OPT=0x1fd`: bad1
* i386, `LHV_OPT=0x3d`: bad1
* i386, `LHV_OPT=0xd`: bad1
* i386, `LHV_OPT=0x5`: bad1
* i386, `LHV_OPT=0x2d1`: bad2, see `results/20220826125935`
* i386, `LHV_OPT=0x2d0`: good
* i386, `LHV_OPT=0x4`: bad1
* i386, `LHV_OPT=0x6`: bad?

Bad1 is that the system suddenly stucks when printing EPT messages. I guess
there is an unexpected exception / interrupt happening.

From bad2 log can see that the problem happens when WRMSR 0x402. This can be
disabled by clearing `LHV_OPT` `0x1`.

If we remove the EPT printf, can see that the problem happens as BSP receives
INIT signal: serial `20220826131631`

In `lhv-dev 97f38144c`, looks the CPUs are stuck when calling `spin_lock()`.
This is strange because printf also uses `spin_lock()`. (Compile with
`./build.sh i386 --lhv-opt 0x6`).

In `lhv-dev 3fcad3181`, halt APs and show that this problem happens when only
1 CPU. In `lhv-dev 0a16ce491`, realize that actually this bug is in debugging
code. The lock should be initialized with 1, not 0.

In `lhv-dev fca3f876a`, serial `20220826152811`, looks like the BSP did
something that caused all CPUs to receive INIT signal (when in guest mode).

In `lhv-dev be088f82b`, looks like the problem happens when BSP accesses
console mmio.

In `lhv-dev 585ec3b29`, the problem disappears if we avoid accessing serial
MMIO in guest mode. Going to workaround this problem by avoid accessing serial
MMIO.

In `lhv-dev 9f77780b8`, workaround this problem by avoiding accessing serial
MMIO in guest mode. Use `LHV_OPT` `0x400 (LHV_NO_GUEST_SERIAL)`. Ported to
`lhv a5455a9db`.

* i386, `LHV_OPT=0x5fc`: good
* amd64, `LHV_OPT=0x5fc`: good

### Testing XMHF LHV

```sh
./build.sh amd64 --lhv-opt 0x5fc && gr && copy32
./build.sh amd64 && gr && copy64 && hpinit6
```

See this error in all CPUs

```
Fatal: Halting! Condition '__vmx_vmptrld (vmcs12_info->vmcs12_shadow_ptr)' failed, line 1513, file arch/x86/vmx/nested-x86vmx-handler.c
```

My guess is that 2540P does not support shadow VMCS. This guess is correct. The
CPU only supports `0xff` in secondary processor controls.

This bug is fixed in `xmhf64-nest 2b7ea954f`. The bug is caused by missing
checks to VMCS shadowing support. After that XMHF LHV is good.

### Complicated booting order in HP

Update HP 2540P to be able to host more than 2 custom GRUB entries easily.
Update `/etc/grub.d/42_xmhf` with the following.

```
#!/bin/bash
for i in build{32,64}{,lhv}; do
	echo 'menuentry "XMHF-'$i'" {'
	echo "	set root='(hd0,msdos5)'		# point to file system for /"
	echo "	set kernel='/boot/xmhf/$i/init.bin'"
	echo '	echo "Loading ${kernel}..."'
	echo '	multiboot ${kernel} serial=115200,8n1,0x5080'
	echo "	module /boot/xmhf/$i/hypervisor.bin.gz"
	echo "	module --nounzip (hd0)+1	# where grub is installed"
	echo "	module /boot/i5_i7_DUAL_SINIT_51.BIN"
	echo "}"
	echo
done
```

Add the following to `/etc/grub.d/00_header`, near the last occurrence of
`next_entry`.

```
set next_entry="\${next_entry_2}"
set next_entry_2="\${next_entry_3}"
set next_entry_3="\${next_entry_4}"
set next_entry_4="\${next_entry_5}"
set next_entry_5="\${next_entry_6}"
set next_entry_6="\${next_entry_7}"
set next_entry_7="\${next_entry_8}"
set next_entry_8="\${next_entry_9}"
set next_entry_9=
save_env next_entry next_entry_2 next_entry_3 next_entry_4 next_entry_5 next_entry_6 next_entry_7 next_entry_8 next_entry_9
```

Above should be
```
else
    cat <<EOF
if [ "\${next_entry}" ] ; then
   set default="\${next_entry}"
   set next_entry=
   save_env next_entry
   set boot_once=true
else
   set default="${GRUB_DEFAULT}"
fi
```

Below should be
```
EOF
fi
```

Set multiple GRUB defualt entries using:
```
sudo grub-editenv /boot/grub/grubenv set next_entry=XMHF-build64 next_entry_2=XMHF-build32lhv
```

### Testing XMHF XMHF LHV

```
# build64
./build.sh amd64 fast && gr
# build32
./build.sh amd64 fast --sl-base 0x20000000 --no-init-smp && gr
# build32lhv
./build.sh amd64 --lhv-opt 0x5fc 
# any
copyxmhf
hpgrub XMHF-build64 XMHF-build32 XMHF-build32lhv
hpinit6
```

Very slow, but looks good.

### Fixing LHV

```sh
grep 'FIXED_MTRR\[0\]' bug_036/results/20220208171146
# FIXED_MTRR[0]: 0x000b8000-0x000bbfff=0x00000000
grep 'VAR_MTRR' bug_036/results/20220208171146 | sort -u
```

Looks like VGA MMIO memory should be uncached, but in LHV all EPT memory is
mapped as WB. This may be the cause of the problem. The PoC is to map all
memory as UC, and it works.

```diff
diff --git a/xmhf/src/xmhf-core/xmhf-runtime/xmhf-startup/lhv-ept.c b/xmhf/src/xmhf-core/xmhf-runtime/xmhf-startup/lhv-ept.c
index 96338b90b..e08680bbf 100644
--- a/xmhf/src/xmhf-core/xmhf-runtime/xmhf-startup/lhv-ept.c
+++ b/xmhf/src/xmhf-core/xmhf-runtime/xmhf-startup/lhv-ept.c
@@ -99 +99 @@ u64 lhv_build_ept(VCPU *vcpu, u8 ept_num)
-       hpt_pmeo_setcache(&pmeo, HPT_PMT_WB);
+       hpt_pmeo_setcache(&pmeo, HPT_PMT_UC);
```

The formal way to implement it is to read MTRR MSRs. The logic to be copied is
`_vmx_getmemorytypeforphysicalpage()`. Fixed in `a5455a9db..00b24ffff`. After
the fix, LHV runs well in 2540p, with `LHV_OPT 0x1fc`.

In `fbcd23e10..847246ba5`, skip MC MTRRs in 2540p. Now 2540p can run
`LHV_OPT 0x1fd` well.
* i386, `LHV_OPT=0x1fd`: good
* amd64, `LHV_OPT=0x1bf`: good
* amd64, `LHV_OPT=0x1fd`: good

Then need to fix `xmhf64 983c6bc82..d1d313501` to support `#GP` in guest WRMSR.
After that XMHF LHV works well. XMHF XMHF LHV also works well.

### XMHF XMHF LHV O3

When compiling everything in amd64 and O3, see error in XMHF XMHF LHV:
```
Fatal: Halting! Condition '0 && "Debug: guest hypervisor VM-entry failure"' failed, line 1094, file arch/x86/vmx/nested-x86vmx-handler.c
```

The error also happens when only running O3 LHV. When LHV is O0 and XMHFs are
O3, the error also disappears.

To debug this problem, we try to dump the entire VMCS content. Also
fortunately, this bug is reproducible on QEMU.

QEMU O0 serial `20220828161443`, QEMU O3 serial `20220828161507`. Diff with
```sh
diff \
 <(grep -F 'CPU(0x00): vcpu->vmcs.' results/20220828161443) \
 <(grep -F 'CPU(0x00): vcpu->vmcs.' results/20220828161507)
```

I think the most likely problem comes from CR0 and CR4.
```
-CPU(0x00): vcpu->vmcs.host_CR0=0x0000000080000031
-CPU(0x00): vcpu->vmcs.host_CR4=0x0000000000002020
+CPU(0x00): vcpu->vmcs.host_CR0=0x0000000080000011
+CPU(0x00): vcpu->vmcs.host_CR4=0x0000000000000020
```

Looks like CR4.VMXE is missing. It is almost obviously wrong. The problem is
that this function will error, which looks like that `write_cr4()` fails.

```c
void func(void)
{
	{
		ulong_t cr4 = read_cr4();
		HALT_ON_ERRORCOND((cr4 & CR4_VMXE) == 0);
		write_cr4(cr4 | CR4_VMXE);
	}
	HALT_ON_ERRORCOND(read_cr4() & CR4_VMXE);
}
```

Looks like the problem is that in `_processor.h`, `write_cr4()` only uses
`__asm__`, but should use `__asm__ __volatile__`. The same problem happens in
`_vmx.h`. Fixed in `xmhf64 d1d313501..433803237`.

Another problem happens, where `__vmx_invvpid` cannot pass arguments correctly
to the INVVPID instruction. Reproduced in `lhv-dev 37614d91e`. The second
assertion below fails.
```c
void func(void)
{
	HALT_ON_ERRORCOND(!__vmx_invvpid(0, 0, 0x0));
	HALT_ON_ERRORCOND(__vmx_invvpid(0, 3, 0x0));
}
```

This bug can be reproduced standalone in `a1.c` as a 64-bit user program. Then
it can be reduced in `a2.c`. The problem is that the struct is not volatile.
Asked <https://stackoverflow.com/questions/73523342/>. Fixed in
`xmhf64 433803237..a4c0dd40c`.

After the fix, LHV and XMHF XMHF LHV are tested and work fine.

Then, thanks to the discussion on stackoverflow, I realized that the asm
volatile code is still wrong because modifying input operands is not allowed.
See <https://gcc.gnu.org/onlinedocs/gcc/Extended-Asm.html>
6.47.2.5 Input Operands:
> Warning: Do not modify the contents of input-only operands (except for inputs
> tied to outputs). ... It is not possible to use clobbers to inform the
> compiler that the values in these inputs are changing.

### Testing XMHF XMHF Debian

We then test running XMHF XMHF Debian. Also see all cores receiving INIT
signal in VMX nonroot mode. Serial `20220827125332`. I guess it may be similar
to LHV console mmio problem above. This behavior is also similar to `bug_036`.

First, we try running Linux with only one CPU. Looks like we can use
`nosmp` or `maxcpus=0` in GRUB command line.

Also note that sometimes the error happens earlier if keyboard interaction is
made in GRUB. The error becomes:

```
CPU(0x00): OS tries to write microcode!
gva for microcode update: 0xffff8880327c7ae0

Fatal: Halting! Condition 'result == 0' failed, line 122, file arch/x86/vmx/peh-x86vmx-guestmem.c
```

So we need to edit GRUB to make `nosmp` the default, instead of editing
dynamically. At this time, I still guess that the problem happens because of
some problem in EPT code. Maybe related to MTRR.

When `nosmp` in Linux, can see that the error still happens. At worst we can
use monitor trap to debug Linux and see (likely) which memory access causes the
problem.

TODO: disable update ucode
TODO: dump ept and vmcs at last known location
TODO

