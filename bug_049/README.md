# Merge x86 and x86_64 code

## Scope
* All XMHF
* Git `486583a05`

## Behavior
A long time ago we decided to split `x86` and `x86_64` code. However, not
splitting seems better because there are a lot of repeating code. In this bug,
we hope to merge the code in an automatic way.

Plan is:
* `x86` represents Intel instruction family (its sibling is `arm`)
* `i386` and `amd64` mean 32-bit and 64-bit respectively
  e.g. Debian uses this naming

## Debugging
The original splitting was done in `replace_x86.py`

Plan
* Phase 1: rename function names from `x86_64` back to `x86`
* Phase 2: edit files by adding `#ifdef`s to make them the same
* Phase 3: merge files (should be just deleteing `x86_64` versions)

### Phase 1

Use `merge_x86.py`

Need to update `xmhf_sl_arch_x86_setup_runtime_paging` in `xmhf-sl.h`

### Phase 2

Still use `merge_x86.py`

Found regression in commit `28e42cef9`. For x86 XMHF see:
"Unhandled intercept: 49"

```sh
git bisect start
git bisect bad 28e42cef9
git bisect good dd266a926
git bisect bad bf7bd770a
git bisect good 217859474
git bisect bad 699eed0d0
git bisect good 643b4fee4
```

Can see that the problem is that `P4L_NPDT` and `P4L_NPT` are wrong. Their
values are 0. This is because `PAGE_SIZE_4K` is 32-bits. Fixed in `c51de4165`.

