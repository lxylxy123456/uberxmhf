# Windows 11 DRT blue screen

## Scope
* Windows 11 UEFI
* Dell 7050
* `xmhf64 64db0f61f`
* `xmhf64-dev 9d905dc3a`
* DRT
* Does not matter: DMAP

## Behavior

This bug is first found in `bug_071`. See section `### DMAP`.

After Windows 11 boots for some time, see blue screen. Reason
`KMODE_EXCEPTION_NOT_HANDLED`. This bug only happens when DRT enabled.

## Debugging

By modifying exception bitmap, can intercept all exceptions in guest mode.

Git `xmhf64-dev 6446f3e7e`, serial `20230508212546`. Looks like the blue screen
is caused by `#DE`. First there are 125 `#GP(0)` exceptions (0x80000b0d), with
period of 25. Then there is a `#DE` exception (0x80000300).

Git `xmhf64-dev d865531fe`, serial `20230509014612`, print the page of code
that cause `#DE`.

TODO: reverse engineer code causing `#DE`
TODO: make sure no DRT does not have `#DE`
TODO: try to continue exeucting without injecting `#DE`, see whether workaround

