# Branches in <https://github.com/lxylxy123456/uberxmhf>

* `master`: Forked from <https://github.com/uberspark/uberxmhf>
	* `lxy-compile`: Fix compile errors on Debian 11
* `notes`: Notes when studying XMHF; does not contain code
* `xmhf-latest`: XMHF code of `lxy-compile`, but v0.2.2 directory structure
	* `xmhf64`: Main development branch
		* <del>`xmhf64-no-drt` (ea33d732f): Make trustvisor runnable with
		  `--disable-drt`</del>
		* <del>`xmhf64-comp` (62c6eb8c6): Make trustvisor compilable in 64
		  bits</del>
		* <del>`xmhf64-long`: set up long mode in secureloader</del>
		* <del>`xmhf64-acpi`: get into x64 runtime</del>
		* <del>`xmhf64-ap`: be able to spawn APs in xmhf-runtime</del>
		* <del>`xmhf64-vm`: deal with VMWRITE and VMLAUNCH failed problem</del>
		* (no longer use sub-branches)
		* `xmhf64-dev`: development commits

## Change Log

### `xmhf64-no-drt`
`bc29c25e4..ea33d732f`
* Disable call to `trustvisor_master_crypto_init()`
* Disable call to `utpm_extend()`

### `xmhf64-comp`
`0f069fa78..62c6eb8c6`
* Add `__X86__`, `__X86_64__`, `__XMHF_X86__`, and `__XMHF_X86_64__` and macros
* Port `xmhf/src/libbaremetal/libxmhfc/include/sys/ia64_*.h`, similar to `i386*`
* When casting to/from pointers and uints, change from uint32 to `uintptr_t`
  or equivalent
* Remove assembly instructions that do not compile (e.g. `pusha`)
* Remove some function definitions when `__DRT__` is not defined
* Add uint definitions for pointers: `hva_t`, `spa_t`, `gva_t`, `gpa_t`, `sla_t`
* Modify Makefile to compile bootloader in x86, others in x86_64
* Compile some object files twice to support x86 bootloader

### `xmhf64-long`
`7e142744b..7bf294603`
* Transition from protected mode to long mode in beginning of secure loader
	* <https://www.gnu.org/software/grub/manual/multiboot/multiboot.html>
	* <https://wiki.osdev.org/Setting_Up_Long_Mode>
	* <https://0xax.gitbooks.io/linux-insides/content/Booting/linux-bootstrap-4.html>
	* <https://wiki.osdev.org/Creating_a_64-bit_kernel#Loading>
	* Intel v3 9.8.5 Initializing IA-32e Mode
	* Paging: Intel v3 chapter 4
	* MSR (cr0, cr3, cr4): Intel v3 2.5 CONTROL REGISTERS
	* MSR (EFER): Intel v3 2.2.1 Extended Feature Enable Register
	* GDT entries: Intel v3 3.4.5 Segment Descriptors
	* In x64 segment base=0: Intel v1 3.4.2.1 Segment Registers in 64-Bit Mode
* x86 use GDT to create VA to PA offset -> x86_64 use page table
* Implement spinlock in x64
* For SLPB (passing parameter from x86 bootloader to x86_64 secure loader), use
  32-bit data types, and cast in secure loader
* Fix stdarg in x64 (so that printf can work correctly)

### `xmhf64-acpi`
`7bf294603..4e1ddca98`
* Support accessing low physical memory (e.g. 0x40e)
	* e.g. ACPI problem in x64: `xmhf_baseplatform_arch_x86_acpi_getRSDP`
	* Solution: map these memory at high end of the page table
* Change run time GDT TSS to 64-bits in `xmhf_sl_arch_xfer_control_to_runtime()`
	* TSS ref: Intel v3 7.2.3 TSS Descriptor in 64-bit mode
* 4-level identity paging in `xmhf_sl_arch_x86_64_setup_runtime_paging()`
* Port exception handler (IDT) to 64 bits
	* Intel v3 6.3 SOURCES OF INTERRUPTS
	* Intel v3 6.10 INTERRUPT DESCRIPTOR TABLE (IDT)
	* Intel v3 6.14 EXCEPTION AND INTERRUPT HANDLING IN 64-BIT MODE
	* Intel v3 6.12.1 Exception- or Interrupt-Handler Procedures (stack content)
	* Find out CPU's LAPIC ID: <https://wiki.osdev.org/APIC>
