# Support guest hypervisor setting MSR and EPT

## Scope
* All subarchs
* Git branch `xmhf64-nest` commit `d3c97a24b`
* Git branch `lhv` commit `d90b70785`

## Behavior
Currently for the guest hypervisor running in XMHF, EPT is not supported.
MSR load / store count must be 0. These need to be changed in order to run
unrestricted nested guests.

## Debugging

### Support MSR load / store

In LHV `d90b70785..fb6b251d6`, added code to load MSR and check whether the
add is successful. Need to build with `--lhv-opt 1`.

Note: MSR load / store area can be larger than 1 page. Be careful.

