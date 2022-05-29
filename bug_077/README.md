# GRUB image cannot boot Windows

## Scope
* QEMU (HP not tested)

## Behavior
Using the grub image to boot Linux is fine, but when booting Windows, e.g.
`./bios-qemu.sh --qb32 --gdb 2198 -d build32 +1 -d win10x86 -a -smp 1`

See strange KVM error
```
CPU(0x00): x86vmx SMP but only one CPU
xmhf_runtime_main[00]: starting partition...
CPU(0x00): VMCLEAR success.
CPU(0x00): VMPTRLD success.
CPU(0x00): VMWRITEs success.error: kvm run failed Input/output error
EAX=00000020 EBX=0000ffff ECX=00000000 EDX=0000ffff
ESI=00000000 EDI=00002300 EBP=00000000 ESP=00006d8c
EIP=00000018 EFL=00000046 [---Z-P-] CPL=0 II=0 A20=1 SMM=0 HLT=0
ES =f000 000f0000 ffffffff 00809300
CS =cb00 000cb000 ffffffff 00809b00
SS =0000 00000000 ffffffff 00809300
DS =0000 00000000 ffffffff 00809300
FS =0000 00000000 ffffffff 00809300
GS =0000 00000000 ffffffff 00809300
LDT=0000 00000000 0000ffff 00008200
TR =0000 00000000 0000ffff 00008b00
GDT=     00000000 00000000
IDT=     00000000 000003ff
CR0=00000010 CR2=00000000 CR3=00000000 CR4=00000000
DR0=00000000 DR1=00000000 DR2=00000000 DR3=00000000 
DR6=ffff0ff0 DR7=00000400
EFER=0000000000000000
Code=0e 07 31 c0 b9 00 10 8d 3e 00 03 fc f3 ab 07 b8 20 00 e7 7e <cb> 0f 1f 80 00 00 00 00 6b 76 6d 20 61 50 69 43 20 00 00 00 2d 02 00 00 d9 02 00 00 00 03
KVM_GET_PIT2 failed: Input/output error
./bios-qemu.sh: line 117: 747462 Aborted                 (core dumped) "$QEMU_BIN" "${ARGS[@]}" < /dev/null
```

As mentioned in
<https://www.gnu.org/software/grub/manual/grub/html_node/DOS_002fWindows.html>,
Windows' bootloader is incorrect, so booting it requires some hacks. For
example,
```
menuentry 'Windows' {
	insmod part_msdos
	insmod ntfs
	set root='hd1,msdos1'
	parttool ${root} hidden-
	drivemap -s (hd0) ${root}
	chainloader +1
}
```

However, even with this workaround, Windows still shows this error

## Debugging

Currently just documenting this bug. The best hope is to file the bug to KVM.

Trying to not file too many bugs at the same time. See `bug_076`.

Early phase 3 of the research is not dealing with Windows, so we ignore this
issue for now.

Later, realized that it is another form of the BIOS SMM bug. See `bug_031`. The
workaround is to use a version of SeaBIOS compiled with SMM mode disabled.
The `./bios-qemu.sh` option is `--win-bios`.

It is time to report bug to KVM. Maybe together with `bug_031`.

### Booting Windows

For Windows 7 and above, can boot as a second disk

```sh
./bios-qemu.sh        -d build64 +1 -d win10x64 -t -d f -t --win-bios
./bios-qemu.sh --qb32 -d build32 +1 -d win10x86 -t -d f -t --win-bios
```

For Windows XP, need to use another GRUB image for chainloading. The GRUB image
can be created after git `ab7968ed8`, using command
```sh
python3 tools/ci/grub.py --subarch windows --work-dir . --boot-dir tools/ci/boot
```

The image is stored in this bug as `grub_windows.img`.

```sh
./bios-qemu.sh        -d build64 +1 -d grub_windows +1 -d winxpx64 -t --win-bios
./bios-qemu.sh --qb32 -d build32 +1 -d grub_windows +1 -d winxpx86 -t --win-bios
```

### KVM bug

We do print debugging in KVM a little bit.

Recall the error message 

```
error: kvm run failed Input/output error


EAX=00000020 EBX=0000ffff ECX=00000000 EDX=0000ffff
ESI=00000000 EDI=00002300 EBP=00000000 ESP=00006d8c
EFER=0000000000000000
Code=0e 07 31 c0 b9 00 10 8d 3e 00 03 fc f3 ab 07 b8 20 00 e7 7e <cb> 0f 1f 80 00 00 00 00 6b 76 6d 20 61 50 69 43 20 00 00 00 2d 02 00 00 d9 02 00 00 00 03


KVM_GET_PIT2 failed: Input/output error
```

The first line comes from QEMU's `kvm_cpu_exec()`. It happens when
`run_ret = kvm_vcpu_ioctl(cpu, KVM_RUN, 0);` returns negative value.

Then some kind of register dump appears. Finally the `KVM_GET_PIT2` looks like
unrelated to the root cause. It happens because KVM goes away, so this ioctl
call errors.

`KVM_RUN` is handled in
<https://elixir.bootlin.com/linux/v5.18/source/virt/kvm/kvm_main.c#L3923>.
The error happens when `r = kvm_arch_vcpu_ioctl_run(vcpu);` (line 3943) gives
`-5`.

The call stack is
```
QEMU: ioctl(..., KVM_RUN, ...)
  kvm_vcpu_ioctl()
    kvm_arch_vcpu_ioctl_run()
      vcpu_run()
        vcpu_enter_guest()
          vmx_handle_exit() (static_call(kvm_x86_handle_exit))
            __vmx_handle_exit()
              KVM_BUG_ON(vmx->nested.nested_run_pending, vcpu->kvm)
```

Bug reported in <https://bugzilla.kernel.org/show_bug.cgi?id=216046>. KVM bugs
will be tracked in `bug_076`.

## Result

Use `--win-bios` to avoid SMM mode when booting Windows. Use `grub_windows.img`
for Windows XP. KVM bug reported.

