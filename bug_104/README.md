# Testing Hyper-V

## Scope
* Windows guest
* All configurations
* `xmhf64 6792c4c41`
* `xmhf64-nest d52b4718c`

## Behavior
We need to try Hyper-V, the hypervisor that is only available in Windows.

### Enabling Hyper-V

First enable Hyper-V in Windows 10. Follow official guide:
* <https://learn.microsoft.com/en-us/virtualization/hyper-v-on-windows/reference/hyper-v-requirements>
	* Use `systeminfo` in power shell to check whether hardware supports
	  Hyper-V
* <https://learn.microsoft.com/en-us/virtualization/hyper-v-on-windows/quick-start/enable-hyper-v>
	* In "Windows Features", turn on Hyper-V
	* Restart
* <https://learn.microsoft.com/en-us/virtualization/hyper-v-on-windows/quick-start/quick-create-virtual-machine>
	* TODO

### XMHF unable to boot Windows 10 with Hyper-V enabled

After enabling Hyper-V in Windows 10, XMHF is no longer to boot it. At some
point during the boot sequence, XMHF simply stops outputing anything.

XMHF build command is:
```
./build.sh amd64 O3 fast --mem 0x230000000 --event-logger && gr
```

Sometimes see "windows fail to start" with error code 0xc0000225.

We use VMX preemption timer to see whether Windows 10 is stuck. Can follow some
code from `bug_036` in `70aaa7ff2`. The patch applied is very simple, as shown
below. Looks like the serial output of XMHF is good.

```diff
diff --git a/xmhf/src/xmhf-core/xmhf-runtime/xmhf-eventhub/arch/x86/vmx/peh-x86vmx-main.c b/xmhf/src/xmhf-core/xmhf-runtime/xmhf-eventhub/arch/x86/vmx/peh-x86vmx-main.c
index 321796db5..185568003 100755
--- a/xmhf/src/xmhf-core/xmhf-runtime/xmhf-eventhub/arch/x86/vmx/peh-x86vmx-main.c
+++ b/xmhf/src/xmhf-core/xmhf-runtime/xmhf-eventhub/arch/x86/vmx/peh-x86vmx-main.c
@@ -1199,9 +1199,13 @@ u32 xmhf_parteventhub_arch_x86vmx_intercept_handler(VCPU *vcpu, struct regs *r){
 	 * is for quiescing (vcpu->vmcs.info_vmexit_reason == VMX_VMEXIT_EXCEPTION),
 	 * otherwise will deadlock. See xmhf_smpguest_arch_x86vmx_quiesce().
 	 */
-//	if (vcpu->vmcs.info_vmexit_reason != VMX_VMEXIT_EXCEPTION) {
-//		printf("{%d,%d}", vcpu->id, (u32)vcpu->vmcs.info_vmexit_reason);
-//	}
+	if (vcpu->vmcs.info_vmexit_reason != VMX_VMEXIT_EXCEPTION) {
+		printf("{%d,%d}", vcpu->id, (u32)vcpu->vmcs.info_vmexit_reason);
+		/* guest_VMX_preemption_timer_value */
+		vcpu->vmcs.control_VMX_pin_based |=
+			(1U << VMX_PINBASED_ACTIVATE_VMX_PREEMPTION_TIMER);
+		__vmx_vmwrite32(0x482E, 0x10000);
+	}
 
 #ifdef __DEBUG_EVENT_LOGGER__
 	if (vcpu->vmcs.info_vmexit_reason == VMX_VMEXIT_CPUID) {
@@ -1226,6 +1230,9 @@ u32 xmhf_parteventhub_arch_x86vmx_intercept_handler(VCPU *vcpu, struct regs *r){
 		//xmhf-core and hypapp intercepts
 		//--------------------------------------------------------------
 
+		case 52:
+			break;
+
 		case VMX_VMEXIT_VMCALL:{
 			//if INT 15h E820 hypercall, then let the xmhf-core handle it
 			if(vcpu->vmcs.guest_CS_base == (VMX_UG_E820HOOK_CS << 4) &&
```

My current guess is that Windows requires some hypervisor feature but does not
see it. As a result it halts. Now we need to figure out which feature Windows
needs. Otherwise, we can also use VMEXIT info / monitor trap to debug.

When without XMHF, `systeminfo` says that Hyper-V is supported. However, when
with XMHF, `systeminfo` gives the same result:
```
Hyper-V Requirements:      VM Monitor Mode Extensions: Yes
                           Virtualization Enabled In Firmware: Yes
                           Second Level Address Translation: Yes
                           Data Execution Prevention Available: Yes
```

