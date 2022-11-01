# Trustvisor in L2

## Scope
* Nested virtualization, all configurations
* `lhv 83723a519`
* `xmhf64 28ce646f2`
* `xmhf64-nest 9ba342c98`
* `xmhf64-nest-dev 186e6588f`

## Behavior
The current design does not support running TrustVisor in L2, because all
VMCALLs from L2 are forwarded to L1. However, when Hyper-V is enabled, Windows
becomes an L2 guest.

Challenges include:
* Find a calling convention for TrustVisor, without comflict with L2 to L1
  interface
* L2 may request a page in EPT12, but is not yet in EPT02
* Implement "remove a page from EPT02", need to also remove the page in EPT01
* Check to make sure TrustVisor does not access VMCS using `vmcs->vcpu`
	* Need to extend functions such as `VCPU_grip()`
* L2 cannot produce any 201 VMEXIT during PAL

## Debugging

TODO: test using LHV

