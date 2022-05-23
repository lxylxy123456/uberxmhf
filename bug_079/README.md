# Implement nested virtualization

## Scope
* x86 and x64
* XMHF git `78b56c9ca`
* LHV git `14b8c6c19`

## Behavior
We should start implementing nested virtualization. Currently LHV already has
some simple functionalities.

## Debugging

Creating a new branch `xmhf64-nest` for nested virtualization feature.

First we see `Unhandled intercept: 27`. We need to handle intercepts due to
executing VMX instructions. See "APPENDIX C VMX BASIC EXIT REASONS"
* 18: VMCALL
* 19: VMCLEAR (m64)
* 20: VMLAUNCH
* 21: VMPTRLD (m64)
* 22: VMPTRST (m64)
* 23: VMREAD (r/m32, r32; r/m64, r64)
* 24: VMRESUME
* 25: VMWRITE (r32, r/m32; r64, r/m64)
* 26: VMXOFF (m64)
* 27: VMXON

We need to find a way to get the operands of these instructions. This is
provided in "26.2.5 Information for VM Exits Due to Instruction Execution",
"Table 26-13" and "Table 26-14" (page 993).

```
(gdb) x/i vcpu->vmcs.guest_RIP
   0x820442c:	vmxon  -0x18(%rbp)
(gdb) p/x vcpu->vmcs.info_vmx_instruction_information
$2 = 0x62e148ac
(gdb) 
```

TODO: Use `_vmx_decode_reg()` to convert from integer to register

TODO (optional): change `struct regs` to match Intel's order in table 26-13
* In `_processor.h`: `struct regs`
* In `xmhf-baseplatform-arch-x86.h`: `enum CPU_Reg_Sel`
* In `peh-x86-amd64vmx-entry.S`: asm instructions for VMEXIT and VMRESUME
* In `part-x86-amd64vmx-sup.S`: asm instructions for VMLAUNCH
* In `lhv-asm.S`: for lhv
* In `xcph-stubs-amd64.S`: for exception handling
* The list above can be constructed by simply searching for `r13` in git

TODO

## Fix

`78b56c9ca..` (96f0022ac)
* Add nested virtualization configuration option

