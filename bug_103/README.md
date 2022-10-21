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

TODO: assertion error

