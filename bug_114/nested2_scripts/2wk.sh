#!/bin/bash
set -xe
set -o pipefail
COMMENT="2wk: Debian full, VMware, Debian light, KVM, Debian light"
rm -rf ben
mkdir ben
cd ben
script -c "../sysbench2.sh $COMMENT" log
# sleep 5
# script -c "../palbench.sh ../pal_bench64 $COMMENT" plog
echo DONE
tar c * | xz -9 | base64
echo END
