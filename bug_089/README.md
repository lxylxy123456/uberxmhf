# Update CI for KVM XMHF XMHF Debian

## Scope
* All configurations
* `xmhf64 8968e4c63`
* `xmhf64-nest 66b2ad27e`
* `lhv e1f091f54`

## Behavior
After `bug_088`, we should update CI to automatically test the
`KVM XMHF XMHF Debian` configuration.

## Debugging

First develop POC in `xmhf64-nest-dev 839c01af9..b7e2e8b2a`. Now we realize
that the testing code should be formalized.

Also use <https://stackoverflow.com/questions/37800195/> to extract common
logic to separate Groovy file.

## Fix

`xmhf64 8968e4c63..35b8a931f`
* Update CI

`xmhf64-nest 66b2ad27e..536d95d87`
* Update CI

`lhv e1f091f54..f8111a463`
* Update CI

