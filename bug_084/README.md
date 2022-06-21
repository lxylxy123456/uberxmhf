# Support guest hypervisor setting EPT

## Scope
* All subarchs
* Git branch `xmhf64-nest` commit `acad9ef02`
* Git branch `lhv` commit `d6eea8116`

## Behavior
Currently for the guest hypervisor running in XMHF, EPT is not supported.
This need to be changed in order to run unrestricted nested guests.

## Debugging

First use EPT in lhv.

TODO: try to remove bss from XMHF's compressed image, at least in build option
TODO

