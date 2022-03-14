# PR from Miao

## Scope
All XMHF

## Behavior
Merge <https://github.com/lxylxy123456/uberxmhf/pull/1>, but before that there
is something wrong with the change. XMHF no longer works.

From now on, in QEMU, need to disable DMAP (configure `--disable-dmap`).

The serial output ends with:
```
CPU(0x00): All cores have successfully been through appmain.
CPU(0x00): setup x86_64vmx SMP guest capabilities
xmhf_runtime_main[00]: starting partition...
CPU(0x00): VMCLEAR success.
CPU(0x00): VMPTRLD success.
CPU(0x00): VMWRITEs success.
CPU(0x00): INT 15(e820): AX=0xe820, EDX=0x534d4150, EBX=0x00000000, ECX=0x00000014, ES=0x6800, DI=0x0004
```

Looks like E820 handling is incorrect.

## Debugging

First revert `flags` in `struct regs`: Apply `a.diff` on commit `2c20edd29`.

### Booting bug

Looked into E820 code, then randomly removed these two lines in
`xmhf_parteventhub_arch_x86_64vmx_intercept_handler()`. Then problem solved.
```c
r->rsp = VCPU_grsp(vcpu);
VCPU_grsp_set(vcpu, r->rsp);
```

Maybe useful:
```
hb _vmx_int15_handleintercept
```

GitHub comment:
<https://github.com/lxylxy123456/uberxmhf/pull/1/files#r825388004>

### PAL bug

After solving Booting bug, the problems are:
* QEMU without DMAP: running PAL results in
  "SECURITY: incorrect scode EPT configuration!"
* HP with DMAP: screen becomes unreadable

Focus on QEMU bug first. `hpt_emhf_set_root_pm_pa()` in
`hpt_scode_switch_scode()` should set `control_EPT_pointer` for the current
CPU, but looks like it is changed back for some reason.

GitHub comment:
<https://github.com/lxylxy123456/uberxmhf/pull/1/files#r825393271>

Git: branch `miao-xmhf64-bugfix`, commit `301ba2827`

### CR0 bug

Found bug in `vmx_handle_intercept_cr0access_ug()` (not related to this PR):
```
diff --git a/xmhf/src/xmhf-core/xmhf-runtime/xmhf-eventhub/arch/x86_64/vmx/peh-x86_64vmx-main.c b/xmhf/src/xmhf-core/xmhf-runtime/xmhf-eventhub/arch/x86_64/vmx/peh-x86_64vmx-main.c
index d0eec7191..6be116825 100644
--- a/xmhf/src/xmhf-core/xmhf-runtime/xmhf-eventhub/arch/x86_64/vmx/peh-x86_64vmx-main.c
+++ b/xmhf/src/xmhf-core/xmhf-runtime/xmhf-eventhub/arch/x86_64/vmx/peh-x86_64vmx-main.c
@@ -604,9 +604,9 @@ static void vmx_handle_intercept_cr0access_ug(VCPU *vcpu, struct regs *r, u32 gp
 			vcpu->vmcs.guest_CR0 |= old_cr0 & pg_pe_mask;
 			//printf("\n[cr0-%02x] RETRY:  old=0x%08llx", vcpu->id,
 			//	vcpu->vmcs.guest_CR0);
-			/* Sanity check: for bits masked, guest CR0 = CR0 shadow */
+			/* Sanity check: for bits masked, requested value = CR0 shadow */
 			HALT_ON_ERRORCOND(
-				((vcpu->vmcs.guest_CR0 ^ vcpu->vmcs.control_CR0_shadow) &
+				((cr0_value ^ vcpu->vmcs.control_CR0_shadow) &
 				vcpu->vmcs.control_CR0_mask) == 0);
 			/*
 			 * Sanity check: for bits not masked other than CR0.PG and CR0.PE,
```

There is something wrong in sanity check. Not a big problem.

### Afterwards

Decided to not change regs structure. Merged PR

## Fix

`215bff899..0240f2b7a`
* PR from Miao: try to add DMAP support to XMHF

