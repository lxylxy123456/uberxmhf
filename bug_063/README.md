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

Wrote `test.py` using qemu-nbd to copy

Then can write Jenkins file, for example `Jenkinsfile` (some details removed)

Looks like Circle CI also supports VMX, but need to investigate further.
<https://discuss.circleci.com/t/new-android-machine-image-now-available-to-all-users/39467>

Then realized that qemu-nbd is not elegant, so wrote `test2.py`. This one uses
scp.

## Result
Built self-hosted Jenkins CI

