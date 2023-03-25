#!/bin/bash
set -xe
set -o pipefail
COMMENT="u9: XMHF64 O3, Ubuntu"
# Running the following line 2 times will fail. Use `umount /tmp`
sudo rm -rf /media/dev/ntfs/tmp/ && mkdir /media/dev/ntfs/tmp/ && sudo mount --bind /media/dev/ntfs/tmp/ /tmp/
rm -rf ben
mkdir ben
cd ben
script -c "../palbench2.sh ../pal_bench64 $COMMENT" plog
echo DONE
echo BEGIN_PAL
cat pal
echo END_PAL
echo BEGIN_ALL_FILES
tar c * | xz -9 | base64
echo END_ALL_FILES
echo END
