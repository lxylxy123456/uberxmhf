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
	
	# Old implementation: $1 will be overwritten even if no changes are made.
	# indent -linux -ts4 -i4 "$1"

	# New implementation: only modify $1 when changing content.
	TEMP_FILE="`mktemp`"
	cp "$1" "$TEMP_FILE"
	VERSION_CONTROL=none indent -linux -ts4 -i4 "$TEMP_FILE"
	if diff "$1" "$TEMP_FILE" > /dev/null; then
		rm "$TEMP_FILE"
	else
		mv "$1" "$1~"
		mv "$TEMP_FILE" "$1"
	fi
fi

