#!/bin/bash
set -xe
set -o pipefail
COMMENT="u6: XMHF64 O0, Ubuntu"
# Running the following line 2 times will fail. Use `umount /tmp`
sudo rm -rf /media/dev/ntfs/tmp/ && mkdir /media/dev/ntfs/tmp/ && sudo mount --bind /media/dev/ntfs/tmp/ /tmp/
rm -rf ben
mkdir ben
cd ben
export PATH="$HOME/sysbench-1.0.20/src:$PATH"
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
