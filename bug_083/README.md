# Support software NMI blocking

## Scope
* All XMHF
* Git `d2daaeaf9`

## Behavior
In some cases we want to prevent XMHF from injecting NMIs to the guest OS. We
need to implement this new feature carefully to prevent race conditions

## Debugging

### Current logic

The current logic originated from `bug_018`. The logic pseudo code is:
```c
var vmx_guest_inject_nmi;

peh() {
	vmx_guest_inject_nmi = 0
	if (exit_reason == nmi-windowing-exit) {
		VMCS.nmi-window = 0;
		inject NMI to guest;
	} else {
		...
	}
	if (vmx_guest_inject_nmi) {
		VMCS.nmi-window = 1;
	}
}

nmi_handler() {
	VMCS.nmi-window = 1;
	vmx_guest_inject_nmi = 1;
}
```

### Logic in PR 10

In <https://github.com/lxylxy123456/uberxmhf/pull/10>, Miao proposed a new
logic:
* `vmx_guest_inject_nmi` becomes `vmx_guest_start_inject_nmi`
* New CPU global variable `guest_nmi_enable` for `! software NMI blocking`
* New CPU global variable `guest_nmi_pending` for NMI pending when sw block

```c
var vmx_guest_start_inject_nmi;
var guest_nmi_enable;
var guest_nmi_pending;

peh() {
	vmx_guest_start_inject_nmi = 0
	if (exit_reason == nmi-windowing-exit) {
		if (guest_nmi_enable) {
			VMCS.nmi-window = 0;
			inject NMI to guest;
			guest_nmi_pending = 0
		} else {
			/* pass */
		}
	} else if (sw_nmi_try_disable) {
		guest_nmi_enable = 0;
	} else if (sw_nmi_enable) {
		guest_nmi_enable = 1;
		if (guest_nmi_pending) {
			inject_nmi()
		}
	} else {
		...
	}
	if (vmx_guest_start_inject_nmi) {
		VMCS.nmi-window = 1;
	}
}

nmi_handler() {
	if (guest_nmi_enable) {
		VMCS.nmi-window = 1;
		guest_nmi_pending = 1;
		vmx_guest_start_inject_nmi = 1;
	} else {
		guest_nmi_pending = 1;
		vmx_guest_start_inject_nmi = 0;
	}
}

inject_nmi() {
	VMCS.nmi-window = 1;
	guest_nmi_pending = 1;
	vmx_guest_start_inject_nmi = 1;
}
```

This has a race condition. When `nmi_handler()` is called just before
`sw_nmi_try_disable`, the `VMCS.nmi-window` will be set to 1. As a result,
the NMI will be lost forever, because in `peh()` there is `/* pass */`.

Also, `vmx_guest_start_inject_nmi = 0` in `nmi_handler()` is questionable,
because it should be 0 in the first place. (replace with assertion?)

Also two comments of `inject_nmi()`:
* This function should probably check `guest_nmi_enable`, instead of asking
  the caller to check.
* `nmi_handler()` should call this function instead of reimplementing its body.

### Revise interface

Now we try to make the interface better such that `sw_nmi_try_disable` can
cancel NMI window. Also, we let NMI-windowing exit always succeed.

Make sure that the only possible source of race is the NMI exception handler.
We do not allow `sw_nmi_disable()` and `sw_nmi_enable()` to run in the
exception handler (only intercept handler allowed).

```c
var vmx_guest_start_inject_nmi;	/* Ideally the same as VMCS.nmi-window, but needed to prevent race condition */
var vmx_guest_cancel_inject_nmi;	/* sw_nmi_disable is called, clear VMCS.nmi-window */
var guest_nmi_enable;	/* NMI is not blocked by software */
var guest_nmi_pending;	/* NMI pending, regardless of software NMI blocking */

peh() {
	vmx_guest_start_inject_nmi = 0
	vmx_guest_cancel_inject_nmi = 0;
	if (exit_reason == nmi-windowing-exit) {
		VMCS.nmi-window = 0;
		inject NMI to guest;
		guest_nmi_pending = 0
	} else if (sw_nmi_disable) {
		guest_nmi_enable = 0;
		vmx_guest_cancel_inject_nmi = 1;
	} else if (sw_nmi_enable) {
		guest_nmi_enable = 1;
		if (guest_nmi_pending) {
			inject_nmi()
		}
	} else {
		...
	}
	if (vmx_guest_start_inject_nmi) {
		VMCS.nmi-window = 1;
	}
	if (vmx_guest_cancel_inject_nmi) {
		VMCS.nmi-window = 0;
	}
}

nmi_handler() {
	if (guest_nmi_enable) {
		inject_nmi();
	} else {
		guest_nmi_pending = 1;
	}
}

inject_nmi() {
	VMCS.nmi-window = 1;
	guest_nmi_pending = 1;
	vmx_guest_start_inject_nmi = 1;
}
```

