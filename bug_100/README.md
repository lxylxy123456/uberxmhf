# Possible deadlock in Bootloader while performing DRT

## Scope
* Dell 7050 only
* Reproducible when DRT, not sure about no DRT
* `xmhf64 f8029f6e8`

## Behavior
Sometimes, XMHF stucks at bootloader after printing the second line of
"Sending INIT IPI to all APs...". There should be 3 APs printing
"Waiting for DRTM establishment...", but only 2 of them printed this message.

Normally the log should look like:
```
Sending INIT IPI to all APs...Done.
Sending SIPI-0...Done.
Sending SIPI-1...AP(0x06): Waiting for DRTM establishment...
AP(0x04): Waiting for DRTM establishment...
Done.
APs should be awake!
AP(0x02): Waiting for DRTM establishment...
BSP(0x00): Rallying APs...
BSP(0x00): APs ready, doing DRTM...
LAPIC base and status=0xfee00900
Sending INIT IPI to all APs...
```

## Debugging

The intended control flow is:
1. BSP enters `cstartup()`
2. BSP calls `wakeupAPs()`
3. BSP and APs call `mp_cstartup()`
4. Barrier to let BSP wait for AP
5. APs print "Waiting for DRTM establishment..."
6. BSP calls `do_drtm()`
7. BSP calls `send_init_ipi_to_all_APs()`, send INIT to APs

Now it is hard to reproduce this bug, but clearly there is a deadlock:
* BSP and AP reach 4
* BSP reaches 6
* AP grabs the printf lock in step 5
* BSP reaches 7, send INIT to AP
* The next time BSP calls `printf()`, there is a deadlock

Fixed in `xmhf64 f8029f6e8..153a61f99`

## Fix

`xmhf64 f8029f6e8..153a61f99`
* Fix deadlock in bootloader in `mp_cstartup()`

