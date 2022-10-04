# Test on Dell Desktop

## Scope
* All configurations
* `xmhf64 427b6d6b7`

## Behavior
Recently received a Dell desktop. Should test XMHF on it.

The machine info is:
* CPU: `Intel(R) Core(TM) i5-7600 CPU @ 3.50GHz`
* Address sizes: 39 bits physical, 48 bits virtual
* Memory: 7.6Gi
	* 0x0000000000000000-0x00000000cfffffff
	* 0x0000000100000000-0x000000022fffffff

## Debugging

### Set up AMT

For some reason, setting on Dell is not able to enable SOL. Need to
install MeshCommander on private laptop (that controls Dell). Official website:
<https://www.meshcommander.com/meshcommander>. Can use a Windows machine to
control, or install on Fedora using npm:
<https://www.npmjs.com/package/meshcommander>
```sh
sudo dnf install npm
cd /path/to/meshcommander/install
npm install meshcommander
cd ./node_modules/meshcommander
node meshcommander
# Open http://127.0.0.1:3000/default.htm
```

Add the remote computer, then connect. Can see "Serial-over-LAN" tab. Then can
connect. Maybe need MeshCommander to remotely enable SOL. Also have
"Remote Desktop" to control KVM (keyboard, video, mouse) of Dell, using VNC
protocol but on port 16994.

To use `amtterm` from Linux command line, need to use version 1.7 or above. It
is easy to compile from source. Git is <https://git.kraxel.org/cgit/amtterm/>.

dmesg shows that the serial port for SOL is on I/O port 0xf0a0, so need to
change GRUB accordingly:
`0000:00:16.3: ttyS1 at I/O 0xf0a0 (irq = 20, base_baud = 115200) is a 16550A`

Difference of this configuration compared to HP 2540p is
* `amttool` no longer works, need to use MeshCommander to reset the machine
* `amtterm` will no longer print "The system is powered on." and "The system is
  powered off." So need to chop output file manually.

### Memory problem

With the default settings, will halt because `MAX_PHYS_ADDR` is too small. The
maximum memory should be set to `0x230000000`, or 8.75 GiB. Use
`--mem 0x230000000` in `build.sh`.

The complete command is now
```sh
./build.sh amd64 --event-logger --mem 0x230000000 && gr
make -j 4 && gr && copyxmhf && hpgrub XMHF-build64 0 && hpinit6
```

### Halt before PCI initialize

Currently, see some error when Linux is booting. However, when using nested
virtualization (git `xmhf64-nest 3ca7e20f5`) and the following compile options,
see error earlier.

```sh
./build.sh amd64 fast O3 circleci --no-x2apic --event-logger --mem 0x230000000
```

The machine halts after printing the following:
```
SL: setup runtime paging structures.
Transferring control to runtime
```

Normally, should see after that:
```
runtime initializing...
xmhf_baseplatform_arch_x86_64_pci_initialize: PCI type-1 access supported.
xmhf_baseplatform_arch_x86_64_pci_initialize: PCI bus enumeration follows:
xmhf_baseplatform_arch_x86_64_pci_initialize: Done with PCI bus enumeration.
```

Try to bisect the configurations. Currently only connecting using SOL (not
connecting remote desktop). DP screen is attached. "Good" below means can see
GRUB the second time
* `xmhf64` branch, `amd64 fast O3 circleci --no-x2apic --event-logger`: not
  tested
* `xmhf64` branch, `amd64 fast O3`: stuck at `Transferring control to runtime`
* `xmhf64` branch, `amd64 O3`: good
* `xmhf64` branch, `amd64`: good
* `xmhf64` branch, `i386 fast`: stuck at `Transferring control to runtime`
	* This one may be wrong
* `xmhf64` branch, `i386`: good
* `xmhf64` branch, `i386 --no-rt-bss`: good
* `xmhf64` branch, `i386 --no-bl-hash`: good
* `xmhf64` branch, `i386 --no-rt-bss --no-bl-hash`: good
* `xmhf64` branch, `i386 fast`: good
* `xmhf64` branch, `amd64 fast`: good
* `xmhf64` branch, `amd64 fast O3`: stuck at `Transferring control to runtime`
* `xmhf64` branch, `i386 fast O3`: good
* `xmhf64` branch, `amd64 O3`: good
* `xmhf64` branch, `amd64 --no-rt-bss O3`: stuck at
  `Transferring control to runtime`
* `xmhf64` branch, `amd64 O3`: good
* `xmhf64` branch, `amd64 --no-rt-bss`: stuck at
  `Transferring control to runtime`
* `xmhf64` branch, `amd64`: good
* `xmhf64` branch, `amd64 --no-rt-bss`: stuck at `SL: RPB, magic=0xf00ddead`
  (earlier than usual)

