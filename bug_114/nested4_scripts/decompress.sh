#!/bin/bash
set -xe
set -o pipefail

NAMES="log plog pal result"

[ -n "$1" ]

for i in $NAMES; do
	[ ! -f "$i" ]
	[ ! -f "$1_$i" ]
done

base64 -d | tar Jx

for i in $NAMES; do
	if [ -f "$i" ]; then
		mv -n "$i" "$1_$i"
	fi
done

echo DONE

