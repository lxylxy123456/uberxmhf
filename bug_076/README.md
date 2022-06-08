# QEMU `!env->exception_has_payload` assertion fail

## Scope
* A lot of configurations
* QEMU bug, not XMHF bug
* Reproducible in `qemu-kvm-6.1.0-14.fc35.x86_64` running in Intel Core i7-4510U
* Reproducible in QEMU 5.2.0 running in Intel Core i7-1185G7
* Reproducible in SMP, not reproducible in UP
* Note: this bug is found since `bug_004`

## Behavior

XMHF git `a8610d2f9`, lhv git `10afe107c`

Build XMHF and LHV in x86, then run lhv in XMHF:
```sh
./bios-qemu.sh --gdb 1234 -d build32 +1 -d build32lhv +1 --qb32 -smp 2
```

In another terminal, set any break point in GDB:
```sh
gdb --ex 'target remote :::1234' --ex 'hb *0' --ex 'c'
```

Then will see assertion error in KVM

```
...
CPU #0: vcpu_vaddr_ptr=0x01e06080, esp=0x01e11000
CPU #1: vcpu_vaddr_ptr=0x01e06540, esp=0x01e15000
BSP(0x00): Rallying APs...
BSP(0x00): APs ready, doing DRTM...
LAPIC base and status=0xfee00900
Sending INIT IPI to all APs...qemu-system-i386: ../target/i386/kvm/kvm.c:645: kvm_queue_exception: Assertion `!env->exception_has_payload' failed.
./bios-qemu.sh: line 117: 107580 Aborted                 (core dumped) "$QEMU_BIN" "${ARGS[@]}" < /dev/null
$ 
```

## Debugging

### Building Linux

A lot of bug reporting websites require testing against latest Linux version.
So we should try to build Linux ourselves.

Following
<https://kernel-team.pages.debian.net/kernel-handbook/ch-common-tasks.html#s-kernel-org-package> to build Linux on Debian

```sh
apt-get install build-essential fakeroot libncurses-dev
apt-get build-dep linux

wget https://mirrors.edge.kernel.org/pub/linux/kernel/v5.x/linux-5.17.9.tar.xz
tar xaf linux-5.17.9.tar.xz 
cd linux-5.17.9/
cp /boot/config-5.10.0-14-amd64 .config
scripts/config --disable MODULE_SIG
scripts/config --disable DEBUG_INFO
make nconfig
make deb-pkg
```

However, have some problems when building, give up.

Unused: building on Fedora
* <https://docs.fedoraproject.org/en-US/quick-docs/kernel/build-custom-kernel/>

### Later CPU

The two bugs are also reproducible in HP 840. The advantage is that QEMU runs a
lot faster.

### Report bugs

Guides
* <https://www.linux-kvm.org/page/Bugs>
* <https://docs.fedoraproject.org/en-US/quick-docs/howto-file-a-bug/>

Reports
* The `!env->exception_has_payload` assertion error is reported in
  <https://bugzilla.kernel.org/show_bug.cgi?id=216002>
* The `ret < cpu->num_ases && ret >= 0` (`bug_031`) assertion error is reported
  in <https://bugzilla.kernel.org/show_bug.cgi?id=216003>
* VMXON does not check CR0 (`bug_079`) is reported in
  <https://bugzilla.kernel.org/show_bug.cgi?id=216033>

Another bug not reported: see `bug_077`

Waiting for KVM people.

### Reposting `ret < cpu->num_ases && ret >= 0`

This bug is first found in `bug_031`. I first posted this bug on KVM at
<https://bugzilla.kernel.org/show_bug.cgi?id=216003>, but then it looks like
this bug is in QEMU. Now before re-posting this bug, I would like to look into
it and possibly try it on a later version of QEMU (self-compiled).

We can replace `si 1000` with a while loop, and print the instructions. Here
is the updated GDB script
```
set confirm off
set pagination off
hb *0x7c00
c
set $i = 0
while $i < 1000
    p $i
    si
    set $i = $i + 1
end
```

The output is

```
0x00000000000f7d16 in ?? ()
$770 = 770
0x00000000000f7d1b in ?? ()
$771 = 771
0x00000000000f7d20 in ?? ()
$772 = 772
0x00000000000f7d22 in ?? ()
$773 = 773
```

When the bug happens, the CPU is in protected mode, as `cr0 = 0x11 [ ET PE ]`.
`EIP = 0xf7d22`. We can break at `0xf7d20`.

This bug may be related the one in `bug_077`. However, I am not sure at this
point. So still file the bug on QEMU anyway.

The updated procedure to reproduce on Fedora is
1. `qemu-system-i386 --drive media=disk,file=w.img,format=raw,index=1 -s -S -enable-kvm -display none`
2. Use the following GDB script

```
set confirm off
set pagination off
hb *0x7c00
c
d
hb *0xf7d20
c
d
si
si
```

Also note that the problem happens in BIOS code. So the BIOS binary should
also be submitted to the bug. The default installation path of BIOS is
`/usr/share/seabios/bios.bin`. When reproducing, use `-L /usr/share/seabios/`
in QEMU command line (`-bios /usr/share/seabios/bios.bin` does not work).

This bug also happens on newer QEMU versions. The difference is that the
break point address changes, because the BIOS image is likely different.

Maybe the best GDB script is still
```
gdb --ex 'target remote :::1234' --ex 'hb *0x7c00' --ex c --ex d --ex 'si 1000' --ex q
```

However, this GDB script does not work on QEMU 7.0.0 and Debian 11 Linux 5.17.9.
On other combinations, it works. I guess the reason is that `si 1000` halts in
another place due to change in behavior of KVM and QEMU. We probably need to
still set 2 break points. But before that, we need to know the address of
breakpoint.

We use the following GDB script

```
set confirm off
set pagination off
hb *0x7c00
c
d
set $i = 0
while $i < 1000
    set $i = $i + 1
	p $i
    x/i $eip
    si