* Other Reference
	* E820: <https://wiki.osdev.org/Detecting_Memory_(x86)>, or ACPI standard
* EPT: similar to 4-level paging, but some fields differ
	* Intel v3 27.2 THE EXTENDED PAGE TABLE MECHANISM (EPT)

### `xmhf64-ap`
`386d791fd..05113224d`
* AP bootstrap
	* <https://wiki.osdev.org/Symmetric_Multiprocessing#AP_startup>
	* Intel v3 8.4.4 MP Initialization Example
	* Intel v3 10.6.1 Interrupt Command Register (ICR)
	* Intel v3 10.5.3 Error Handling
	* <https://stackoverflow.com/questions/70147401/>
	* Use `objdump -m i8086 -Sd runtime.exe` to view real mode code
* Rename functions from `*x86*` to `*x86_64*`, using `replace_x86.py` for help
* Add GitHub actions to check whether the project compiles automatically

### `xmhf64-vm`
`88ffba9e3..2c7662626`
* Solved old problem: VMWRITE in QEMU fails because not implemented by KVM
	* Remember that QEMU uses KVM nested virtualization (Turtles Project)
	* <https://www.kernel.org/doc/html/latest/virt/kvm/nested-vmx.html>
	* In Linux source code `arch/x86/include/asm/vmx.h`, see `enum vmcs_field`
	* Turns out that the field written to are unused by XMHF (value = 0).
	* So just skip them. Create a compiler switch to control the skip
	* QEMU x86 now can at least enter GRUB and see E820 interrupts
	* Still have strange error when booting Debian
* Ported some assembly code from x86 to x86_64

`2c7662626..c2d641da3`
* Add proposal of changing EPT in `_vmx_setupEPT()`
* Prevent VMREAD to fail in QEMU
* VMENTRY fail in QEMU: fix incorrect VMCS setting. After fix can enter grub
	* Intel v3 25 VM ENTRIES
	* Intel v3 23.7.1 VM-Exit Controls
	* Intel v3 25.2.4 Checks Related to Address-Space Size
* QEMU and HP: encounter "Unhandled intercept: 0x00000002"
* Unhandled intercept: 0x00000002 (triple fault) (`bug_001`)
	* Intel v3 Appendix C: VMX BASIC EXIT REASONS
	* Caused by guest trying to set CR0.PG
	* Add `nokaslr` in grub: keep kernel address the same
	* Compare x86 and x64 registers, found that IA32_EFER.LME is mistakenly set
	* Clear IA32_EFER.LME manually to fix triple fault (first fault is GP)
	* Now x86 and x64 progresses to the same location in QEMU

### `xmhf64`
`eca643c3c..e74a28b83`
* Allow `INVPCID` in guest (`bug_002`)
	* Intel v3 23.6.2 Processor-Based VM-Execution Controls
	* Intel v3 Table 23-7
* Allow `RDTSCP` in guest (`bug_003`)
	* Intel v3 17.17.2 IA32_TSC_AUX Register and RDTSCP Support
* Propagate RDMSR exception to guest (`bug_004`)
	* Intel v3 25.6 EVENT INJECTION
* (Now can boot Debian 11 in QEMU, x86 and x64)
* Check `INVPCID` and `RDTSCP` before setting them in VMCS (`bug_005`)
* (Now can boot Ubuntu 12.04 in HP, x86 and x64)

`e74a28b83..e945f7b27`
* Change TrustVisor code to use correct size (`bug_006`)
* Follow CR0 fields that need to be 1 (`bug_008`)
* Do not forward read / write of EFER MSR (`bug_009`)
* Set "IA-32e mode guest" VM Entry Control correctly (`bug_009`)
* Do not forward read / write of GS base MSR (`bug_010`)
* Disable x2APIC in CPUID (`bug_011`)
* (Now can boot x64 Debian in QEMU, HP no regression, HP x86 Debian 11 success)