We also update the names
* `vmx_guest_start_inject_nmi -> vmx_guest_vmcs_nmi_window_set`
* `vmx_guest_cancel_inject_nmi -> vmx_guest_vmcs_nmi_window_clear`
* `guest_nmi_enable -> guest_nmi_block`
* `guest_nmi_pending -> guest_nmi_pending`

### Finalize

```c
var vmx_guest_vmcs_nmi_window_set;	/* Ideally the same as VMCS.nmi-window, but needed to prevent race condition */
var vmx_guest_vmcs_nmi_window_clear;	/* nmi_block is called, clear VMCS.nmi-window */
var vmx_guest_nmi_blocking_modified;	/* either nmi_block or nmi_unblock is called, used to catch calling both / calling multiple times */
var guest_nmi_block;	/* NMI is not blocked by software */
var guest_nmi_pending;	/* NMI pending, regardless of software NMI blocking */

peh() {
	vmx_guest_vmcs_nmi_window_set = 0
	vmx_guest_vmcs_nmi_window_clear = 0;
	vmx_guest_nmi_blocking_modified = 0;
	if (exit_reason == nmi-windowing-exit) {
		VMCS.nmi-window = 0;
		inject NMI to guest;
		guest_nmi_pending = 0
	} else if (nmi_block) {
		assert(!vmx_guest_nmi_blocking_modified);
		vmx_guest_nmi_blocking_modified = 1;
		guest_nmi_block = 1;
		vmx_guest_vmcs_nmi_window_clear = 1;
	} else if (nmi_unblock) {
		assert(!vmx_guest_nmi_blocking_modified);
		vmx_guest_nmi_blocking_modified = 1;
		guest_nmi_block = 0;
		if (guest_nmi_pending) {
			VMCS.nmi-window = 1;
			guest_nmi_pending = 1;
			vmx_guest_vmcs_nmi_window_set = 1;
		}
	} else {
		...
	}
	if (vmx_guest_vmcs_nmi_window_set) {
		VMCS.nmi-window = 1;
	}
	if (vmx_guest_vmcs_nmi_window_clear) {
		VMCS.nmi-window = 0;
	}
}

nmi_handler() {
	if (quiesce) {
		...
	} else {
		inject_nmi();
	}
}

inject_nmi() {
	guest_nmi_pending = 1;
	if (!guest_nmi_block) {
		VMCS.nmi-window = 1;
		vmx_guest_vmcs_nmi_window_set = 1;
	} /* no else */
}
```

This is implemented in `7518bc1d1..abe41f5f1`.

### Review call to `xmhf_smpguest_arch_x86vmx_unblock_nmi()`

We wonder whether `xmhf_smpguest_arch_x86vmx_unblock_nmi()` is called at the
right place?

In `5067ce7ec..713a3899e`, set up an experiment to test.

The serial output when I enter `nmi` in QEMU monitor is:
```
[iret][peh]CPU(0x00): inject NMI
```

This is correct. When NMI hits the guest OS (will print `[peh]`), NMI will be
blocked during VMEXIT. However, VMENTRY will not execute IRET, so we need to
execute IRET manually (will print `[iret]`).

When NMI hits hypervisor (will print `[xcph]`), the exception handler will
return with IRET, so we do not need to execute IRET manually (will print
`[no iret]`).

## Fix

`7518bc1d1..abe41f5f1`
* Implement NMI virtualization that supports blocking and unblocking NMI

