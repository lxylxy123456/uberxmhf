# Add type checking to `_paging.h` macros

## Scope
* All XMHF
* Git `822842a83`

## Behavior
Type checking should be performed such that `PA_PAGE_ALIGNED_4K()` is only used
for 64-bit types, and `PAGE_ALIGNED_4K()` is only used for word-size types.

## Debugging
`092ec73dc` combines a lot of common logic in similar macros (like
`PAGE_ALIGNED_4K()` and `PAGE_ALIGNED_2M()`).

Then use `_Static_assert()` to check `sizeof()` types passed to arguments.
There is a problem: these macors can no longer be used in declaring global
variable's array sizes. StackOverflow question asked:
<https://stackoverflow.com/questions/71699493/>

The workaround is to provide `PA_PAGE_ALIGN_UP_NOCHK_4K()` for such global
variables. Only align up macros are used in these case (i.e. not align down,
not aligned test)

## Fix
`822842a83..092ec73dc`, `2d1aa5f5f..85141ca6b`
* Combine common logic in page alignment macros
* Add type checking in page alignment macros

