# Xen

## Scope
* All configurations
* `xmhf64 6bac49570`

## Behavior
We should try running Xen, make sure XMHF supports it.

## Debugging

Resources
* <https://wiki.debian.org/Xen> (use web.archive.org)
* <https://serverfault.com/questions/511923/>

Use `sudo xl list -l` to determine whether Xen is PV or HVM.

Looks like dom0 is cannot be configured to be HVM. DomU can be HVM or PV.

Currently not going to use Xen. XMHF paper is configuring domU guests.

## Result

Currently not going to try Xen.

