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

Then after some development,
* QEMU images are hosted on Google Drive, can download using `gdown`
	* Jenkins preserves workspace, so cache in workspace
	* Circle CI supports caching
* Built self-hosted Jenkins CI, works well
* Built Circle CI, but test fails. Looks like nested virtualization has problem

Good
* Intel CPU
	* KVM (QEMU, Host is Fedora)
		* XMHF
			* Debian 11

Bad
* Intel CPU
	* KVM (QEMU, Host is Fedora)
		* KVM (QEMU, Host is Debian 11)
			* XMHF
				* Debian 11

In the bad case, Debian 11 fails to start. This is probably a problem worth
another bug.

Note: `test2.py` no longer maintained in notes. From now on maintained in
xmhf64 source code, in `.jenkins/test2.py`.

## Fix

`3bd65eec4..`
* Support running Jenkins for CI
* Support running Circle CI for CI

