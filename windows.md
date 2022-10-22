# Running XMHF and Windows on QEMU

Download Windows 10 installer from Microsoft website.

Install Windows using:
```
qemu-img create -f qcow2 win10x64.qcow2 32G
qemu-system-x86_64 -m 1G \
	--drive media=cdrom,file=Win10_21H2_English_x64.iso,index=1 \
	--drive media=disk,file=win10x64.qcow2,index=2 \
	--enable-kvm \
	-smp 2 -cpu Haswell
qemu-img create -f qcow2 -b win10x64.qcow2 -F qcow2 win10x64-a.qcow2
```

Then use QEMU to load two disks, one for Linux and one for Windows:
```
qemu-system-x86_64 -m 1G \
	--drive media=disk,file=/home/lxy/Documents/GreenBox/tmp/qemu/c4.qcow2,index=1 \
	--drive media=disk,file=win10x64.qcow2,index=2 \
	--enable-kvm \
	-smp 2 -cpu Haswell,vmx=yes -serial stdio
```

Boot into Linux, and run `sudo grub-update`, should detect Windows at
`/dev/sdb1`. Then in GRUB can boot Windows.

A problem is that GRUB will try to search disk / partition with a (likely)
UUID. If we change `/dev/sdb` into another Windows installation, will see
a warning. The solution is to remove the search.

Can write a custom GRUB menu entry at `/etc/grub.d/40_windows`, then disable
`/etc/grub.d/30_os-prober`
```
#!/bin/sh
exec tail -n +3 $0
menuentry 'Windows' --class windows --class os $menuentry_id_option 'windows' {
	insmod part_msdos
	insmod ntfs
	set root='hd1,msdos1'
	parttool ${root} hidden-
	drivemap -s (hd0) ${root}
	chainloader +1
}
```

In `/etc/default/grub`, Can change to `GRUB_DEFAULT=Windows`

## Debugging Windows

### WinDbg

Using tutorial from <https://www.cnblogs.com/csnd/p/11800535.html>
Downloaded zip file "Windbg XP 安装.zip"

Install `dbg_x86.msi`

Install `WindowsXP*symbols-full-ENU.exe` (to `C:\WINDOWS\Symbols`)

May need to copy `C:\WINDOWS\Symbols\*\*.pdb` to `C:\WINDOWS\Symbols\`

Or following the blog's way, use symbol server and manually copy files to
cache directory. Maybe can calculate cache directory in Linux (e.g.
<https://stackoverflow.com/questions/1419347/>)

In WinDbg, select `File -> Kernel debug`, debug local kernel (for convenience)

```
!sym noisy
.sympath C:\WINDOWS\Symbols
.reload
```

Want to set break point, need to debug remote machine.

Refs
* <https://stackoverflow.com/questions/12513205/remote-debugging-using-windbg>
* <https://web.archive.org/web/20130605113813/http://blogs.msdn.com/b/ntdebugging/archive/2009/02/09/remote-debugging-connecting-to-a-remote-stub-using-the-microsoft-debugging-tools-for-windows.aspx>
* <https://docs.microsoft.com/en-us/windows-hardware/drivers/debugger/process-server-examples>

In kernel debug, if select COM / serial port, on QEMU can see it writes
to serial port a lot of "i"s. This should be run on the debugger.

Need to run `dbgsrv.exe` on the debugged machine. This is installed with windbg.

In cmd, syntax should be: `kdsrv.exe -t com:com1,baud=115200`

#### Serial port

In Linux, serial port is `/dev/ttyS{0..3}`. In Windows, is `COM{1..4}`.

In QEMU, can specify multiple `-serial` arguments. For example,
`-serial file:/tmp/a -serial stdio` means `/dev/ttyS0` is pipe and `/dev/ttyS1`
is stdio.

QEMU's `-serial file:` does not allow input, so we can use UDP. Should be able
to connect two computers together using `-serial udp::1152@:5211` on one
machine and `-serial udp::5211@:1152` on another. Or use TCP (as in tutorials):
`-serial tcp::11520,server,nowait` (debugger) and `-serial tcp:localhost:11520`
(debugee)

In extreme case, can use Wireshark to packet capture communication.

Refs:
* <https://resources.infosecinstitute.com/topic/kernel-debugging-qemu-windbg/>
* <http://mangoprojects.info/virtualization/virtual-serial-connection-between-two-qemu-vm/>
* <https://stackoverflow.com/questions/30049300/windbg-first-connect-then-stuck-on-debuggee-not-connected-message-during-kern>
* <https://docs.microsoft.com/en-us/windows-hardware/drivers/debugger/getting-started-with-windbg--kernel-mode->

First start debugger, and WinDbg, then start debugee.

Will see some information on WinDbg, and "Debugee not connected"

If hit "Debug -> Break", can stop the debugee and enter `kd` commands

## Windows 10

In Windows 10, Resource Manager, the Memory section can show whether XMHF is
running. Under "Physical Memory" there are "Available, Cached, Total,
Installed". Installed will always be actual physical memory. For example, when
QEMU has `-m 2G`, Installed will be 2048 MB. When XMHF is not running, Total is
2047 MB. When x86 XMHF is running, Total is 1893 MB. For x84 XMHF it is 1827
MB.

### Shutdown

Ref:
<https://superuser.com/questions/968672/shortcut-to-shutdown-or-restart-windows-10-or-11-in-less-than-three-keystrokes>
* Use QEMU's power button
* In desktop (shown using `Windows + D`), `Alt + F4`, then `Enter`
* `Windows + X`, `U`, `U`

### Remote desktop

Can enable remote desktop in settings. Can use Remmina to connect (for some
reason rdesktop does not work). Use the RDP protocol to prevent checking
certificates.

### Running Linux programs in Windows

* WSL: <https://docs.microsoft.com/en-us/windows/wsl/install>
* Cygwin?: <https://www.cygwin.com/>
	* May have trouble because Cygwin does not use Linux's calling convention

### Faking time in QEMU

Sometimes Windows XP asks the user to activate Windows. Maybe a workaround is
to time travel to when the OS was just installed. In QEMU KVM this is
done by `-rtc base=2011-11-11T11:11:00,clock=rt`. For some reason Debian
still sees host's date, but Windows XP x64 will see the fake date.

Ref: <https://unix.stackexchange.com/questions/345483/>

### Windows 10 Disable Update

Ref: <https://www.cleverfiles.com/howto/disable-update-windows-10.html>

Run `services.msc`, "Windows Update", "Properties", "Startup type" set to
"Disabled".

Or maybe just cut the Internet.

