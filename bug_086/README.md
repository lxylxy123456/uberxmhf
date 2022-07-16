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

TODO: LHV cannot VMLAUNCH correctly after entering guest mode
TODO: one TSS and TR per CPU

