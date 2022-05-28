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

Decoding the instruction information manually
```
0b0110 0010 1110 0001 0100 1000 1010 1100
                                       ^^ Scaling = 1 (0)
                                 ^^^ ^^   Undefined
                             ^^ ^         Address size = 32-bit (1)
                            ^             0
                       ^^^ ^              Undefined
                   ^^ ^                   Segment register = SS (2)
              ^^ ^^                       Index register = R8 (8)
             ^                            Index register is invalid
        ^^^ ^                             Base register = RBP (5)
       ^                                  Base register is valid
  ^^^^                                    Undefined
```

Another example: using x64 LHV and x64 XMHF
```
(gdb) x/i vcpu->vmcs.guest_RIP
   0x8207a1a:	vmxon  -0x18(%rbp)
(gdb) p/x vcpu->vmcs.info_vmx_instruction_information
$1 = 0x62e1492c
(gdb) p/x vcpu->vmcs.info_vmexit_instruction_length 
$2 = 0x5
(gdb) x/5b vcpu->vmcs.guest_RIP
0x8207a1a:	0xf3	0x0f	0xc7	0x75	0xe8
(gdb) p/x vcpu->vmcs.info_exit_qualification
$3 = 0xffffffffffffffe8
(gdb) 
```

```
0b0110 0010 1110 0001 0100 1001 0010 1100
                                       ^^ Scaling = 1 (0)
                                 ^^^ ^^   Undefined
                             ^^ ^         Address size = 64-bit (2)
                            ^             0
                       ^^^ ^              Undefined
                   ^^ ^                   Segment register = SS (2)
              ^^ ^^                       Index register = R8 (8)
             ^                            Index register is invalid
        ^^^ ^                             Base register = RBP (5)
       ^                                  Base register is valid
  ^^^^                                    Undefined
```

The displacement is stored in exit qualification:
> For INVEPT, INVPCID, INVVPID, LGDT, LIDT, LLDT, LTR, SGDT, SIDT, SLDT, STR,
> VMCLEAR, VMPTRLD, VMPTRST, VMREAD, VMWRITE, VMXON, XRSTORS, and XSAVES, the
> exit qualification receives the value of the instruction’s displacement
> field, which is sign-extended to 64 bits if necessary (32 bits on processors
> that do not support Intel 64 architecture). If the instruction has no
> displacement (for example, has a register operand), zero is stored into the
> exit qualification.

Git `c5ed37782` implements decoding the information from VMCS to an address in
guest. Now we need to access this memory in the guest. Microcode update already
has such code. We need to extract a module from here.

As a side project, in git `78b56c9ca..123ee41f6` (branch `xmhf64`) changed
`struct regs` to match Intel's order in table 26-13
* In `_processor.h`: `struct regs`
* In `xmhf-baseplatform-arch-x86.h`: `enum CPU_Reg_Sel`
* In `peh-x86-amd64vmx-entry.S`: asm instructions for VMEXIT and VMRESUME
* In `part-x86-amd64vmx-sup.S`: asm instructions for VMLAUNCH
* In `lhv-asm.S`: for lhv
* In `xcph-stubs-amd64.S`: for exception handling
* The list above can be constructed by simply searching for `r13` in git

In `123ee41f6..173be4f5b`, extracted guest virtual memory accessing code from
ucode. The new file is called `peh-x86vmx-guestmem.c`.

### Possible KVM VMXON bug

When executing VMXON, if LHV does not set CR0.NE, KVM will not error. However,
according to Intel documentation, I think KVM should inject `#GP(0)` to guest.
Note: CR0 fixed MSRs are 0x80000021 0xffffffff.

For example, XMHF git `156c56e47`, LHV git `fb3ab6524`, KVM + LHV succeeds, but
KVM + XMHF + LHV errors (detects invalid CR0 when VMXON). This is fixed in LHV
git `342f0d7ac`.

