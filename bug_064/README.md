# KVM nested virtualization booting Debian on XMHF stuck / slow

## Scope
* All subarch
* QEMU on nested KVM only
* Git `05943b8e0`

## Behavior
This bug is discovered in `bug_063`. When XMHF runs on two levels of KVM,
things become very slow, and Debian 11 fails to boot in a few minutes.
* Good
	* Intel CPU
		* KVM (QEMU, Host is Fedora)
			* XMHF
				* Debian 11
* Bad
	* Intel CPU
		* KVM (QEMU, Host is Fedora)
			* KVM (QEMU, Host is Debian 11)
				* XMHF
					* Debian 11

## Debugging
The guess is that nested virtualization is too slow. However we need to confirm
it.

First add some printing in Debian. Git `7930f296f`, see count for all types of
VMEXITs
```
      3 ,0}
   3469 ,10}	cpuid
      1 ,28}
     12 ,31}	rdmsr
   8016 ,32}	wrmsr
```

We now print more information about CPUID and WRMSR. Git `ff6455437`.
Can see that almost all problematic VMEXITS are related to WRMSR 0x6e0, which
is `IA32_TSC_DEADLINE`. So to fix the problem, we need to make WRMSR and CPUID
very fast.

In git `60081b12a`, optimized CPUID and WRMSR.ecx=0x6e0. Can feel that things
become faster.

Implemented `36879f183` in xmhf64 branch.

### Testing with nested virtualization

We use curlftpfs to prevent copying QCOW2 files around. In Debian, install
dependencies
```
sudo apt-get install curlftpfs qemu-system-x86
sudo usermod -aG kvm lxy
# Another workaround: sudo chmod 666 /dev/kvm
```

In host, host files in PWD using FTP
```
pip3 install pyftpdlib
python3 -m pyftpdlib -u lxy -P PASSWORD -i IP -p PORT
```

In Debian, use curlftpfs
```
mkdir mnt
curlftpfs HOST:PORT mnt -o user=USER:PASS
```

QEMU can read backing image using FTP, but cannot write to FTP images.

In GRUB, remove `quiet` when booting Debian shows the progress of Debian
booting.

Test result is that: when XMHF's SMP = 1, can boot. However, when SMP = 2, see
strange errors: `{0,48}{0,0}` (EPT violation and exception)

We likely need to try some QEMU + GDB debugging.

### LAPIC

Can see that `{0,48}{0,0}` are due to write to LAPIC registers. Need to disable
Kaslr for Debian in GRUB, then see
The first exception happens when `RIP = 0xffffffff81062a52`.
The second exception happens when `RIP = 0xffffffff81062a58`.

GDB snippet for performing page walk in long mode is written in `page.gdb`.
For example, usage is
```
(gdb) source .../bug_064/page.gdb
(gdb) walk_pt vcpu->vmcs.guest_RIP
$3 = "4-level paging"
$4 = "Page size = 2M"
$5 = 0x1062a52
(gdb) x/3i $ans - 2
   0x1062a50:	mov    %edi,%edi
   0x1062a52:	mov    %esi,-0xa03000(%rdi)
   0x1062a58:	ret    
(gdb) 
```

`0xffffffff81062a50` is `native_apic_mem_write`. The call stack is
```
(gdb) bt
#0  native_apic_mem_write (reg=176, v=0) at include/asm-generic/fixmap.h:33
#1  0xffffffff810fae33 in handle_edge_irq (desc=0xffff88800354e600) at kernel/irq/chip.c:803
#2  0xffffffff81a010ef in asm_call_on_stack () at /build/linux-ha3l9a/linux-5.10.70/arch/x86/entry/entry_64.S:788
#3  0xffffffff818b3b40 in __run_irq_on_irqstack (desc=0xffff88800354e600, func=<optimized out>) at arch/x86/include/asm/irq_stack.h:48
#4  run_irq_on_irqstack_cond (regs=0xffffffff82603c88, desc=0xffff88800354e600, func=<optimized out>) at arch/x86/include/asm/irq_stack.h:101
#5  handle_irq (regs=0xffffffff82603c88, desc=0xffff88800354e600) at arch/x86/kernel/irq.c:230
#6  __common_interrupt (vector=48 '0', regs=0xffffffff82603c88) at arch/x86/kernel/irq.c:249
#7  common_interrupt (regs=0xffffffff82603c88, error_code=48) at arch/x86/kernel/irq.c:239
#8  0xffffffff81a00c5e in asm_common_interrupt () at /build/linux-ha3l9a/linux-5.10.70/arch/x86/include/asm/idtentry.h:626
#9  0x0000000000000000 in ?? ()
(gdb) 
```

Can see that Debian is trying to write to APIC's EOI register. Linux is trying
to handle an edge irq. We only know vector=48, but do not know the source
device of interrupt.

`/proc/interrupts` does not show anything useful.

Possible next steps
* Do not use SMP
* Try to decrease timer speed?

### Dive into Linux source code

Using the call stack from GDB above, draw call stack in C code
```
common_interrupt
 handle_irq
  run_irq_on_irqstack_cond(handle_edge_irq, ...)
   __run_irq_on_irqstack
    asm_call_irq_on_stack(handle_edge_irq, ...) == asm_call_on_stack
     handle_edge_irq
      irq_chip_ack_parent (desc->irq_data.chip->irq_ack)
       apic_ack_edge (data->chip->irq_ack)
        apic_ack_irq
         ack_APIC_irq
          apic_eoi
           kvm_guest_apic_eoi_write (apic->eoi_write)
            native_apic_mem_write (apic->native_eoi_write)
             movl ...
      handle_irq_event
       handle_irq_event_percpu
        __handle_irq_event_percpu
         timer_interrupt
          tick_handle_periodic (global_clock_event->event_handler)
           ...
```

We now confirm that the device is timer.

We also note that the call stack contains `kvm_guest_apic_eoi_write()`.
This is enabled when `KVM_FEATURE_PV_EOI` is detected using CPUID 0x40000001.
We should try to disable this in CPUID and see what happens.

When running "Intel CPU -> KVM -> XMHF -> Debian", also see `{0,48}{0,0}`, but
soon other things start running.

Then optimized intercept 48 and intercept 0, solved the problem.

### About timer

<https://www.kernel.org/doc/Documentation/virtual/kvm/api.txt> mentions that 
kvm-pit is a kernel thread used to inject PIT timer interrupts for KVM.
However, I do not see an easy way to change the timer frequency etc.

See also: <https://wiki.osdev.org/Talk:Programmable_Interval_Timer>
> The PIT isn't emulated properly on QEMU/VirtualBox. 
> Use real machines or Bochs to test it (if you delay 
> function works well on QEMU this doesn't mean it will 
> work on a real machine). There is also a piece of bad 
> PIT code in books, so look out!!!

## Fix

`05943b8e0..7abab5b91`
* Optimize XMHF for repeated intercepts (CPUID, WRMSR 0x6e0)
* Optimize XMHF for sending LAPIC EOI
* Enable Circle CI testing

