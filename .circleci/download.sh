#!/bin/bash

set -xe
if [ $# -le 0 ]; then
	echo "Error: no enough arguments"; exit 1
fi

DOWNLOAD_DIR="$1"
mkdir -p "$DOWNLOAD_DIR"

if ! python3 -c 'import gdown'; then
	python3 -m pip install gdown
fi

download () {
	if [ -f "$2" ]; then
		echo "$2 already exists"
	else
		gdown "$1" -O "$2"
	fi
}

download "1T1Yw8cBa2zo1fWSZIkry0aOuz2pZS6Tl" "$DOWNLOAD_DIR/debian11x86.qcow2"
# download "1WntdHCKNmuJ5I34xqriokMlSAC3KcqL-" "$DOWNLOAD_DIR/debian11x64.qcow2"

