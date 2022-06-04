#!/bin/bash
set -xe

DISK_FILE="$1"

[ -n "$DISK_FILE" ]
rm -f $DISK_FILE

dd if=/dev/zero of="$DISK_FILE" bs=1M count=0 seek=2
mformat -i "$DISK_FILE" ::

for i in hypapps/trustvisor/pal_demo/{main,test,test_args}{32,64}{,.exe}; do
	mcopy -i "$DISK_FILE" $i ::
done

mdir -i "$DISK_FILE" ::