`e945f7b27..565cef3cf`
* Double runtime stack size for x64 (`bug_013`)
* Update `hpt_emhf_get_guest_hpt_type()` to detect long mode paging (`bug_012`)
* Fix typo of `assert(lvl<=3);` for `HPT_TYPE_LONG` (`bug_012`)
* (Now can run x86 TrustVisor `pal_demo` in QEMU x64 XMHF x64 Debian 11)

`565cef3cf..36fc963ae`
* Implement `xmhf_baseplatform_arch_x86_64vmx_dump_vcpu()` for debugging
* Update `vmx_handle_intercept_cr0access_ug()`
* Change TR access right initialization (`bug_014`)
* Only update "IA-32e mode guest" in VMCS when CR0.PG changes (`bug_015`)
* (Now can boot x64 Debian x64 XMHF in HP)
* Change `scode.c` to extend integer sizes to 64-bits (`bug_017`)
* Consider CR3 as match if address bits match (`bug_017`)
* Increase runtime stack size from 32K to 64K (`bug_017`)
* (Now can stably run x86 TrustVisor `pal_demo` in QEMU x64 XMHF x64 Debian 11)

`36fc963ae..b04ca351a`
* Lock pages to RAM in `pal_demo`
* Fix race condition in quiesce code (`bug_018`)
* <del>Make access to `vcpu->quiesced` atomic in NMI handler</del> (`bug_018`)
* Prevent accessing memory below RSP in xcph (`bug_018`)
* Prevent overwriting other fields when reading 32-bit VMCS fields (`bug_018`)
* Implement virtual NMI, unblock NMI when needed (`bug_018`)
* Add gcc `-mno-red-zone` to prevent vmread fail (`bug_018`)

`b04ca351a..592fbd12c`
* Fix e820 not able to detect last entry error (`bug_019`)
* Update runtime paging and EPT to support more than 4GB memory (`bug_020`)
* Prevent injecting NMI to PAL by fixing race condition (`bug_021`)
* Fix gpa truncated bug in `_vmx_handle_intercept_eptviolation` (`bug_022`)
* Optimize VMCS-related code for dealing with 64 bit fields (`bug_023`)
* Fix deadlock in `xmhf_smpguest_arch_x86{,_64}vmx_quiesce()` (`bug_024`)
* (Now running x86 PALs at all configurations are stable)

`592fbd12c..f835a5710`
* Update `pal_demo` for 64-bits (`bug_026`)
* Change `trustvisor.h` to be able to hold 64-bit pointers (`bug_026`)
* Update TrustVisor's APIs to accept 64-bit pointers (`bug_026`)
* Detect whether guest application running in 64-bit mode (`bug_026`)
* Implement `scode_marshall64()` to marshall arguments for 64-bit (`bug_026`)
* Update `pal_demo` to test argument passing (`bug_026`)
* (Now can run x64 PALs, all configurations (Debian and Ubuntu) look good)

`3b199dbe0..43b3d6552`
* Clear VMXE from CR4 shadow, update CR4 intercept handler (`bug_027`)
* For PAE paging, update `guest_PDPTE*` after changing `guest_CR3` (`bug_028`)
* Support booting XMHF with only one CPU (`bug_032`)
* Make virtual APs' state follow Intel's specification (`bug_033`)
* Update CR0 intercept handler to halt when PAE changes to set (`bug_033`)
* Found a problem in QEMU, need to disable SMM in SeaBIOS (`bug_031`)
* Support `CR{0,4}` intercept with R8-R15 (`bug_034`)
* (Now can boot Windows 10 in QEMU)

`43b3d6552..5324ae8e8`
* Fix bug of interpreting RSP in real mode (`bug_036`)
* Refactor MTRR code (`bug_036`)
* Remove hardcoding of memorytype=WB in `_vmx_setupEPT()` (`bug_036`)
* Block guest's changes to MTRR (`bug_036`)
* Fix compile errors due to data types (`bug_037`)
* Use DS to access memory to work in Inel TXT mode (`bug_037`)
* Read MTRR default type MSR and apply it (`bug_036`)
* For PAL, update Makefile to set Windows macro (`bug_038`)
* For PAL, use Windows memory APIs (`bug_038`)
* (Now can boot x86 Windows 10 in HP, and run PAL)

