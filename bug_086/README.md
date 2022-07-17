# Reboot an AP and get out of VMX nonroot mode (security)

## Scope
* All subarchs
* `xmhf64 f41b83a6b`

## Behavior

In `bug_075` section `### INIT two times`, INIT-SIPI-SIPI multiple times is
first seen to have strange behaviors. In `bug_085` section
`### Running LHV as nested guest`, I realized that XMHF can be compromised
through INIT-SIPI-SIPI twice. Now it's time to demonstrate this attack.

## Debugging

The logic of attack is
* XMHF boots, wakes up Red OS BSP
* Red OS BSP sends INIT-SIPI-SIPI to AP. The sequence is captured by XMHF, so
  XMHF runs AP in VMX non-root mode
* Red OS BSP sends another INIT-SIPI-SIPI to AP
	* After sending INIT, XMHF runs shutdown code on AP
	* After SIPI, the AP runs Red OS code without VMX

The shutdown code's behavior will affect how the exploit is written. The
shutdown code is `xmhf_baseplatform_arch_x86vmx_reboot()`, which runs VMXOFF
and then shuts down the machine using the 8042 keyboard reset pin (see
<https://wiki.osdev.org/Reboot>). It looks like RESET is similar to INIT. So
in the best case, we do not need to do anything special. Otherwise, we can try:
* Send a lot of INIT's so that AP does not reach keyboard reset
* Try to send a lot of keyboard and mouse events?
* See whether it is possible to reorder I/O ports so that the AP cannot access
  keyboard.

We can use `lhv-dev`'s previous work on guest mode to write the malicious OS,
this allows us to demonstrate TrustVisor being hacked.

### Support user mode

Previous LHV work that supports user mode (ring 3) is in `bug_084` at
`lhv-dev 77c71f01b..ba6290f05` (base is `lhv 5eb712013`). This is re-applyed
in `lhv-dev 12533871a`.

Back in `bug_084`, user to kernel mode transition was not supported, because it
is not needed. When user mode receives an interrupt / exception, triple fault
happens. Now we support it to make it more clean. Looks like we just need to
place the correct values in TSS.

In `lhv-dev 70e5a8a66`, set correct TSS and solve the triple fault problem.
Also set up system call to allow the guest go back to supervisor cleanly.
However, for some reason after going to guest mode then back, VMLAUNCH fails
with error code 8 (VMENTRY with invalid host state).

We also need to have different ESP0 for each CPU. So this means each CPU has
its own TSS and TR. For XMHF we can easily create 10 entries in GDT. In 15410
there is one unique GDT per CPU.

Nest steps:
* Solve VMENTRY problem
* One TSS and TR per CPU to be able to run guest mode in SMP

The VMENTRY problem happens because ES, FS, and GS are in ring 3. Fixed in
`lhv-dev 39bc0ad41`.

In `lhv-dev b934fec82`, implemented one GDT, TR, and TSS per CPU (similar to
15410). Now can enter user mode in different CPUs.

In `lhv-dev 0b5689d60`, can run 64-bit user mode well. However, looks like when
two CPUs run TrustVisor at the same time, a bug will happen and cause things to
halt. The problem can be worked around by restricting access to TrustVisor one
at a time using a spin lock.

Looks like this is a problem with tlsf allocation in Trust Visor. So we
workaround this problem by allowing at most 2 CPUs access Trust Visor at the
same time. We do not have a good mutex, so just poll.

The changes for supporting user mode are in `lhv-dev de9fa0197..99161120b`.
They are squashed to `lhv fef5e7fdd`.

### Construct attack

The key exploit code is in `lhv-dev e772b8786`. For a normal machine with 2
CPUs, when both CPUs are running, CPU 0 needs one INIT-SIPI-SIPI sequence to
restart CPU 1 to real mode. In XMHF, guest CPU 0 needs to first INIT to
shutdown XMHF. Then CPU 0 runs INIT-SIPI-SIPI to put CPU 1 to real mode.

This attack works in QEMU / KVM. Looks like when CPU 1 accesses the 8402
keyboard reset pin, CPU 0 is not affected too much. Maybe CPU 0's interrupts
are gone. Nevertheless, CPU 0 can issue the remaining interrupts and make CPU 1
run the real mode code.

In Bochs, looks like the interrupts for BSP are not gone. However, need to
confirm whether AP is running. Maybe the best way is to print to the screen.
Fortunately we can copy some code from `bug_085` in `xmhf64-dev 51f41b5b9`.

In `lhv-dev a9b726e51`, write real mode code to let AP draw to screen. In QEMU,
looks the devices are lost, even for BSP. In GDB, when running
`x/200c 0xb8000`, will see 0s (instead of what is seen on the screen).
However, in Bochs can see that AP is running and drawing to the screen.

In `lhv-dev 521d60775`, if we decrease the delay of the first INIT, then QEMU
will likely not access the 8042 keyboard reset pin. As a result, the exploit
becomes also reproducible on QEMU.

At this point, it is better to try running the programs on real hardware early.
Maybe a detailed exploit will require writing BIOS-like code to reset the
devices. Also I am not sure whether the behavior will differ when running under
TXT. We also need to be careful that when printfs are removed, things run
faster and may produce different results.

TODO: try on real hardware
TODO: remove printf and see behavior

