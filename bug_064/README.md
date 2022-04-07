# KVM nested virtualization booting Debian on XMHF stuck / slow

## Scope
* All subarch
* QEMU on nested KVM only
* Git `05943b8e0`

## Behavior
This bug is discovered in `bug_063`. When XMHF runs on two levels of KVM,
things become very slow, and Debian 11 fails to boot in a few minutes.
* Good
	* Intel CPU
		* KVM (QEMU, Host is Fedora)
			* XMHF
				* Debian 11
* Bad
	* Intel CPU
		* KVM (QEMU, Host is Fedora)
			* KVM (QEMU, Host is Debian 11)
				* XMHF
					* Debian 11

## Debugging
The guess is that nested virtualization is too slow. However we need to confirm
it.

First add some printing in Debian. Git `7930f296f`, see count for all types of
VMEXITs
```
      3 ,0}
   3469 ,10}	cpuid
      1 ,28}
     12 ,31}	rdmsr
   8016 ,32}	wrmsr
```

We now print more information about CPUID and WRMSR. Git `ff6455437`.
Can see that almost all problematic VMEXITS are related to WRMSR 0x6e0, which
is `IA32_TSC_DEADLINE`. So to fix the problem, we need to make WRMSR and CPUID
very fast.

In git `60081b12a`, optimized CPUID and WRMSR.ecx=0x6e0. Can feel that things
become faster.

Implemented `36879f183` in xmhf64 branch.

### Testing with nested virtualization

We use curlftpfs to prevent copying QCOW2 files around. In Debian, install
dependencies
```
sudo apt-get install curlftpfs qemu-system-x86
sudo usermod -aG kvm lxy
# Another workaround: sudo chmod 666 /dev/kvm
```

In host, host files in PWD using FTP
```
pip3 install pyftpdlib
python3 -m pyftpdlib -u lxy -P PASSWORD -i IP -p PORT
```

In Debian, use curlftpfs
```
mkdir mnt
curlftpfs HOST:PORT mnt -o user=USER:PASS
```

QEMU can read backing image using FTP, but cannot write to FTP images.

In GRUB, remove `quiet` when booting Debian shows the progress of Debian
booting.

Test result is that: when XMHF's SMP = 1, can boot. However, when SMP = 2, see
strange errors: `{0,48}{0,0}` (EPT violation and exception)

We likely need to try some QEMU + GDB debugging.

### LAPIC

Can see that `{0,48}{0,0}` are due to write to LAPIC registers.
The first exception happens when `RIP = 0xffffffffa6c62a52`.
The second exception happens when `RIP = 0xffffffffa6c62a58`.

GDB snippet for performing page walk in long mode
```gdb




set $a = vcpu->vmcs.guest_RIP
set $c = vcpu->vmcs.guest_CR3
set $p0 = ($c           ) & ~0xfff | ((($a >> 39) & 0x1ff) << 3)
set $p1 = (*(long*)($p0)) & ~0xfff | ((($a >> 30) & 0x1ff) << 3)
set $p2 = (*(long*)($p1)) & ~0xfff | ((($a >> 21) & 0x1ff) << 3)
set $p3 = (*(long*)($p2)) & ~0xfff | ((($a >> 12) & 0x1ff) << 3)
set $p4 = (*(long*)($p3)) & ~0xfff
set $b = $p4 | ($a & 0xfff)
```

(gdb) p/x ((($a >> 39) & 0x1ff) << 3)
$46 = 0xff8
(gdb) p/x ((($a >> 30) & 0x1ff) << 3)
$47 = 0xff0
(gdb) p/x ((($a >> 21) & 0x1ff) << 3)
$48 = 0x9b0
(gdb) p/x ((($a >> 12) & 0x1ff) << 3)
$49 = 0x310
(gdb) 

