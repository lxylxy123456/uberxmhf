#!/bin/bash

set -xe

# Inputs from environment variables
[ -d "$GIT_DIR" ]
[ -d "$GDB_DIR" ]

BUILD_DIR="$(realpath "$1")"
[ -d "$BUILD_DIR" ]

if ! mountpoint "$BUILD_DIR"; then
	sudo mount -t tmpfs tmpfs -o uid=$USER,gid=$USER "$BUILD_DIR"
fi

cd "$GIT_DIR"
find * -type d -exec mkdir -p "${BUILD_DIR}"/{} \;
find * -type f -exec ln -Tsf "${GIT_DIR}"/{} "${BUILD_DIR}"/{} \;
ln -Tsf "${GDB_DIR}" "${BUILD_DIR}/gdb"
echo SUCCESS

