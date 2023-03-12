#!/bin/bash
set -xe
set -o pipefail
COMMENT="1x: XMHF64, Debian light"
sudo rm -rf /media/dev/Last/tmp/ && mkdir /media/dev/Last/tmp/ && sudo mount --bind /media/dev/Last/tmp/ /tmp/
rm -rf ben
mkdir ben
cd ben
script -c "../sysbench4.sh $COMMENT" log
sleep 5
script -c "../palbench.sh ../pal_bench64 $COMMENT" plog
echo DONE
echo BEGIN_RESULT
cat result
echo END_RESULT
echo BEGIN_PAL
cat pal
echo END_PAL
echo BEGIN_ALL_FILES
tar c * | xz -9 | base64
echo END_ALL_FILES
echo END
