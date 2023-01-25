# Race condition in `xmhf_smpguest_arch_x86vmx_mhv_nmi_enable`

## Scope
* Reproducible in QEMU
* x64 (x86 not tried)
* `xmhf64 531c11afd`
* `xmhf64-dev 21422c5b3`

## Behavior

This bug was found while debugging `bug_025`. Build and run commands:
```sh
./build.sh amd64 fast && gr
../tmp/bios-qemu.sh --gdb 1234 -d build64 +1 -d debian11x64 -t -qmp tcp:localhost:4444,server,wait=off -smp 1 &
python3 bug_018/qemu_inject_nmi.py 4444 0
```

After a few seconds, stop `qemu_inject_nmi.py`. There are two possibilities:
* Stuck in Linux code (`bug_025`)
* Stuck in XMHF code, because `vcpu->vmx_mhv_nmi_visited = 0xfffffffe` (this
  bug)

## Debugging

Apparently, `vcpu->vmx_mhv_nmi_visited` is decremented from 0. To confirm this,
running `xmhf64-dev 0388aaacc` will see assertion error,

The 2 key functions are `xmhf_smpguest_arch_x86vmx_mhv_nmi_enable` and
`xmhf_smpguest_arch_x86vmx_eventhandler_nmiexception`. Pseudo code:

```
handle_nmi
	...

nmi_disable
D1	HALT_ON_ERRORCOND(g_nmi_enable);
D2	g_nmi_enable = false;

nmi_enable
E1	HALT_ON_ERRORCOND(!g_nmi_enable);
E2	g_nmi_enable = true;
E3	while (g_nmi_visited) {
E4		g_nmi_visited--;
E5		g_nmi_enable = false;
E6		handle_nmi(vcpu);
E7		g_nmi_enable = true;
E8	}
E9	HALT_ON_ERRORCOND(g_nmi_enable);

nmi_handler
H1	if (!g_nmi_enable) {
H2		g_nmi_visited++;
H3		HALT_ON_ERRORCOND(g_nmi_visited);
H4	} else {
H5		nmi_disable(vcpu);
H6		handle_nmi(vcpu);
H7		nmi_enable(vcpu);
H8	}
```

The design is first made in `bug_083` (commit `abe41f5f1`). At first, the
design is that number of NMIs are not counter. Instead allow combining multiple
NMIs received from hardware to one call to `handle_nmi()`.

However, the design is changed to make sure that number of calls to
`handle_nmi()` match number of NMIs delivered to hardware. In `bug_087` commit
`c01a32408`, `guest_nmi_pending` is changed from `bool` to `u32`. However, this
causes the race condition.

Note that `nmi_enable()` can be called in normal environment (NMI unblocked),
though `nmi_handler()` has NMI disabled. The call stack is

```
Normal function (e.g. peh-x86vmx-main.c)
	nmi_enable()
		Line E3: see g_nmi_visited == 1, enter loop
		NMI arrives
			nmi_handler()
				Line H1: see g_nmi_enable = true
				Line H5: disable nmi
				Line H6: handle_nmi()
				Line H7: call nmi_enable()
					Line E3: see g_nmi_visited == 1, enter loop
					Line E4: g_nmi_visited--, becomes 0
					Line E6: handle_nmi()
		Line E4: g_nmi_visited--, becomes -1
```

To fix this problem, we need to make sure `nmi_enable()` is reentrant. We want
to implement "atomic decrease if not zero". Looks like there is no such
instruction in x86. A possible way is to use a while loop with CMPXCHG. However
we change the while loop to make sure `!g_nmi_enable` when modifying to
`g_nmi_visited`.

```
nmi_enable
	HALT_ON_ERRORCOND(!g_nmi_enable);
	g_nmi_enable = true;
	while (g_nmi_visited) {
		g_nmi_enable = false;
		if (g_nmi_visited) {
			g_nmi_visited--;
			handle_nmi(vcpu);
		}
		g_nmi_enable = true;
	}
	HALT_ON_ERRORCOND(g_nmi_enable);
```

Also make sure to add memory barriers.

Implemented in `xmhf64-dev b3e5e098c`, cherry picked to `xmhf64 7ea6b5f31`.

However, then I realized that it is not a good idea. Should remove call to
`nmi_enable()` in `nmi_handler()` in H5 and H7. Implemented in
`xmhf64 127869130`.

## Fix

`xmhf64 531c11afd..7ea6b5f31`
* Make `xmhf_smpguest_arch_x86vmx_mhv_nmi_enable()` reentrant

`xmhf64 37ff9bd7d..127869130`
* Revert `7ea6b5f31`, fix using another way

`xmhf64-dev 21422c5b3..227783839`
* Test unsigned underflow in `xmhf_smpguest_arch_x86vmx_mhv_nmi_enable()`

