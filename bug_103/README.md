# Nested virtualization does not handle EPT01 flush correctly

## Scope
* SMP
* All configurations with nested virtualization on
* `xmhf64 c36d37b38`
* `xmhf64-nest a2b8a1bb8`

## Behavior
Suppose CPU 0 of the guest is running nested guest. CPU 1 of the guest runs PAL
and causes XMHF to execute INVEPT. Ideally all VMCS02 fields that depend on
EPT01 should be flushed. However, curently only EPT02 is correctly handled.
Other fields (like IO bitmaps) are not invalidated.

## Debugging

After reviewing `xmhf_nested_arch_x86vmx_vmcs12_to_vmcs02()` and
`xmhf_nested_arch_x86vmx_vmcs02_to_vmcs12()`, looks like the related VMCS
fields are mostly simple `FIELD_PROP_GPADDR`. So we only need to run relevant
translations in `xmhf_nested_arch_x86vmx_vmcs12_to_vmcs02()`. There is no need
to call `xmhf_nested_arch_x86vmx_vmcs02_to_vmcs12()`

Wrote `xmhf_nested_arch_x86vmx_rewalk_ept01` to only perform translations for
`FIELD_PROP_GPADDR` fields. Git `xmhf64-nest a2b8a1bb8..a8ce63845`.

### Group nested-x86vmx-vmcs12.c into functions

In `31943437c..1b3bc9ba0`, break `xmhf_nested_arch_x86vmx_vmcs02_to_vmcs12()`
and `xmhf_nested_arch_x86vmx_vmcs12_to_vmcs02()` to smaller static functions.

Created branch `xmhf64-nest-tmp` and PR 19:
<https://github.com/lxylxy123456/uberxmhf/pull/19>

Merged to `xmhf64-nest a5adb60e6`.

### `cpu_has_current_vmcs12(vcpu)` assertion error

During testing, start `./test_args64 7 700000000 7` and VMware Windows 10 on
Dell (looks like ). After some time see assertion error:
```
Fatal: Halting! Condition 'cpu_has_current_vmcs12(vcpu)' failed, line 372, file arch/x86/vmx/nested-x86vmx-handler1.c
```

The build command is
```sh
./build.sh amd64 O3 fast --mem 0x230000000 --event-logger && gr
```

Looks like the bug happens in VMware BIOS, so a different OS may also reproduce
this problem.

Looks like this error happens after PR 19. This is also reproducible without
optimization. By adding `asm volatile ("ud2");` to `HALT_ON_ERRORCOND`, can
obtain stack trace easily. See `stack_trace1.txt`.

The exception call stack is not interesting
```
XtRtmIdtStub2
  xmhf_xcphandler_hub
    xmhf_xcphandler_arch_hub
      xmhf_smpguest_arch_x86_eventhandler_nmiexception
        xmhf_smpguest_arch_x86vmx_eventhandler_nmiexception
          xmhf_smpguest_arch_x86vmx_nmi_check_quiesce
            xmhf_memprot_flushmappings_localtlb
              xmhf_memprot_arch_flushmappings_localtlb
                xmhf_memprot_arch_x86vmx_flushmappings_localtlb
                  xmhf_nested_arch_x86vmx_flush_ept02
                    xmhf_nested_arch_x86vmx_flush_ept02_effect
                      xmhf_nested_arch_x86vmx_find_current_vmcs12
                        HALT_ON_ERRORCOND(cpu_has_current_vmcs12(vcpu));
```

The NMI interrupted code is
`xmhf_nested_arch_x86vmx_handle_vmxon+631 = 0x10209d3c`, which is

```
                                        vcpu->vmx_nested_is_vmx_operation = 1;
    10209d20:   48 8b 85 78 ff ff ff    mov    -0x88(%rbp),%rax
    10209d27:   c7 80 58 05 00 00 01    movl   $0x1,0x558(%rax)
    10209d2e:   00 00 00 
                                        vcpu->vmx_nested_vmxon_pointer = vmxon_ptr;
    10209d31:   48 8b 55 d8             mov    -0x28(%rbp),%rdx
    10209d35:   48 8b 85 78 ff ff ff    mov    -0x88(%rbp),%rax
->  10209d3c:   48 89 90 60 05 00 00    mov    %rdx,0x560(%rax)
                                        vcpu->vmx_nested_is_vmx_root_operation = 1;
    10209d43:   48 8b 85 78 ff ff ff    mov    -0x88(%rbp),%rax
    10209d4a:   c7 80 68 05 00 00 01    movl   $0x1,0x568(%rax)
    10209d51:   00 00 00 
                                        vcpu->vmx_nested_cur_vmcs12 = INVALID_VMCS12_INDEX;
    10209d54:   48 8b 85 78 ff ff ff    mov    -0x88(%rbp),%rax
    10209d5b:   c7 80 6c 05 00 00 ff    movl   $0xffffffff,0x56c(%rax)
    10209d62:   ff ff ff 
```

So the order of `xmhf_nested_arch_x86vmx_handle_vmxon` should be changed. It is
better to set `vmx_nested_is_vmx_root_operation` before
`vmx_nested_is_vmx_operation`. However, it is even better to merge these two
variables, as there are only 3 possible states. Also simply changing the order
in `xmhf_nested_arch_x86vmx_handle_vmxon()` seems to create problems in O3
(e.g. memory barrier needed).

Fixed in `xmhf64-nest 415ae5adf`.

Also added official VMCS field name from Intel SDM in `xmhf64-nest ec5f0e368`.

## Fix

`xmhf64-nest a2b8a1bb8..ec5f0e368`
* Update VMCS02 fields during EPT01 flush
* Split VMCS translation to small function
* Merge variables to track VMX mode (fix EPT02 flush bug)
* Add official VMCS field names in nested-x86vmx-vmcs12-fields.h

