#!/bin/bash

set -xe

CONFIGURE_ARGS="--with-approot=hypapps/trustvisor"
CONFIGURE_ARGS="${CONFIGURE_ARGS} --disable-drt"
CONFIGURE_ARGS="${CONFIGURE_ARGS} --enable-debug-symbols"

if [ "$1" == "x86" ]; then
	CONFIGURE_ARGS="${CONFIGURE_ARGS} CC=i686-linux-gnu-gcc"
	CONFIGURE_ARGS="${CONFIGURE_ARGS} LD=i686-linux-gnu-ld"
else if [ "$1" == "x86_64" ]; then
	CONFIGURE_ARGS="${CONFIGURE_ARGS} --with-target-hwplatform=x86_64"
else
	echo '$1 incorrect, should be x86 or x86_64'; exit 1
fi; fi

if [ "$2" == "release" ]; then
	CONFIGURE_ARGS="${CONFIGURE_ARGS}"
else if [ "$2" == "debug" ]; then
	CONFIGURE_ARGS="${CONFIGURE_ARGS} --enable-debug-qemu"
else
	echo '$2 incorrect, should be release or debug'; exit 1
fi; fi

./autogen.sh
./configure ${CONFIGURE_ARGS}
make -j "$(nproc)"

