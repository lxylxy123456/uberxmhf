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
another bug. See `bug_064`.

Note: `test2.py` no longer maintained in notes. From now on maintained in
xmhf64 source code, in `.jenkins/test2.py`.

### Jenkins Setup

Sha512 of the QCOW2 files are
```
ebab48dd69c587e268ecfbf9a0897a661490c771d10cb2ee6d14191532301d6e90063db1e1339d6cba917df5a5e87ccf48709d776c87244f1fda9c79b0c4d4af  debian11x64.qcow2
1f5c76e1638fd1d64019a3f68cb95914ee5ec4ae5742289fb69bcedd474e1901ab23b1e51ff5e81bdcef33aff432100c144fa9fae7a98921008701d88270e6c0  debian11x86.qcow2
```

Consider prefetch / pre-populate cache files in
`/var/lib/jenkins/workspace/xmhf/cache`

Hook
* In <http://127.0.0.1:8080/job/xmhf/configure>, allow "触发远程构建"
* In <http://127.0.0.1:8080/user/lxy/configure>, create API token

Shell script for Hook
```sh
#!/bin/bash
set -e
SCRIPT_DIR="$(realpath "$(dirname $BASH_SOURCE)")"
cd "$SCRIPT_DIR"

XMHF_BRANCH=xmhf64
if [ "$#" -gt 0 ]; then
	XMHF_BRANCH="$1"
fi
TOKEN=<REDACTED>
USR=lxy:<REDACTED>
URL="http://127.0.0.1:8080/job/xmhf/buildWithParameters?token=$TOKEN"
curl --user "$USR" "${URL}&XMHF_BRANCH=${XMHF_BRANCH}"
echo Build requested
```

## Fix

`3bd65eec4..05943b8e0`
* Support running Jenkins for CI
* Support running Circle CI for CI