By printing all RDMSR values, we see that `IA32_VMX_BASIC` are read at some
point:
```
RDMSR @ 0xfffff8016ffd2aa3: 0x00000480
RDMSR @ 0xfffff8016ffd2adf: 0x0000048d
RDMSR @ 0xfffff8016ffd2adf: 0x0000048e
RDMSR @ 0xfffff8016ffd2adf: 0x0000048f
RDMSR @ 0xfffff8016ffd2adf: 0x00000490
RDMSR @ 0xfffff8016ffd2b09: 0x00000486
RDMSR @ 0xfffff8016ffd2b09: 0x00000487
RDMSR @ 0xfffff8016ffd2b09: 0x00000488
RDMSR @ 0xfffff8016ffd2b09: 0x00000489
RDMSR @ 0xfffff8016ffd2b30: 0x0000048b
RDMSR @ 0xfffff8016ffd2b52: 0x0000048c
RDMSR @ 0xfffff8016ffd2b6e: 0x00000491
RDMSR @ 0xfffff8016ffd2b83: 0x00000485
...
RDMSR @ 0x000000000088c422: 0x00000485
...
```

Git `xmhf64-nest-dev bda4680bd`, serial `20221023131120`. Windows basically
sends itself an NMI at the end. The last 3 VMEXITs are corresponding to
instructions:
```
mov    0x300(%rax),%r8d
mov    %edx,0x310(%rax)		EDX=0
mov    %ecx,0x300(%rax)		ECX=0x400
```

However looks like XMHF does not detect it.

### Reproducing on QEMU

Take `win10x64.qcow2`, enable Hyper-V, get `win10x64-h.qcow2`. Use the
following to run on QEMU:
```sh
./bios-qemu.sh --gdb 2198 -m 1G -smp 1 -d build64 +1 -d win10x64-h -t --win-bios | tee /tmp/amt
```

See XMHF assertion error:
```
Fatal: Halting! Condition '!vcpu->vmx_nested_ept02_flush_disable' failed, line 599, file arch/x86/vmx/nested-x86vmx-ept12.c
```

Looks like EPT02 block is called during VMENTRY, but VMENTRY fails and XMHF
forgets to unblock EPT02. Fixed in `xmhf64-nest 0cbfc9524`.

After the fix, QEMU can proceed later. However, it still halts at some point.

Looks like by changing QEMU to SMP (`-smp 2`), we can reproduce the problem on
Dell. Git `xmhf64-nest-dev 65d297f38`, serial `20221023143113`.

By debugging using QEMU, when Windows 10 CPU 0 is sending NMI to itself using
IPI, the "Blocking by NMI" in guest Interruptibility State is 1. Since NMI is
blocked unexpectedly, Windows 10 stucks.

Git `xmhf64-nest-dev 363688fdc`, serial `20221023170746`. We see that guest NMI
blocking happens after XMHF trying to emulate LAPIC memory move instruction. In
`xmhf_smpguest_arch_x86vmx_eventhandler_dbexception()`, the code is
```c
  //restore NMI blocking (likely enable NMIs)
  vcpu->vmcs.guest_interruptibility |= g_vmx_lapic_guest_intr_nmimask;
```

However, this code is essentially no-op because the NMI blocking bit of
`vcpu->vmcs.guest_interruptibility` is always 1 before the instruction. I guess
in other OSes, there will be some form of IRET happening afterwards, which
hides this bug. Fixed in `xmhf64 6792c4c41..74dcb4c0e`.

Unused ideas
* Try to reproduce on 2540p
* Review list of all VMEXITs by Windows
* Compare list of all VMEXITs by Windows, with / without Hyper-V
* Check Windows settings about VBS
	* VBS is not enabled at all. Also, VBS probably requires UEFI.

### Windows 10 still fails to boot with Hyper-V

`xmhf64-nest-dev e21c9638b`, HP serial `20221023172918` and reboots
automatically. QEMU serial `20221023173134` and stops at an HLT instruction
below. IDT is set to 0xffffffffffffffff:0x00000000. QEMU UP serial
`20221023173621` and result is similar.

```
   0xfffff81c36c77974:	cli    
   0xfffff81c36c77975:	lidt   0x20(%rsp)
   0xfffff81c36c7797a:	hlt    
=> 0xfffff81c36c7797b:	jmp    0xfffff81c36c7797a
```

I think QEMU UP and SMP are the same, so debug on UP to make things simple.
Dell and QEMU are also similar.

TODO: check XMHF issued VMENTRY errors

