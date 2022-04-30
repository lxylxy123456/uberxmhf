# Sometimes remaking XMHF with parallel build does not update everything

## Scope
* All configurations
* Git `016013a8c`

## Behavior
After making XMHF, then update some code, and run `make -j 4`, some code are
not updated correctly.

The last few columns of `ls -l` are:
```
Apr 20 22:32 ./xmhf/src/xmhf-core/xmhf-runtime/runtime.bin
Apr 20 22:28 ./xmhf/src/xmhf-core/hypervisor-x86-amd64.bin.gz
Apr 20 22:28 ./hypervisor-x86-amd64.bin.gz
```

## Debugging

The problem is in `xmhf/src/xmhf-core/Makefile`. A similar logic is copied in
`Makefile.1`. Running this results in the following behavior
```
$ make -j 4
sleep .1
cd rt && echo rt > rt.bin && date >> rt.bin
sleep .1
cd sl && echo sl > sl.bin && date >> sl.bin
sleep 0.1
cd bl && echo bl > bl.bin && date >> bl.bin
# concatenate sl image and runtime image 
cat sl/sl.bin rt/rt.bin bl/bl.bin > gz
$ make -j 4
sleep .1
cd rt && echo rt > rt.bin && date >> rt.bin
sleep .1
cd sl && echo sl > sl.bin && date >> sl.bin
sleep 0.1
cd bl && echo bl > bl.bin && date >> bl.bin
$ 
```

We can see that the period is 2. In half of the time, the `$(HYPERVISOR_BIN_GZ)`
target is not made. The problem is that the prerequisits for
`$(HYPERVISOR_BIN_GZ)` are evaluated before `xmhf-runtime` directory is built.
Make sees that `xmhf-runtime/runtime.bin` is older than `$(HYPERVISOR_BIN_GZ)`,
so decides that `$(HYPERVISOR_BIN_GZ)` does not need to be updated. However,
later `xmhf-runtime/runtime.bin` is updated.

I have thought of two possible solutions
1. Make `sl rt bl` also prerequisits for `$(HYPERVISOR_BIN_GZ)`
2. Make `$(HYPERVISOR_BIN_GZ)` a phony target.

<https://www.gnu.org/software/make/manual/html_node/Phony-Targets.html> says
"A phony target should not be a prerequisite of a real target file". So I think
we should go with option 2. This is reasonable because the actual prerequisits
for `$(HYPERVISOR_BIN_GZ)` may contains hundreds of files. It is too hard to
list all of them, so we just always remake `$(HYPERVISOR_BIN_GZ)`.

## Fix

`016013a8c..5c29efd57`
* Fix bug in xmhf/src/xmhf-core/Makefile to correctly parallel remake

