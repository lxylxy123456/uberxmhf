# The Mysterious Exception 0x27 in LHV

## Scope
* Dell
* Bochs
* All subarchs
* `lhv ce69c270c`
* `lhv-nmi fa93291f4`

## Behavior
When running LHV or lhv-nmi in Dell, see "The Mysterious Exception 0x27". Even
after handling this exception, the exception still appears.

This problem is first observed when running lhv-nmi in Bochs, see `bug_090`.

## Debugging

### EPT exit qualification

When running LHV on Dell, see a new assertion error. The usual qualification is
0x181, but now it becomes 0x581. This can be explained by reading Intel v3
"Table 26-7. Exit Qualification for EPT Violations". The additional bits are
set due to support to "advanced VM-exit information for EPT violations". I
changed LHV in `lhv de25814dc` to ignore these bits.

### The Mysterious Exception 0x27

TODO: run pebbels kernel, see what happens
TODO: review tboot `calibrate_tsc()`
TODO: see `bug_090`

TODO
TODO: be able to run LHV in Dell
TODO: be able to run lhv-nmi in Dell

