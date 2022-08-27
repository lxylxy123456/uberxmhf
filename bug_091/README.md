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

### Testing XMHF XMHF Debian

TODO: XMHF XMHF Debian cannot boot

