#!/bin/bash
set -xe
set -o pipefail
COMMENT="2xk: XMHF64, Debian full, KVM, Debian light"
rm -rf ben
mkdir ben
cd ben
script -c "../sysbench3.sh $COMMENT" log
sleep 5
# TODO
#script -c "../palbench.sh ../pal_bench64L2 $COMMENT" plog
echo DONE
tar c * | xz -9 | base64
echo END
