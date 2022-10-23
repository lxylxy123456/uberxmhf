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
	* TODO
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

TODO: <https://learn.microsoft.com/en-us/virtualization/hyper-v-on-windows/reference/hyper-v-requirements>
TODO: check whether Windows requires some VMX features
TODO: print all RDMSR ECX
TODO: check Windows settings about VBS

