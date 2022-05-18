# QEMU `!env->exception_has_payload` assertion fail

## Scope
* A lot of configurations
* QEMU bug, not XMHF bug
* Reproducible in `qemu-kvm-6.1.0-14.fc35.x86_64` running in Intel Core i7-4510U
* Reproducible in SMP
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

TODO: try on later CPU
TODO: report bug to KVM
* <https://www.linux-kvm.org/page/Bugs>
* <https://docs.fedoraproject.org/en-US/quick-docs/howto-file-a-bug/>
TODO: compile upstream kernel
* <https://docs.fedoraproject.org/en-US/quick-docs/kernel/build-custom-kernel/>

