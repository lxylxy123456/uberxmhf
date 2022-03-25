# GRUB 

## Scope
* HP only (QEMU works well)
* DRT only (no DRT works well)
* GRUB graphical mode only (GRUB text mode works well)
* x86 and x64 XMHF
* Git `02343124d`

## Behavior
When using GRUB graphics mode, GRUB does not appear the second time.

The last few lines of serial are:
```
CPU(0x00): INT 15(e820): AX=0xe820, EDX=0x534d4150, EBX=0x00000010, ECX=0x00000014, ES=0x6800, DI=0x0004
CPU(0x00): INT 15(e820): AX=0xe820, EDX=0x534d4150, EBX=0x00000011, ECX=0x00000014, ES=0x6800, DI=0x0004
CPU(0x00): INT 15(e820): AX=0xe820, EDX=0x534d4150, EBX=0x00000012, ECX=0x00000014, ES=0x6800, DI=0x0004
CPU(0x00): WRMSR (MTRR) 0x0000020c 0x00000000c0000001 (old = 0x00000000c0000001)
```

## Debugging

`20220324150509` is a normal x86 XMHF without DRT session.
`20220324150733` is an x86 XMHF with DRT.

In successful one, there are 3 writes to MSR
```
CPU(0x00): WRMSR (MTRR) 0x0000020c 0x00000000c0000001 (old = 0x00000000c0000001)
CPU(0x00): WRMSR (MTRR) 0x0000020d 0x0000000ffe010800 (old = 0x0000000ffe010000)
CPU(0x00): Update EPT memory types due to MTRR
```

But for the DRT one, only the first one is printed.

The debugging code in `91b493b2f` is outdated because of code merge. New
version is git `fded7ace5`.

Git `fded7ace5`, no drt serial `20220324153711`, drt serial `20220324153827`.
Looks like WRMSR on MTRR fails to return to guest mode.

Git `1c5a765ac`, drt serial `20220324154552`

Seems that after removing the quiesce code (line 1 and 3 following, git
`3d242e160`), XMHF can work well. Maybe DRT and NMI handling are incorrect?
```
	xmhf_smpguest_arch_x86vmx_quiesce(vcpu);
	hypapp_status = xmhf_app_handlemtrr(vcpu, msr, val);
	xmhf_smpguest_arch_x86vmx_endquiesce(vcpu);
```

Git `309de9c10` removes print debugging etc. But the problem is why NMI fails.

Related files are
```
smpg-x86vmx.c
peh-x86vmx-main.c
```

### Test quiesce

Git `ebdb5d278`, no drt serial `20220324160822`, drt serial `20220324160954`.
Can see that in DRT, after one CPU requests quiesce, everyone else does
nothing. So it is likely that the NMI is delivered to other CPUs, but a
deadlock or something similar happens.

Git `31dcd368b`, drt serial `20220324162142`. Can see clear evidence that
quiesce code has problems.

Git `fc19a0573`, qemu nodrt serial `20220324163100`, drt serial
`20220324163031`. Now my guess is that NMI is blocked in host / guest.

Git `f227f6bd4`, drt serial `20220324163624`. This time we run IRET at the
beginning of runtime, and problem solved.

Now I can explain what is happening:
* When XMHF runtime is just started
	* Hypervisor NMI blocking is (blocked if DRT / not blocked if no DRT)
	* Guest NMI blocking is (not blocked)
* When running PALs, all CPUs are running, so when one CPU is quiescing, most
  other CPUs are in guest mode, which is NMI not blocked
* However, when running GRUB, APs are busy waiting for SIPI in host mode. So
  NMIs are blocked.

This explains why PALs can run normally, but GRUB always fails.

## Fix

`02343124d..ec78a946a`
* For Intel, unblock NMI when runtime starts