end
```

On Fedora (`5.17.8-200.fc35.x86_64`, `qemu-6.1.0-14.fc35`), we get

```
0x000f7d16 in ?? ()
$770 = 770
=> 0xf7d16:	mov    $0x5678,%ecx
0x000f7d1b in ?? ()
$771 = 771
=> 0xf7d1b:	mov    $0x7d25,%ebx
0x000f7d20 in ?? ()
$772 = 772
=> 0xf7d20:	out    %al,$0xb2
0x000f7d22 in ?? ()
$773 = 773
=> 0xf7d22:	pause
```

So the break point should be at `0xf7d20`.

On Debian old kernel old QEMU (`5.10.0-14-amd64`, QEMU `5.2.0`), we get

```
$770 = 770
=> 0xf7dd4:     mov    $0x5678,%ecx
0x000f7dd9 in ?? ()
$771 = 771
=> 0xf7dd9:     mov    $0x7de3,%ebx
0x000f7dde in ?? ()
$772 = 772
=> 0xf7dde:     out    %al,$0xb2
0x000f7de0 in ?? ()
$773 = 773
=> 0xf7de0:     pause  
```

So the break point should be at `0xf7dde`. Note that `$i` is the same.

On Debian old kernel new QEMU (`5.10.0-14-amd64`, QEMU `7.0.0`), we get

```
$770 = 770
=> 0xf7d76:     mov    $0x5678,%ecx
0x000f7d7b in ?? ()
$771 = 771
=> 0xf7d7b:     mov    $0x7d85,%ebx
0x000f7d80 in ?? ()
$772 = 772
=> 0xf7d80:     out    %al,$0xb2
0x000f7d82 in ?? ()
$773 = 773
=> 0xf7d82:     pause  
```

The break point should be `0xf7d80`. We confirm using another GDB script.

```
set confirm off
set pagination off
hb *0x7c00
c
d
hb *0xf7d80
c
d
si
si
```

This script works. Now we reboot Debian to test the new kernel. Unfortunately,
the bug is no longer reproducible on the new kernel with the new QEMU version.
What is even worse is that it looks like a new bug: after some `si`
instructions, if I put `c` to try to resume operation, the Windows error page
never shows up.

Using the GDB that contains while 1000 loop, the result is

```
$736 = 736
=> 0x760:       and    $0x2,%al
0x00000762 in ?? ()
$737 = 737
=> 0x762:       ret    
0x000006e1 in ?? ()
$738 = 738
=> 0x6e1:       sti    
0x000006e2 in ?? ()
$739 = 739
=> 0x6e2:       mov    $0x1acdbb00,%eax
0x000006e5 in ?? ()
$740 = 740
=> 0x6e5:       int    $0x1a
```

For now I am going to report the bug with the old Linux kernel version.

```
gdb --ex 'target remote :::1234' --ex 'hb *0x7c00' --ex c --ex 'si 1000' --ex q
```

Reported as <https://gitlab.com/qemu-project/qemu/-/issues/1047>.

### Tracking QEMU / KVM bugs

This page will be used to track various bugs reported to QEMU / KVM

* `bug_076`: assertion `!env->exception_has_payload`
	* <del><https://bugzilla.kernel.org/show_bug.cgi?id=216002></del>
	* <https://gitlab.com/qemu-project/qemu/-/issues/1045>
* `bug_031`: assertion `ret < cpu->num_ases && ret >= 0`
	* <del><https://bugzilla.kernel.org/show_bug.cgi?id=216003></del>
	* <https://gitlab.com/qemu-project/qemu/-/issues/1047>
* `bug_079`: VMXON does not check CR0
	* <https://bugzilla.kernel.org/show_bug.cgi?id=216033>
	* Fix sent by email, from Sean Christopherson <seanjc@google.com>, title
	  "[PATCH v5 03/15] KVM: nVMX: Inject #UD if VMXON is attempted with
	   incompatible CR0/CR4", time Tue,  7 Jun 2022 21:35:52 +0000
* `bug_078`: Linux stucks when using x2APIC on HP 840
	* <https://bugzilla.kernel.org/show_bug.cgi?id=216045>
* `bug_077`: XMHF booting Windows results in "kvm run failed Input/output error"
	* <https://bugzilla.kernel.org/show_bug.cgi?id=216046>
* `bug_079`: 10 VMCS fields not supported
	* <https://bugzilla.kernel.org/show_bug.cgi?id=216091>

