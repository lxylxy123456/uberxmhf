# Dell cannot run DRT

## Scope
* DRT enabled
* i386 and amd64
* `xmhf64 ac3f3a510`

## Behavior
When trying to run XMHF on Dell with DRT, the machine automatically resets
after seeing `GETSEC[SENTER]` in serial port. Log for amd64 (nested
virtualization branch) looks like `20221003220343`. Log for i386 looks like
`20221003221201`.

## Debugging

TODO: look at TXT.ERRORCODE, see `bug_046`
TODO: see xlsx file in `6th_7th_gen_i5_i7-SINIT_79.zip`