So the problem should be related to the `--no-rt-bss`, however it is not very
stable.

I am trying to trigger a triple fault using the code snippet below. However,
looks like Dell does not reset at triple fault. I tried it in bootloader and
secure loader. The machine just halts.
```c
	asm volatile (
		"pushl $0;"
		"pushl $0;"
		"lidt (%esp);"
		"ud2;"
	);
```

Maybe the best way to communicate with the outside world is to use the VGA:
```
	/* SL */
	asm volatile ("movq $0xf00b8000, %%rax; 1: incb (%%rax); jmp 1b;":::"%rax");
	/* RT */
	asm volatile ("1: incb 0xb8000; jmp 1b;");
```

Fortunately we are not dealing with DRT at this time, or things will be
complicated.

The VGA trick works. Once the screen responds after the `movq %r8, %cr3`
instruction (switch to runtime paging) in `sl-x86-amd64-sup.S`. However it is
hard to reproduce it (i.e. not stable).

The possible places to trigger the bug are:
* After `SL: RPB, magic=0xf00ddead`
	* `sl.c:149-195`
* Around `jmpq    *%rax`

I realized that in `sl-x86-amd64-sup.S`, after the call to
`xmhf_setup_sl_paging`, the TLB needs to be flushed. It can be flushed by MOV
to CR3. Fixed in `xmhf64 d6ce3f2ad`.

After that, still may stuck at `SL: RPB, magic=0xf00ddead`.

Actually, `sl-x86-amd64-entry.S` may also be in trouble. First identity paging
in 64-bit mode is set. Then offset is set (to simulate address change caused
by segmentation in 32-bit). However, TLB is not flushed. Fixed in
`xmhf64 2f01c38a1`.

After the fix, everything looks good.

### Linux panic

Basically in all configurations, see Linux kernel panic during boot (before VGA
resolution increase).

In Linux, can use `console=ttyS1` to output dmesg to serial port. Otherwise the
panic message is too long.

```sh
./build.sh amd64 fast --mem 0x230000000 && gr
make -j 4 && gr && copyxmhf && hpgrub XMHF-build64 0 && hpinit6
```

To help debugging, `nokaslr` is disabled.

Using the build command above, Linux dmesg is in `linux_panic1.txt`. Looks like
some problem happens in FPU. Note that there are a few FPU warnings before
panic. Finally the FPU problem causes kernel stack overflow, which causes the
panic.

Can also add `nosmp` to Linux, and the problem is still reproducible.

The first warning message in `linux_panic1.txt` happens very early. The warning
happens on this line:
<https://elixir.bootlin.com/linux/v5.10.140/source/arch/x86/include/asm/fpu/internal.h#L306>

```
/*
 * This function is called only during boot time when x86 caps are not set
 * up and alternative can not be used yet.
 */
static inline void copy_kernel_to_xregs_booting(struct xregs_state *xstate)
{
	u64 mask = -1;
	u32 lmask = mask;
	u32 hmask = mask >> 32;
	int err;

	WARN_ON(system_state != SYSTEM_BOOTING);

	if (boot_cpu_has(X86_FEATURE_XSAVES))
		XSTATE_OP(XRSTORS, xstate, lmask, hmask, err);
	else
		XSTATE_OP(XRSTOR, xstate, lmask, hmask, err);

	/*
	 * We should never fault when copying from a kernel buffer, and the FPU
	 * state we set at boot time should be valid.
	 */
	WARN_ON_FPU(err);
}
```

So `XSTATE_OP` receives an exception and triggers the warning. The
`X86_FEATURE_XSAVES` macro refers to CPUID
"Processor Extended State Enumeration Sub-leaf (EAX = 0DH, ECX = 1)" with bit 3
in EAX (i.e. `CPUID.(0DH, 1):EAX.[3]`). The description of this bit is:
> Supports XSAVES/XRSTORS and IA32_XSS if set

