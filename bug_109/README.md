# Reproduce TrustVisor Vulnerabilities

## Scope
* All configurations
* `lhv a5052686d`
* `lhv-dev e94c95a72`
* `xmhf64 acadeccf0`
* `xmhf64-dev 9188f4399`

## Behavior

Found the following bugs in TrustVisor. We can now try to reproduce and fix
them.

1. `bug_091` "Testing INIT-SIPI-SIPI twice exploit (`bug_086`)": DRT incorrect
   shutdown, may leak memory
2. `bug_106` "Possible vulnerability about TrustVisor found": does not clean
   running PALs' memory in `tv_app_handleshutdown()`
3. `bug_106` "TrustVisor Vulnerability 1": break memory integrity of guest OS
4. `bug_106` "TrustVisor Vulnerability 2": race condition when changing EPTP

## Debugging

### 1. DRT incorrect shutdown, may leak memory

From MLE SDM "2.4.6 Protecting Secrets", looks like XMHF should set
`TXT.CMD.SECRETS`. This is also done in tboot:

```c
1092  void txt_post_launch(void)
1093  {
...
1135      /* always set the TXT.CMD.SECRETS flag */
1136      write_priv_config_reg(TXTCR_CMD_SECRETS, 0x01);
1137      read_priv_config_reg(TXTCR_E2STS);   /* just a fence, so ignore return */
1138      printk(TBOOT_INFO"set TXT.CMD.SECRETS flag\n");
...
1144  }
```

In `lhv-dev 0e94db268`, write some known bytes to memory and see whether they
persist after reboot. The experiment is:
* Start lhv, which writes to memory
* Reset Dell 7050 (use AMT, `Mesh Commander -> Power Actions -> Reset`)
* Start lhv again, which reads memory. See most memory are intact (vulnerable)

Now we test XMHF
* Start `xmhf64 0b1bcb839`, with DRT and DMAP
* Start lhv, write to memory
* Reset Dell 7050 using AMT
* Start xmhf64, with DRT and DMAP
* Start lhv, read from memory. See most memory are intact (vulnerable)

We test other ways to reset the system
* Start xmhf64, lhv
* Press power button to power off, then press again to power on
* Start xmhf64, lhv, looks like most memory are lost.

However there is a pattern
```
  0x0000000100000000: c45c026d 96c8a744 86c9b745d45d126c
...
  0x000000022b000000: c45c026d 96c8a744 86c9b745d45d126c
  0x000000022c000000: c45c026d 96c8a744 86c9b745d45d126c
  0x000000022d000000: c45c026d 96c8a744 86c9b745d45d126c
```

The above pattern is also reproduced at a cold start
* Start xmhf64, lhv
* Press power button to power off
* Cut AC power supply to the machine
* Wait for more than 1 minute
* Restore AC power
* Press power button to power on
* Start xmhf64, lhv, see `c45c026d 96c8a744 86c9b745d45d126c` pattern

We try crashing XMHF. In theory it can be implemented using the "INIT-SIPI-SIPI
twice exploit". For simplicity we simply triple fault XMHF when LHV triple
faults.
* Start `xmhf64-dev 3da2654e5`, with DRT and DMAP
* Start `lhv-dev 4868c7a21`, write to memory, sleep, triple fault
* Start xmhf64, with DRT and DMAP
* Start lhv, read from memory. See most memory are intact (vulnerable)

Fix this bug in `xmhf64 01a889ad5`. Set `TXT.CMD.SECRETS` in SL. This way when
XMHF triple faults, looks like the machine takes longer to reboot. The memory
is cleared to another pattern:

```
  0x0000000100000000: 0e3075ee 6d28b3df 6d29b3de0e3175ef
...
  0x000000022b000000: 0e3075ee 6d28b3df 6d29b3de0e3175ef
  0x000000022c000000: 0e3075ee 6d28b3df 6d29b3de0e3175ef
  0x000000022d000000: 0e3075ee 6d28b3df 6d29b3de0e3175ef
```

Now the behavior is
* Start `xmhf64 8a4b3ba84`, with DRT and DMAP
* Start lhv, write to memory
* Reset Dell 7050 using AMT
* Start xmhf64, with DRT and DMAP
* Start lhv, read from memory. See memory are cleaned (secure)

