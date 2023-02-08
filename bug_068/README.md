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

### Documentation

```
pip3 install sphinx sphinx-jsondomain
cd docs
make clean
make docs_html
```

However, see error
```
Extension error:
Could not import extension sphinx.builders.latex (exception: cannot import name 'contextfunction' from 'jinja2' (/usr/local/lib/python3.11/site-packages/jinja2/__init__.py))
make: *** [Makefile:29: docs_html] Error 2
```

### Changes to do

First
* Move all files to xmhf-64 folder
* Restore uberXMHF code (reverts 669ffe4253f6f6389c8217f64e8bdddde36ee394)

Then
* Update `CHANGELOG.md`
* Update `docs/pc-intel-x86_64/`, `docs/.gitignore`, `docs/index.rst`
* Remove `xmhf-64/xmhf/src/xmhf-core/verification`
* Remove unsupported hypapps
* Move CI to `tools/*`, add README
* Remove docs:
	* `xmhf-64/xmhf/doc`
	* `xmhf-64/hypapps/trustvisor/doc`
	* `xmhf-64/hypapps/trustvisor/README.md`
	* `xmhf-64/hypapps/trustvisor/tee-sdk/README.md`
	* `xmhf-64/hypapps/trustvisor/tee-sdk/doc`
	* `xmhf-64/README.md`

PR created: <https://github.com/uberspark/uberxmhf/pull/36>

