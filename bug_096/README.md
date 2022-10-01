# Test on Dell Desktop

## Scope
* All configurations
* `xmhf64 427b6d6b7`

## Behavior
Recently received a Dell laptop. Should test XMHF on it.

## Debugging

### Set up AMT

For some reason, setting on Dell is not able to enable SOL. Need to
install MeshCommander on private laptop (that controls Dell). Official website:
<https://www.meshcommander.com/meshcommander>. Can use a Windows machine to
control, or install on Fedora using npm:
<https://www.npmjs.com/package/meshcommander>
```sh
sudo dnf install npm
cd /path/to/meshcommander/install
npm install meshcommander
cd ./node_modules/meshcommander
node meshcommander
# Open http://127.0.0.1:3000/default.htm
```

Add the remote computer, then connect. Can see "Serial-over-LAN" tab. Then can
connect. Maybe need MeshCommander to remotely enable SOL. Also have
"Remote Desktop" to control KVM (keyboard, video, mouse) of Dell, using VNC
protocol but on port 16994.

To use `amtterm` from Linux command line, need to use version 1.7 or above. It
is easy to compile from source. Git is <https://git.kraxel.org/cgit/amtterm/>.

dmesg shows that the serial port for SOL is on I/O port 0xf0a0, so need to
change GRUB accordingly:
`0000:00:16.3: ttyS1 at I/O 0xf0a0 (irq = 20, base_baud = 115200) is a 16550A`

Difference of this configuration compared to HP 2540p is
* `amttool` no longer works, need to use MeshCommander to reset the machine
* `amtterm` will no longer print "The system is powered on." and "The system is
  powered off." So need to chop output file manually.

### Memory problem

With the default settings, will halt because `MAX_PHYS_ADDR` is too small. The
maximum memory should be set to `0x230000000`, or 8.75 GiB. Use
`--mem 0x230000000` in `build.sh`.

The complete command is now
```
./build.sh amd64 --event-logger --mem 0x230000000 && gr
make -j 4 && gr && copyxmhf && hpgrub XMHF-build64 0 && hpinit6
```

### Halt before PCI initialize

Currently, see some error when Linux is booting. However, when using nested
virtualization (git `xmhf64-nest 3ca7e20f5`) and the following compile options,
see error earlier.

```sh
./build.sh amd64 fast O3 circleci --no-x2apic --event-logger --mem 0x230000000
```

The machine halts after printing the following:
```
SL: setup runtime paging structures.
Transferring control to runtime
```

Normally, should see after that:
```
runtime initializing...
xmhf_baseplatform_arch_x86_64_pci_initialize: PCI type-1 access supported.
xmhf_baseplatform_arch_x86_64_pci_initialize: PCI bus enumeration follows:
xmhf_baseplatform_arch_x86_64_pci_initialize: Done with PCI bus enumeration.
```

Try to bisect the configurations. Currently only connecting using SOL (not
connecting remote desktop). DP screen is attached. "Good" below means can see
GRUB the second time
* `xmhf64` branch, `amd64 fast O3 circleci --no-x2apic --event-logger`: not
  tested
* `xmhf64` branch, `amd64 fast O3`: stuck at `Transferring control to runtime`
* `xmhf64` branch, `amd64 O3`: good
* `xmhf64` branch, `amd64`: good
* `xmhf64` branch, `i386 fast`: stuck at `Transferring control to runtime`
	* This one may be wrong
* `xmhf64` branch, `i386`: good
* `xmhf64` branch, `i386 --no-rt-bss`: good
* `xmhf64` branch, `i386 --no-bl-hash`: good
* `xmhf64` branch, `i386 --no-rt-bss --no-bl-hash`: good
* `xmhf64` branch, `i386 fast`: good
* `xmhf64` branch, `amd64 fast`: good
* `xmhf64` branch, `amd64 fast O3`: stuck at `Transferring control to runtime`
* `xmhf64` branch, `i386 fast O3`: good
* `xmhf64` branch, `amd64 O3`: good
* `xmhf64` branch, `amd64 --no-rt-bss O3`: stuck at
  `Transferring control to runtime`
* `xmhf64` branch, `amd64 O3`: good
* `xmhf64` branch, `amd64 --no-rt-bss`: stuck at
  `Transferring control to runtime`
* `xmhf64` branch, `amd64`: good
* `xmhf64` branch, `amd64 --no-rt-bss`: stuck at `SL: RPB, magic=0xf00ddead`
  (earlier than usual)

So the problem should be related to the `--no-rt-bss`, however it is not very
stable.

I am trying to trigger a triple fault using the code snippet below. However,
looks like Dell does not reset at triple fault. I tried it in bootloader and
secure loader. The machine just halts.
```c
	asm volatile (
		"pushl $0;"
		"pushl $0;"
		"lidt (%esp);"
		"ud2;"
	);
```

Maybe the best way to communicate with the outside world is to use the VGA:
```
	/* SL */
	asm volatile ("movq $0xf00b8000, %%rax; 1: incb (%%rax); jmp 1b;":::"%rax");
	/* RT */
	asm volatile ("1: incb 0xb8000; jmp 1b;");
```

Fortunately we are not dealing with DRT at this time, or things will be
complicated.

The VGA trick works. Once the screen responds after the `movq %r8, %cr3`
instruction (switch to runtime paging) in `sl-x86-amd64-sup.S`. However it is
hard to reproduce it (i.e. not stable).

The possible places to trigger the bug are:
* After `SL: RPB, magic=0xf00ddead`
	* `sl.c:149-195`

TODO: flush TLB after `xmhf_setup_sl_paging` in `sl-x86-amd64-sup.S`

TODO: Linux panic

