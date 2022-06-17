#!/bin/bash

set -xe

if [ $# = 0 ]; then
	find xmhf/src/xmhf-core/xmhf-runtime/xmhf-nested \
		-name '*.[ch]' -type f -exec "$0" {} \;
else
	SKIP_FILES=(
		'nested-x86vmx-vmcs12-fields.h'
		'nested-x86vmx-vmcs12-guesthost.h'
	)
	if [[ " ${SKIP_FILES[*]} " =~ " $(basename "$1") " ]]; then
		echo "Skip $1"
		exit 0
	fi
	indent -linux -ts4 -i4 "$1"
fi

