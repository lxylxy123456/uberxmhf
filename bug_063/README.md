# Construct automated testing for XMHF

## Scope
* All XMHF
* For now QEMU only
* Git `3bd65eec4`

## Behavior
We need a way to automatically test a compiled version of XMHF. Running the
simple `./test_args{32,64} 7 7 7` test should be enough.

## Debugging
`bug_017/test.py` is a good starting point. We need to generalize it.

