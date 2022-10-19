# Dell cannot boot Windows 7

## Scope
* Dell 7050 only
* Windows 7 only (x86 or x64)
* `xmhf64 5185f1e6a`

## Behavior
While running Dell, XMHF, Windows 7, see Windows 7 blue screen. XMHF
configuration: `./build.sh amd64 --mem 0x230000000 && gr`.

Possible related serial output:
```
CPU(0x04): NMI received (MP guest shutdown?)
CPU(0x02): NMI received (MP guest shutdown?)
TV[0]:appmain.c:tv_app_handleshutdown:727:         CPU(0x04): Shutdown intercept!
TV[0]:appmain.c:tv_app_handleshutdown:727:         CPU(0x02): Shutdown intercept!
***** VMEXIT_INIT xmhf_runtime_shutdown

CPU(0x06): NMI received (MP guest shutdown?)
TV[0]:appmain.c:tv_app_handleshutdown:727:         CPU(0x00): Shutdown intercept!
TV[0]:appmain.c:tv_app_handleshutdown:727:         CPU(0x06): Shutdown intercept!
```

If XMHF uses DRT, the machine resets with TXT.ERRORCODE 0x8000000a.

## Debugging

Looks like without XMHF, Windows 7 also has trouble starting.

In BIOS, SATA Operation can be
* Disabled
* AHCI
* RAID On (selected)

If switch from "RAID On" to "Disabled", then GRUB cannot boot.
If switch from "RAID On" to "AHCI", then Debian 11 x64 can boot.

Looks like using `lspci` in Linux can show the SATA mode. The HP 2540p that
installed the Windows 7 uses IDE, but Dell uses RAID.
* Ref: <https://superuser.com/questions/607104/>

Tending to give up at this point, since Windows 7 is considered old OS now.
Ideally, we need to re-install Windows 7 to fix this problem.

## Results

Problem is not on XMHF. Not going to fix.

