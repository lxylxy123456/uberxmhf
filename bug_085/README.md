# Running XMHF in XMHF

## Scope
* All subarchs
* Git branch `xmhf64-nest` commit `eecb1ffce`
* Git branch `lhv` commit `6a8866c1e`

## Behavior
After `bug_084`, I think it should be possible to run XMHF in XMHF. So try it.

## Debugging

### Improving grub.py

`grub.py` needs to be updated for XMHF in XMHF to work. For example, currently
XMHF only boots correctly when it is the first disk. We also add some features,
like setting timeout (to debug GRUB).

In `xmhf64 3b8f57996..307d583ed`, rewrite most part of `grub.py` to implement
the above.

`__TARGET_BASE_SL`

TODO: grub.py windows cannot minimize modules.

TODO: try running XMHF in XMHF (create a new bug)

