# CI for Windows

## Scope
* QEMU
* All subarchs
* Windows, but can also apply to Linux
* Git `4ed89c9c8`

## Behavior
We are manually testing less on Windows, but it will be helpful to test Windows
on CI.

## Debugging

We can use the test `TV_HC_TEST` VMCALL to report results. Then we write a bat
or C script to perform all tests and print the result on the screen. As long as
we can automatically login and run the tests, we do not need to dynamically
control Windows.

We should also be able to compile PAL demo and load it to Windows using a FAT
32 file system. There should be Linux tools to perform this task, like mtools
* Ref: <https://unix.stackexchange.com/questions/629492/>

We will perform most of the development in `xmhf64-dev` branch (starting at
`8630c2519`). Then we will squash to `xmhf64` branch. The Linux testing code
will remain the same (using SSH), so we will copy new files.

The benefit of testing without SSH is that it is more stable (e.g. no need to
test whether guest is up using SSH). It also does not require network access of
the guest at all (good for reproducibility?) The down side is that in theory
the tested code can cheat by printing the answers to the serial port.

### Windows configurations

* Automatically login
	* Windows 7
		* Windows+R, netplwiz, uncheck "Users must enter a user name and
		  password to use this computer"
		* <https://windowsreport.com/windows-7-auto-login/>
	* Windows 10
		* `reg ADD "HKLM\SOFTWARE\Microsoft\Windows NT\CurrentVersion\PasswordLess\Device" /v DevicePasswordLessBuildVersion /t REG_DWORD /d 0 /f`
			* Ref: <https://www.askvg.com/fix-users-must-enter-a-user-name-and-password-to-use-this-computer-checkbox-missing-in-windows-10/>
		* Then same as Windows 7
* Automatically start program
	* Windows 7
		* "C:\ProgramData\Microsoft\Windows\Start Menu\Programs\Startup"
		* <https://answers.microsoft.com/en-us/windows/forum/all/how-to-start-program-before-user-logon-windows-7/2bff97c4-c037-437c-9fa7-b143a3ae5189>
* Installation time
	* Windows XP:  2022-01-09
	* Windows 7:   2022-03-21
	* Windows 8.1: 2022-03-21
	* Windows 10:  2022-01-09
	* Change time using `-rtc base=2011-11-11T11:11:00,clock=rt`
* Activating
	* Windows 7
		* Requires some clicks to skip activating
	* Windows 10
		* Requires some clicks to skip setting up things
* Mounting
	* Windows 7
		* `C:` is Windows, `F:` is disk containing `pal_demo`
	* Windows 10
		* `C:` is Windows, `F:` is disk containing `pal_demo`

However, it looks like not easy to fake time for modern OS (e.g. Windows 10).
So let's try starting XMHF before user login. Hopefully the interface will not
stuck at activate Windows etc.

Looks like we can use task scheduler. Schedule a task to run at computer
startup, with or without user logged in. This method succeeds in Windows 7.

* Windows 7 complete instructions
	1. Windows + R, `taskschd.msc`
	2. New folder in Task Scheduler Library
	3. In new folder, Action -> Create Task
		* Run whether user is logged in or not
		* Run with highest privileges
		* Triggers: At startup
		* Actions: Start a program, `F:\win7x86.bat`
* Windows 8.1: working
* Windows 10: working (if only 512M memory, not working)

To debug, we can copy files from Jenkins directory

```
# cd to xmhf64
mkdir -p tmp/grub
cp /var/lib/jenkins/workspace/xmhf64-windows/tmp/pal_demo.img tmp/
cp /var/lib/jenkins/workspace/xmhf64-windows/tmp/grub/c.img tmp/grub
# /var/lib/jenkins/workspace/xmhf64-windows/tools/ci/windows/grub_windows.img
# /var/lib/jenkins/workspace/xmhf64-windows/tools/ci/windows/bios.bin
python3 -u ./tools/ci/test4.py \
	--guest-subarch amd64 \
	--qemu-image /var/lib/jenkins/workspace/xmhf64-windows/cache/win10x64-j.qcow2 \
	--work-dir tmp/ \
	--no-display --verbose --watch-serial

```

However, before we have a chance to debug, we realized that providing more
memory likely solves the problem.

### Time statistics

* i386 XMHF
	* i386 Windows 7:    16  16  16
	* i386 Windows 10:   58  56  56
* amd64 XMHF
	* i386 Windows 7:    16  17  19
	* amd64 Windows 7:   48  51  51
	* amd64 Windows 8.1: 116 112 74
	* i386 Windows 10:   59  57  62
	* amd64 Windows 10:  216 251 236

### Windows XP

When setting up for Windows XP SP3, see error
> The new task has been created, but may not run because the account
> information could not be set. The specific error is: 0x80070005: Access is
> denied.

We are not going to solve this for now.

### Squashing changes

* `xmhf64-dev` before changes: `8630c2519`
* `xmhf64-dev` after changes: `9781c8c67`
* Display changes to be squashed: `git diff cb871f15f..9781c8c67`
* `xmhf64` commit about the changes: `cb871f15f..c447283b9`

## Fix

`4ed89c9c8..c447283b9`
* Update CI docs
* Make `pal_demo` compilable on Fedora
* Write Jenkins CI for Windows 7 and higher

