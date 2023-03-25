#!/bin/bash
set -xe
set -o pipefail
COMMENT="2xk: XMHF64, Debian full, KVM, Debian light"
rm -rf ben
mkdir ben
cd ben
script -c "../palbench2.sh ../pal_bench64L2 $COMMENT" plog
echo DONE
echo BEGIN_PAL
cat pal
echo END_PAL
echo BEGIN_ALL_FILES
tar c * | xz -9 | base64
echo END_ALL_FILES
echo END
