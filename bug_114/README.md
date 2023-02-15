# XMHF64 Evaluation

## Scope
* All configurations

## Behavior
We need to evaluate XMHF64's performance for the research

## Debugging

### XMHF Versions

We use latest version `xmhf64 f6c71ded5` as XMHF64, we use `v6.1.0 3cd28bfe5`
as XMHF.

Remove the call to `xmhf_debug_arch_putc` in `emhfc_putchar`. See
`xmhf64-dev d0e5be5d8` for an example.

old XMHF compile:
```
./autogen.sh && ./configure '--with-approot=hypapps/trustvisor' '--disable-dmap' '--disable-debug-serial'???
```

new XMHF compile:
```
./autogen.sh && ./configure '--with-approot=hypapps/trustvisor' '--disable-dmap' '--with-target-subarch=i386' && make -j 8
```

new XMHF compile with O3:
```
./autogen.sh && ./configure '--with-approot=hypapps/trustvisor' '--disable-dmap' '--with-target-subarch=i386' '--with-opt=-O3 -Wno-array-bounds' && make -j 8
```

new XMHF64:
```
TODO: max memory
```

XMHF64 compile

TODO: Remove all printfs.

### Benchmarks

