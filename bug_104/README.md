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
	* Open "Hyper-V Manager", follow tutorial

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

In git `xmhf64-nest-dev b51f67a20`, QEMU UP serial `20221024103137`, print
whether VMENTRY fails or not. Can see that Windows first performs a VMLAUNCH
and a series of VMRESUME's and succeeds. The exit reason for those are VMCALL.
Then Windows performs two failed VMLAUNCHs and halts.

If we retry booting Windows, Windows shows error message for previous boot,
which is 0xc0000017 "There isn't enough memory to create a ramdisk device."

By printing, the VMENTRY failure error code is 7
(`VM_INST_ERRNO_VMENTRY_INVALID_CTRL`). The problem happens at bit 28 of
`control_VMX_cpu_based` (`VMX_PROCBASED_USE_MSR_BITMAPS`). Windows sets this
bit, but XMHF does not allow it.

```
(gdb) p/x val
$5 = 0xb6a06dfa
(gdb) p/x fixed0
$6 = 0x4006172
(gdb) p/x fixed1
$7 = 0xeff9fffe
(gdb) 
```

Also interestingly, the behavior of Windows is to first try with
`val=0xb6a06dfa`. When that fails, it tries `val=0xb6206dfa` (removes bit 23
`VMX_PROCBASED_MOV_DR_EXITING`). See git `xmhf64-nest-dev 49ae0f18c`, serial
`20221024105343`.

Now it appears that Windows is imperfect since
* It returns error 0xc0000017 to the user, but the error is totally irrelevant
  to insufficient memory.
* It should check MSRs to make sure whether VMCS control fields are allowed.
  When not allowed, it should not blindly remove bit 23.

Now to make Hyper-V work, we just need to support MSR bitmaps in XMHF.

### Implement MSR bitmaps

In `ecf60f5f3..8f73abd70`, implment MSR bitmap. However, after the commit,
(Dell, XMHF, Debian, VirtualBox, Windows) no longer works. So not commiting to
`xmhf64` for now.

Build with command `./build.sh amd64 fast --mem 0x230000000 && gr`, in
`xmhf64-nest ecf60f5f3`, Windows 7 x86 and x64 can all boot correctly.

However, in `xmhf64-nest 8f73abd70`, Windows 7 x86 bluescreen with error code
`0x71(0, 0, 0, 0)`. Windows 7 x64 see Virtual Box
`Guru Meditation 1155 (VINF_EM_TRIPLE_FAULT)`. So the MSR bitmap implementation
is incorrect.

In `lhv a717b82cb..2097a56d4`, add tests for MSR bitmap. However, looks like
XMHF passes the test.

Found a pitfall related to GCC's `-Werror=implicit-fallthrough` warning. In the
following code, `break;` for `case VMX_VMEXIT_RDMSR` is forgotten. However, GCC
does not emit a warning because the following case is simply return / break.
However, this causes unexpected behavior because return and break have
different behavior in my code. Suppose `case VMX_VMEXIT_CPUID` contains
something like `i++;`, then there will be a warning generated.

```c
	switch (info->vmexit_reason) {
	...
	case VMX_VMEXIT_RDMSR:
		switch (r->ecx) {
		case MSR_APIC_BASE: /* fallthrough */
		case IA32_X2APIC_APICID:
			return;
		default:
			HALT_ON_ERRORCOND(r->ebx == MSR_TEST_NORMAL);
			r->ebx = MSR_TEST_VMEXIT;
			break;
		}
		// break;
	case VMX_VMEXIT_CPUID:
		return;
	default:
		HALT_ON_ERRORCOND(0 && "Unknown exit reason");
	}
```

In `xmhf64-nest-dev 51ca53716`, serial `20221025161427`. Can see that there are
only 5 202 VMEXITs due to WRMSR. No 202 VMEXITs due to RDMSR. The WRMSR 202
VMEXITs are for MSR values:
* 0xc0000100 `IA32_FS_BASE`
* 0xc0000101 `IA32_GS_BASE`
* 0xc0000102 `IA32_KERNEL_GS_BASE`

The problem happens because `xmhf_parteventhub_arch_x86vmx_handle_rdmsr()` uses
things like `vcpu->vmcs.guest_GS_base`, which is VMCS01. In this case we should
use VMCS02 instead. The WRMSR function has the same issues. Other callers to
`xmhf_parteventhub_arch_x86vmx_handle_rdmsr()` are VMCS02 and VMCS12
translation functions. These functions also need to be reviewed.

The test to MSR bitmap is implemented in `lhv ce69c270c..a717b82cb`. The
commits `xmhf64-nest-dev ecf60f5f3..a8cbe3d6c` on top of `xmhf64-nest` pass
this test. Squashed the changes to `xmhf64 5dcac3287`.

After the change, (Dell, XMHF x64, Windows 10 Hyper-V) can boot. Looks like
Windows 10 is running as a hypervisor (similar to Xen?), and XMHF sees a lot of
EPT02 full messages.

### Create Hyper-V virtual machine

Create virtual machines using Hyper-V and install Windows 10 x64.

In "Hyper-V Settings", change default directory to store virtual machine disks.
* Ref: <https://www.altaro.com/hyper-v/hyper-v-change-default-vhd-location/>

Result
* Dell, amd64 XMHF, win10x64, Hyper-V, win10x64: good (install and run)

### Virtualization Based Security

TODO

