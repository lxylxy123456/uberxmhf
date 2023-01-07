# Port XMHF64 back to uberXMHF repo

## Scope
* All XMHF
* Git `cb825af84`

## Behavior
I have been working on the fork <https://github.com/lxylxy123456/uberxmhf/>.
Now it's probably time to merge to <https://github.com/uberspark/uberxmhf/>.

## Debugging

### Contacting maintainers
Posted on uberXMHF's forum <https://forums.uberspark.org/t/extending/453>.

Amit proposed using the new name `xmhf-64` for code and `docs/pc-intel-x86_64`
for documentation.

### Creating PR

Going to use `uberxmhf-merge` branch for the PR.

The upstream is at tag `v6.1.0` (`3cd28bfe5`) and has not been changed for a
long time.

### Summarizing changes

See `commits.txt`. First 80 cols are simply list of all commits, generated
using `gi log --oneline | cut -b 1-79`. Other cols are manual annotations.
Use tab size = 4 spaces.

### Process the data above

The data above is also saved in `commits.txt`. Write `commits.py` to process.

```sh
python3 commits.py > feature_bug.md
```

Currently waiting for PR review

### Update after nested virtualization

After implementing nested virtualization, updated `commits.txt` with git
commits `d0f2be0fb..4cfa1eee1`.