`5324ae8e8..221e7b84b`
* Retry MOV CR0 interceptions when CR0.PG changes (`bug_040`)
* Ignore guest OS's request to update microcode (`bug_039`)
* Set hypervisor present flag in CPUID (`bug_039`)
* (Now can boot x64 Windows 10 in HP, and run PAL)

`221e7b84b..6733ca433`
* Translate x64 Windows calling convention to Linux (`bug_038`)
* Fix bug in `hpt_scode_switch_regular()` that cause unaligned RSP (`bug_038`)
* Use `uintptr_t` instead of `long` to get correct size (`bug_038`)
* Support munmap memory in caller.c (`bug_038`)
* (Now can run PAL in x64 Windows 10 and x64 Fedora)

`6733ca433..a08f148a1`
* Support booting a grub located other than `(hd0)` (`bug_043`)
* Remove some unnecessary `__attribute__((packed))` (`bug_043`)
* Fix unexpected content on screen when debugging with VGA (`bug_044`)
* Reset segment descriptors in x64 sl entry (`bug_042`)
* Add attribute packed to _txt_acmod.h (`bug_042`)
* Fix unsigned overflow in udelay() in init.c (`bug_045`)
* Check for overflow of latchregval in udelay() (`bug_045`)
* Fix type size in `xmhf_baseplatform_arch_x86_64vmx_wakeupAPs()` (`bug_046`)
* Fix `x86_64` alignment problem for `_txt_heap.h` (`bug_046`)
* Use sparse file to save build disk space (`bug_047`)

`a08f148a1..0240f2b7a`
* Miscelaneous changes
* PR from Miao: try to add DMAP support to XMHF (`bug_050`)

`0240f2b7a..65a3f5ea4`
* Merge `x86` and `x86_64` code (`bug_049`)
* Ideally, merging should not change semantics, so try to skip these commits
  when bisecting etc.

`65a3f5ea4..516e618f6`
* Check assumptions about EFLAGS.IF in TrustVisor (`bug_051`)
* Block NMI when running PALs (`bug_051`)
* Halt if dropping NMI (`bug_051`)
* Rename `TARGET_WORDSIZE` to `TARGET_SUBARCH`
* Support configuring `AMD64_MAX_PHYS_ADDR`
* Rearrange x64 GDT: make code32 + 8 = data32 (`bug_046`)
* (Now can boot x64 XMHF with DRT, but cannot run PALs)

`516e618f6..ec78a946a`
* Fix problem in types when calling `hash_memory_multi()` (`bug_053`)
* Allow guest to change MTRR (`bug_054`)
* Notify hypapp when MTRR changes (`bug_054`)
* Do not update EPT when MTRRs are disabled, to be efficient (`bug_054`)
* For Intel, unblock NMI when runtime starts (`bug_055`)

`ec78a946a..cd81de2b9`
* Avoid compiling DMAP when DMAP is disabled (`bug_056`)
* Avoid compiling some DRT object files when DRT is disabled (`bug_056`)
* Avoid saving zeros in .o files and .exe files (`bug_056`)
* Remove libtommath.a from bootloader (not used) (`bug_056`)
* Split large build targets for parallelism (`bug_056`)
* Update .github/build.sh (`bug_056`)

`cd81de2b9..822842a83`
* PR from Miao: fix bug in 32-bit DMAP
* Define runtime.mk to capture common logic in runtime Makefiles (`bug_057`)
* Write documentation for setting up XMHF64 (`bug_057`)
* Fix compile errors when compiling on Fedora (`bug_057`)
* Add documentations for compiling on Fedora (`bug_057`)
* Raise compile error when subarch not in (i386, amd64) (`bug_059`)
* Fix compile warnings / errors when compile with `-O3` (`bug_058`)
* Add ELF sections to linker scripts when compile with `-O3` (`bug_058`)
* Use `volatile` keyword to fix runtime problems when `-O3` (`bug_058`)

