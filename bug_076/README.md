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

Another bug not reported: see `bug_077`

Waiting for KVM people.
