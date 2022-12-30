# Review retrying in guestmem code

## Scope
* All configurations
* `lhv 37f8ad0bd`
* `lhv-dev b10f8c919`
* `xmhf64 0230ac339`
* `xmhf64-dev 23e699817`

## Behavior

Currently guestmem functions like `guestmem_copy_h2gv` will retry the guest
memory operation if another CPU changes EPT (`vcpu->vmx_ept_changed = 1`).
However, this may store to guest memory twice, and can be detected by another
CPU.

## Debugging

### Old design

The old design was made in `bug_106`. A CPU local flag `vcpu->vmx_ept_changed`
is used to signal retry.

* Logic for updating EPT (writer)
	* Quiesce (s.t. no other readers and writers are executing code)
	* (Nothing before critical section)
	* Modify EPT
	* Flush EPT TLB with `MEMP_FLUSHTLB_ENTRY`
		* `vcpu->vmx_ept_changed = 1`
	* End quiesce
* Logic for software walk EPT (reader)
	* (Nothing before critical section)
	* Walk EPT
	* Perform memory read / write
	* If `vcpu->vmx_ept_changed == 1`, retry

Current writers are (search for `MEMP_FLUSHTLB_ENTRY`):
* 3 `scode_register()` etc functions (in quiesce)
* `vmx_lapic_changemapping()` (only change current CPU)
* Note: green box code may not be in quiesce

Current readers are (search for `vmx_ept_changed`):
* 4 `guestmem_copy_*()` functions
* `ept12_pa2ptr()`

### Problem demonstration

A scenario demonstrating this problem is:

| Time | CPU 0 | CPU 1 | CPU 2 |
|-|-|-|-|
| 1 | Start page table walk |  |  |
| 2 | Get EPTP |  |  |
| 3 | Get PML4E |  |  |
| 4 | Get PDPTE |  |  |
| 5 | Get PDE |  |  |
| 6 | Get PTE |  |  |
| 7 | Write 1 to 0x1234 |  |  |
| 8 |  |  | XCHG(0x1234, RAX) |
| 9 |  |  | increase counter (b/c RAX == 1) |
| 10 |  | Quiesce |  |
| 11 |  | `vcpu->vmx_ept_changed = 1` |  |
| 12 | Retry (b/c `vcpu->vmx_ept_changed == 1`) |  |  |
| 13 | Get EPTP |  |  |
| 14 | Get PML4E |  |  |
| 15 | Get PDPTE |  |  |
| 16 | Get PDE |  |  |
| 17 | Get PTE |  |  |
| 18 | Write 1 to 0x1234 |  |  |
| 19 | End page table walk |  |  |
| 20 |  |  | XCHG(0x1234, RAX) |
| 21 |  |  | increase counter (b/c RAX == 1) |

CPU 0 performs `*0x1234 = 1` once, but CPU 2 reads `1` written to `*0x1234`
twice. This is a problem if CPU 0 and CPU 2 are running a spin lock.

### New design

The problem now is that quiesce cannot interrupt reader, so we need some
synchronization between reader and writer. We use read write lock that is
optimized for reader.

* Writer: hold write lock, quiesce also holds writer lock before quiesce.
* Reader: hold read lock

The locking design is:

* Write lock
	* Lock
		* Lock `write_lock` (spin lock)
		* `global_write = 1`
		* For each CPU
			* Wait until `vcpu->local_read = 0`
	* Unlock
		* `global_write = 0`
		* Unlock `write_lock`
* Read lock
	* Lock
		* While true
			* While `global_write == 1`, PAUSE
			* `vcpu->local_read = 1`
			* If `global_write == 0`, break
			* `vcpu->local_read = 0`
	* Unlock
		* `vcpu->local_read = 0`

Explanation:
* `write_lock` makes sure there are only 1 writer
* `global_write` indicates a writer is waiting. After this flag is set each CPU
  can execute at most 1 reader critical section
* `vcpu->local_read` indicates a CPU is in reader critical section

Critical section requirements from 15410
* Mutual exclusion: writer sets `global_write = 1` when locking. If reader is
  in critical section, then there must be `vcpu->local_read = 1`. Writer will
  wait for reader to exit critical section before entering writer's critical
  section.
* Progress: after writer sets `global_write = 1`, no more readers will enter
  critical section. Then writer enters critical section and release lock.
* Bounded waiting: when there are too many writers, readers may starve. However
  this problem also exists in previous design (e.g. reader retry forever)

This is implemented in `xmhf64-dev 23e699817..aa55726e2`. Cherry picked to
`xmhf64 e641acea5`. Renames:
* `write_lock` above becomes `g_eptlock_write_lock`.
* `global_write` above becomes `g_eptlock_write_pending`.
* `vcpu->local_read` above becomes `vcpu->vmx_eptlock_reading`.

## Fix

`xmhf64 0230ac339..e641acea5`
* Replace retry logic in guestmem with read write lock