`822842a83..7abab5b91`
* Combine common logic in page alignment macros (`bug_060`)
* Add type checking in page alignment macros (`bug_060`)
* Remove magic numbers depending on `__TARGET_BASE_SL` (`bug_061`)
* Merge LD scripts across different subarchs (`bug_061`)
* Change TR access rights for i386 (`bug_062`)
* In QEMU, print warning if running amd64 guest in i386 XMHF (`bug_062`)
* Support running Jenkins for CI (`bug_063`)
* Support running Circle CI for CI (`bug_063`)
* Optimize XMHF for repeated intercepts (CPUID, WRMSR 0x6e0) (`bug_064`)
* Optimize XMHF for sending LAPIC EOI (`bug_064`)
* Enable Circle CI testing (`bug_064`)

`7abab5b91..18ecd868e`
* Update documentation
* Fix some problems with Makefiles (`bug_066`)
* Update NMI quiesce code (`bug_065`)
* Support x2APIC (`bug_065`)
* Add comments for the hpt library (`bug_067`)
* Use debugfs to speed up CI testing (`bug_069`)

`18ecd868e..` (a65ee0d63)
* Support forwarding guest's microcode update request to hardware (`bug_067`)
* Separate grub.py to build minimal GRUB image for running XMHF
* Allow guest to send INIT-SIPI-SIPI to "All Excluding Self" (`bug_074`)

### `xmhf64-dev`: development workarounds
* `59b3fd053`: Quiet TrustVisor output
* `0d7e0599d`: Handle VMCALL `KVM_HC_VAPIC_POLL_IRQ` (run WinXP SP3 on QEMU)
* `fded7ace5`: Add monitor trap and breakpoint support

### TODO
* Review unaligned structs caused by `__attribute__((packed))`
* Decide a coding format.

## Support Status
Linux
```rst
+----+---+------------------+------------+-------------------------------------+
|    |   |                  |            | Status                              |
|    |   |Operating         |            +------------------+------------------+
|XMHF|DRT|System            |Application | HP               | QEMU             |
+====+===+==================+============+==================+==================+
|  * | * | Ubuntu 12.04 x86 |pal_demo x86| good             | good (no DRT)    |
+----+   +------------------+------------+                  |                  |
|  * |   | Debian 11 x86    |pal_demo x86|                  |                  |
+----+   +------------------+------------+                  |                  |
| x64|   | Debian 11 x64    |pal_demo *  |                  |                  |
+----+   +------------------+------------+                  |                  |
| x64|   | Fedora 35 x64    |pal_demo *  |                  |                  |
+----+---+------------------+------------+------------------+------------------+
```

Windows
```rst
+----+---+------------------+------------+-------------------------------------+
|    |   |                  |            | Status                              |
|    |   |Operating         |            +------------------+------------------+
|XMHF|DRT|System            |Application | HP               | QEMU             |
+====+===+==================+============+==================+==================+
|  * | * | WinXP x86 SP3    |pal_demo x86| good             | good (no DRT)    |
+----+   +------------------+------------+                  |                  |
| x64|   | WinXP x64        |pal_demo *  |                  |                  |
+----+   +------------------+------------+                  |                  |
|  * |   | Win7 x86         |pal_demo x86|                  |                  | 
+----+   +------------------+------------+                  |                  |
| x64|   | Win7 x64         |pal_demo *  |                  |                  | 
+----+   +------------------+------------+                  |                  |
| x64|   | Win8.1 x64       |pal_demo *  |                  |                  |
+----+   +------------------+------------+                  |                  |
|  * |   | Win10 x86        |pal_demo x86|                  |                  |
+----+   +------------------+------------+                  +------------------+
| x64|   | Win10 x64        |pal_demo *  |                  | PAL not tested   |
+----+---+------------------+------------+------------------+------------------+
```

## Limits
* QEMU cannot reboot (`bug_007` fixes part of this problem)
* Grub graphical mode in HP is slow (MTRR not optimized; see `bug_054`)
* Terminating a PAL (e.g. through Ctrl+C) crashes XMHF
* Forwarding very frequent NMIs to Linux may have a bug (`bug_025`)
* x86 XMHF does not support x86 PAE and x64 guests (see `bug_028`)
* x86 Windows XP SP3 has strange VMCALL, workaround is `0d7e0599d` (`bug_029`)
* UEFI is not supported
* XMHF does not support guest OS performing INIT-SIPI-SIPI trice (`bug_075`)