Then the XRSTORS instruction gives us a hint. We go to Intel volume 3c, and see
that:
> XRSTORS. Behavior of the XRSTORS instruction is determined first by the
> setting of the "enable XSAVES/XRSTORS" VM-execution control:
>
> - If the "enable XSAVES/XRSTORS" VM-execution control is 0, XRSTORS causes an
> invalid-opcode exception (#UD).
>
> - If the "enable XSAVES/XRSTORS" VM-execution control is 1, treatment is
> based on the value of the XSS-exiting bitmap (see Section 23.6.20):
> ...

So I guess the story is that
* Linux needs FPU, obviously
* In previous machines I have tested, XSAVE/XRSTOR is supported, but
  XSAVES/XRSTORS is not. So everything works fine in VMX mode.
* In this new Dell machine, XSAVES/XRSTORS becomes supported and is reported in
  CPUID, so Linux uses it. However, the VMX configuration does not say so, so
  Linux gets #UD.

The implication of this bug is that XMHF may never become perfect. Currently
XMHF tries to provide all features from the hardware to the guest. However, in
this case it needs to know everything about the VMX control bits. In this case
setting a bit to 0 causes incorrect behavior. Ideally, after each release of
SDM, XMHF should be reviewed to react to new features correctly.

To fix, simply set "enable XSAVES/XRSTORS" to 1. However, this also requires
setting the "XSS-exiting bitmap" to 0. This is a problem because in some
machines, this VMCS field is not available. In others, this is available.
However, XMHF is not very good at handling VMCS fields that do not exist.

In `xmhf64 3266af64f`, set "enable XSAVES/XRSTORS" but did not touch
"XSS-exiting bitmap". Debian can already successfully boot (with `nosmp`). Also
can successfully boot with SMP.

In `xmhf64-dev 7d1ea3bee`, add the VMCS field `control_XSS_exiting_bitmap` to
XMHF. As expected, 2540P does not have this field and encouters regression.

In `xmhf64-dev 275e2e6c4`, be able to skip a VMCS field due to not existing.
Rebased `xmhf64-dev 4da3af3e5..275e2e6c4` to `xmhf64 6c6c3294b`.

### Testing nested virtualization

After git merging, we are at `xmhf64-nest fbd6319d3`. Using
`./build.sh amd64 fast --mem 0x230000000 --event-logger`, we build nested
virtualization and copy to the Dell desktop.

Looks like KVM can work well booting 15410 Pebbles Kernel.

Looks like by default XSETBV will cause VMEXITs, and VMEXITs will print a
message. This becomes too much messages on the serial port when trying to boot
Debian in KVM. Looks like the cause is in KVM code
<https://elixir.bootlin.com/linux/v5.10.140/source/arch/x86/kvm/x86.c#L892>:

```
void kvm_load_host_xsave_state(struct kvm_vcpu *vcpu)
{
	if (static_cpu_has(X86_FEATURE_PKU) &&
	    (kvm_read_cr4_bits(vcpu, X86_CR4_PKE) ||
	     (vcpu->arch.xcr0 & XFEATURE_MASK_PKRU))) {
		vcpu->arch.pkru = rdpkru();
		if (vcpu->arch.pkru != vcpu->arch.host_pkru)
			__write_pkru(vcpu->arch.host_pkru);
	}

	if (kvm_read_cr4_bits(vcpu, X86_CR4_OSXSAVE)) {

		if (vcpu->arch.xcr0 != host_xcr0)
			xsetbv(XCR_XFEATURE_ENABLED_MASK, host_xcr0);

		if (vcpu->arch.xsaves_enabled &&
		    vcpu->arch.ia32_xss != host_xss)
			wrmsrl(MSR_IA32_XSS, host_xss);
	}

}
```

Basically KVM is doing the right thing of updating XCR0 when host and guest
values do not match. We simply need to remove the `printf()` calls and Debian
can boot. Fixed in `xmhf64 ac3f3a510`. VirtualBox also looks good for booting
x64 and x86 Debian 11. VMware can boot x86, x86pae, and x64 Debian 11.

The problem is that VirtualBox cannot boot x86 Debian 11 with PAE. This happens
even without XMHF. The host machine outputs:
```
[  258.156665] 
[  258.156665] !!Assertion Failed!!
[  258.156665] Expression: (null)
[  258.156665] Location  : /home/vbox/tinderbox/debian11.0-amd64-build-VBox-6.1/svn/src/VBox/VMM/VMMAll/VMAll.cpp(280) int VMSetRuntimeErrorV(PVMCC, uint32_t, const char*, const char*, __va_list_tag*)
[  258.156669] Congratulations! You will have the pleasure of debugging the RC/R0 path.
```

Looks like in VirtualBox configuration,
`Settings -> System -> Process -> Enable PAE/NX` need to be checked. It is
strange that VirtualBox ends up with an assertion error, but not something to
be reported. See <https://forums.virtualbox.org/viewtopic.php?f=6&t=94677>.

Also, interestingly on 2540P, `Enable PAE/NX` does not need to be checked.

After checking `Enable PAE/NX`, XMHF VirtualBox Debian 11 PAE also works well.

### Testing DRT and DMAP

Running DMAP looks fine.

Since this CPU is 7th generation i5, need to download SINIT ACM from Intel.
The file names are `6th_7th_gen_i5_i7-SINIT_74.zip` and
`6th_7th_gen_i5_i7_SINIT_79.BIN`. However, running DRT does not work well.

Will work on it in `bug_097`.

## Fix

`xmhf64 427b6d6b7..ac3f3a510`
* Flush TLB in SL
* Enable XSAVES/XRSTORS instructions in the guest
* Allow some VMCS fields to be not existing in XMHF

