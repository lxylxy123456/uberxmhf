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

### VirtualBox Debian Installer Problem

After trying to boot the Debian netinst image, see Linux kernel panic

TODO

