# Test other hypervisors

## Scope
* All configurations, focus on amd64
* `xmhf64 3875e68b1`
* `xmhf64-nest 35ef22bcd`
* `lhv 49258660f`

## Behavior
Currently only KVM has run in XMHF as a hypervisor. Should try other
hypervisors.

## Debugging

### Setting up VirtualBox
Following <https://wiki.debian.org/VirtualBox> to install VirtualBox on Debian.

```sh
sudo apt install fasttrack-archive-keyring
sudo vi /etc/apt/sources.list
	deb https://fasttrack.debian.net/debian-fasttrack/ bullseye-fasttrack main contrib
	deb https://fasttrack.debian.net/debian-fasttrack/ bullseye-backports-staging main contrib
sudo apt-get update
sudo apt-get install virtualbox
# sudo apt install virtualbox-ext-pack
# restart
```

See the following error. Giving up and trying another way.

```
$ VBoxManage list vms
WARNING: The character device /dev/vboxdrv does not exist.
	 Please install the virtualbox-dkms package and the appropriate
	 headers, most likely linux-headers-amd64.

	 You will not be able to start VMs until this problem is fixed.
$ 
```

Removed virtualbox from previous installation, follow
<https://www.virtualbox.org/wiki/Linux_Downloads> now.

```sh
sudo vi /etc/apt/sources.list
	deb [arch=amd64 signed-by=/usr/share/keyrings/oracle-virtualbox-2016.gpg] https://download.virtualbox.org/virtualbox/debian bullseye contrib
wget -O- https://www.virtualbox.org/download/oracle_vbox_2016.asc | sudo gpg --dearmor --yes --output /usr/share/keyrings/oracle-virtualbox-2016.gpg
sudo apt-get update
sudo apt-get install virtualbox-6.1
sudo /sbin/vboxconfig
	# May be asked to install linux-headers-amd64 linux-headers-5.10.0-10-amd64
	# Need to update kernel version
VBoxManage list vms
	# Should not see error
```

### VirtualBox amd64 Debian kill init Problem

After trying to boot the Debian netinst image, see Linux kernel panic. The same
happens when trying to boot debian11x64 (QCOW2 image installed using KVM). The
panic message is "not syncing: Attempted to kill init! exit code=0x0000000b".

From the kernel panic message, looks like the init process received #GP
exception, which causes `do_exit()` to be called. This triggers kernel panic
because init cannot exit.

The RIP of init process does not look stable. So I guess the #GP may be caused
by an interrupt. Otherwise, by looking at Linux panic message, looks like most
instructions are accessing memory. Maybe there is something wrong with page
table / EPT?

The good news is that VirtualBox is open source. So ideally there is no
blackbox during debugging.

### VirtualBox i386 Debian vmentry failure Problem

When trying to boot debian11x86, another problem happens in XMHF:
"Debug: guest hypervisor VM-entry failure."

This problem is also reproducible when trying to run 15605 p3 kernel in
virtualbox.

Now try to make LHV reproduce this problem
* `lhv-dev fa7d55af6`: LHV do not enter guest mode, not reproducible. However
  timer interrupts (APIC and PIT), keyboard interrupts, and console are good.
* `lhv-dev fdac0ce2b`: LHV enter host user mode, wait forever, timer interrupts
  work, not reproducible.

Now focus on p3 kernel. The VMENTRY failure log is in `entry_fail1.txt`. A
normal VMCS (during BIOS?) look like `sample_vmcs2.txt`. Compare using:
```sh
diff <(cut -d @ -f 1,3 sample_vmcs2.txt | cut -d : -f 2) \
	<(grep failure02 entry_fail1.txt | cut -d @ -f 1,3 | cut -d : -f 2)
```

Note that `info_vmexit_reason = 0x80000022`, which is "34VM-entry failure due
to MSR loading.". This is not the usual "VM-entry failure due to invalid guest
state." Now we need to print MSR data structures.

Git `xmhf64-nest-dev 028a7fecc`, serial `20220919235746`. We have printed all
VMEXITs and some MSRs. From `0x0010a7a0`, the control is transferred to Pebbles
kernel. Can see that `0x00100c65` are EPT misconfig VMEXITs in console driver.
`0x0010cccc` are IO VMEXITs in `outb()`.

Then the following VMEXITs are due to CRx access. The first is CR3, second is
CR0. Looks like VirtualBox emulated the second set CR0 operation, and the
VMENTRY caused the entry failure.
```
CPU(0x05): nested vmexit  0x0010cd17 28
CPU(0x05): nested vmexit  0x0010ccfa 28
```

The relevant SDM chapter is Intel i3 "25.4 LOADING MSRS". This footnote looks
interesting:
> 1. If CR0.PG = 1, WRMSR to the IA32_EFER MSR causes a general-protection
> exception if it would modify the LME bit. If VM entry has
> established CR0.PG = 1, the IA32_EFER MSR should not be included in the
> VM-entry MSR-load area for the purpose of modifying the
> LME bit.

Current directions are
* Print state before and after `0x0010ccfa`, print more MSRs
* Print KVM behavior at `0x0010ccfa` and compare

TODO: double check that KVM does not have the Pebbles kernel problem
TODO: fix i386 vmentry failure
TODO: does LHV have the same problem?
TODO: fix amd64 kill init problem