Also
* Start xmhf64, with DRT and DMAP
* Start lhv, write to memory
* xmhf64 triple fault, Dell automatically reboots
* Start xmhf64, with DRT and DMAP
* Start lhv, read from memory. See memory are cleaned (secure)

At this point, XMHF running DRT should be considered secure. However, when not
running DRT, the INIT-SIPI-SIPI twice exploit is still a security risk.

### Mitigate INIT-SIPI-SIPI twice exploit

In `xmhf64 c8a04911e`, add a barrier in `xmhf_runtime_shutdown()` before
calling `xmhf_app_handleshutdown()`. This prevents the exploit of having one
CPU running guest and one CPU running malicious code as host.

When testing, can only see `xmhf_runtime_shutdown()` being called when HP 2540p
performs restart.
* HP 2540p init 0: not tested
* HP 2540p init 6: called
* Dell 7050 init 0: not called
* Dell 7050 init 6: not called

Does this completely solve the vulnerability? Maybe.

First, VMXOFF is necessary, as mentioned in comment in
`xmhf_baseplatform_arch_x86vmx_reboot()`.

VMX root mode only blocks INIT, so all pending SIPIs should arrive and be
dropped after all CPUs reach the barrier. So at worst all CPUs halt after
VMXOFF.

However, is it possible that all CPUs halt due to INIT? Maybe not. There should
be at least one CPU alive, which resets the system using keyboard controller.

In `xmhf64 a0b7a9112`, added above to comments.

Ideally, possible ways to fix this issue:
* Do not give the guest full control of sending IPIs
* After the barrier, enter a temporary VMX non-root environment to deplete all
  INIT signals.

### 2. Clean running PALs' memory in `tv_app_handleshutdown()`

In `xmhf64 0230ac339`, update `tv_app_handleshutdown()` to clear PAL memory if
PALs are not unregistered when shutdown.

```
***** VMEXIT_INIT xmhf_runtime_shutdown

***** VMEXIT_INIT xmhf_runtime_shutdown

***** VMEXIT_INIT xmhf_runtime_shutdown

***** VMEXIT_INIT xmhf_runtime_shutdown

TV[2]:scode.c:hpt_scode_destroy_all:1505:          whitelist[0] is still registered when shutdown
xThe system is powered off.
```

However, note that there may still be ways to attack. It is possible for the
guest to reset the machine using hardware, without XMHF calling
`xmhf_runtime_shutdown()`. So DRT should be used to prevent this kind of
attack.

#### Exploit the bug of not performming `tv_app_handleshutdown()`

The experiment is:
* Use HP 2540p
* Boot `xmhf64 9fe3a6e7d` (vesion before fix)
* Boot Linux
* Start `pal_demo` in `lhv-dev 29eb66b88`
* Reboot using Sysrq (Alt + PtrSc + B), using `init 6` may cause OS access PAL.
* Boot `lhv-dev 29eb66b88`, scan memory

We can see that this version of XMHF is vulnerable from below. PAL demo output:
```
Mmap: 1 0x7ff42d184000 1
Mmap: 2 0x7ff42d15a000 1
Mmap: 4 0x7ff42d159000 1
Mmap: 3 0x7ff42d158000 1

With PAL:
 a = 0x84abe15
 g_data = 0x7ff42d15a000
 0x7ff435605e15 = my_pal(0x84abe15, 0x7ff42d15a000)
sleeping ...........
```

LHV output:
```
Available: 0x0000000000000000, 0x000000000009f000
Available: 0x0000000000100000, 0x0000000007f00000
Available: 0x000000000a400000, 0x00000000b07c2000
  0x000000000e9be000: 46484d58 007ff42d 00000000084abe15
Available: 0x00000000bb3ff000, 0x0000000000001000
Available: 0x0000000100000000, 0x0000000038000000
WBINVD complete
```

Explanation
```
  0x000000000e9be000: 46484d58 007ff42d 00000000084abe15
                      F H M X  Mmap     a = 0x84abe15
```

If we boot `xmhf64 0230ac339` instead, then the bug is no longer exploitable.

However, on Dell 7050 and `xmhf64 0230ac339` this bug is still reproducible
because `xmhf_runtime_shutdown()` is not called during reboot.

### 3. TrustVisor Vulnerability 1

TODO: 3. `bug_106` "TrustVisor Vulnerability 1": break memory integrity of guest OS
TODO: 4. `bug_106` "TrustVisor Vulnerability 2": race condition when changing EPTP

