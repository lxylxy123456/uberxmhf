#!/bin/bash
set -xe
set -o pipefail
COMMENT="0: Debian light"
sudo rm -rf /media/dev/Last/tmp/ && mkdir /media/dev/Last/tmp/ && sudo mount --bind /media/dev/Last/tmp/ /tmp/
rm -rf ben
mkdir ben
cd ben
script -c "../sysbench3.sh $COMMENT" log
# sleep 5
# script -c "../palbench.sh ../pal_bench64 $COMMENT" plog
echo DONE
tar c * | xz -9 | base64
echo END
