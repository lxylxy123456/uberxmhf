# HP 2540p VGA corruption when DRT + DMAP

## Scope
* bad: DRT + DMAP
* good: DMAP
* good (assumed): DRT
* i386 and amd64
* `xmhf64 5185f1e6a`
* bad: HP 2540P
* good: Dell desktop

## Behavior
When building XMHF with `./build.sh i386 --drt --dmap` and running on HP 2540p,
see VGA corruption after Debian increases resolution.

## Debugging

First try to bisect the commit of this bug. At first I was trying
`./build.sh i386 release` on `xmhf64-tboot10-tmp 77b875fc0`. The oldest
bisection version can be determined using `bug_092`, which should ideally fix
the VGA corruption bug. Using `2c767d24b`.

* `77b875fc0`, DRT + DMAP: bad (`xmhf64-tboot10-tmp`)
* `77b875fc0`,       DMAP: good
* `5185f1e6a`, DRT + DMAP: bad (`xmhf64`)
* `5185f1e6a`,       DMAP: good
* `2c767d24b`, DRT + DMAP: good (oldest version)

Now we can start bisecting. Focus on PR commits.
* `6c6c3294b`, DRT + DMAP: bad2 (PMR cannot be disabled. Halting!)
* `1b9d60b57`, DRT + DMAP: bad (after PR 15)
* `a71084ef8`, DRT + DMAP: good (before PR 13)
* `8fcfa3917`, DRT + DMAP: bad2 (PMR cannot be disabled. Halting!)
* `ac3f3a510`, DRT + DMAP: bad2 (before PR 15)

At this point, I believe the regression is caused by 2 PRs:
* <https://github.com/lxylxy123456/uberxmhf/pull/13>
* <https://github.com/lxylxy123456/uberxmhf/pull/15>

Each PR changes the behavior.

More tests
* `8fcfa3917`,       DMAP: good (after PR 13)
* `1b9d60b57`,       DMAP: bad (after PR 15)
* `5185f1e6a`,       DMAP: bad (`xmhf64`) (previous test is good?!)
* `ac3f3a510`,       DMAP: good (before PR 15)
* `1b9d60b57`,       DMAP: bad (after PR 15)

Now my guess is that PR 15 fixes something broken by PR 13, but it breaks DMAP,
even when without DRT.

## Result

Discussed with superymk, not going to fix at this point. Xen has documented bug
on 2540p firmware / hardware. PR 15 is supposed to fix something, so maybe this
is not a regression.