By digging into `bug_070`'s notes, we can see that the error happened at
VMLAUNCH instead.

In lhv git `75ad1f001`, change LHV so that it can run on VGA guest. Confirmed
on Touch that KVM is buggy. The expected behavior is to raise `#GP(0)`.

Bug reported: <https://bugzilla.kernel.org/show_bug.cgi?id=216033>

### Continue working on VMXON intercept

Looks like "current-VMCS pointer" being invalid means it to be
`FFFFFFFF_FFFFFFFFH`. This is not very clear in Intel's documentation. The best
quote I can find is
> If the operand is the current-VMCS pointer, then that pointer is made invalid
> (set to FFFFFFFF_FFFFFFFFH).

In `bb283aba2`, basically finished VMXON.

We see that in some conditions, VMX instructions should result in `#UD`. We
see whether they can be categorized into one function.

```
INVEPT		IF (not in VMX operation) or (CR0.PE = 0) or (RFLAGS.VM = 1) or (IA32_EFER.LMA = 1 and CS.L = 0)
INVVPID		IF (not in VMX operation) or (CR0.PE = 0) or (RFLAGS.VM = 1) or (IA32_EFER.LMA = 1 and CS.L = 0)
VMCLEAR		IF (register operand) or (not in VMX operation) or (CR0.PE = 0) or (RFLAGS.VM = 1) or (IA32_EFER.LMA = 1 and CS.L = 0)
VMLAUNCH	IF (not in VMX operation) or (CR0.PE = 0) or (RFLAGS.VM = 1) or (IA32_EFER.LMA = 1 and CS.L = 0)
VMPTRLD		IF (register operand) or (not in VMX operation) or (CR0.PE = 0) or (RFLAGS.VM = 1) or (IA32_EFER.LMA = 1 and CS.L = 0)
VMPTRST		IF (register operand) or (not in VMX operation) or (CR0.PE = 0) or (RFLAGS.VM = 1) or (IA32_EFER.LMA = 1 and CS.L = 0)
VMREAD		IF (not in VMX operation) or (CR0.PE = 0) or (RFLAGS.VM = 1) or (IA32_EFER.LMA = 1 and CS.L = 0)
VMWRITE		IF (not in VMX operation) or (CR0.PE = 0) or (RFLAGS.VM = 1) or (IA32_EFER.LMA = 1 and CS.L = 0)
VMXOFF		IF (not in VMX operation) or (CR0.PE = 0) or (RFLAGS.VM = 1) or (IA32_EFER.LMA = 1 and CS.L = 0)
VMXON		IF (register operand) or (CR0.PE = 0) or (CR4.VMXE = 0) or (RFLAGS.VM = 1) or (IA32_EFER.LMA = 1 and CS.L = 0)
```

`_vmx_nested_check_ud()` is written to reduce the checking code.

### Tracking all VMCS

The next question is, does KVM track each page that is VMCLEAR'ed?
`172562fdf..85a505928` tests it. By looking at guest memory usage, looks like
for each VMCS, 4K more memory is used (for storing the first 4 bytes). However,
it is likely that the extra information goes to kernel memory.

To investigate further, there are 2 ways
1. Debug KVM
2. Find a way to compute free kernel memory, compute difference

From Intel v3 "23.11.1 Software Use of Virtual-Machine Control Structures", it
looks like a good hypervisor should VMCLEAR all active VMCS's before recycling
the memory for VMCS. So our hypervisor should be able to track all active
VMCS's. However, if a hypervisor executes VMXOFF with active VMCS's, it is
arguably still following the spec.

Another way is to use O(1) extra memory. This should be possible. However,
later we may need to cache VMCS (similar to caching EPT) to be efficient.

Things to be tracked for a VMCS (see "Figure 23-1. States of VMCS X")
* launch state (clear / launched)
* active / inactive
* current / not current

TODO

## Fix

`78b56c9ca..` (96f0022ac)
* Add nested virtualization configuration option

