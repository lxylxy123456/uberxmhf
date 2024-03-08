#!/bin/bash
#
# The following license and copyright notice applies to all content in
# this repository where some other license does not take precedence. In
# particular, notices in sub-project directories and individual source
# files take precedence over this file.
#
# See COPYING for more information.
#
# eXtensible, Modular Hypervisor Framework 64 (XMHF64)
# Copyright (c) 2023 Eric Li
# All Rights Reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#
# Redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in
# the documentation and/or other materials provided with the
# distribution.
#
# Neither the name of the copyright holder nor the names of
# its contributors may be used to endorse or promote products derived
# from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
# CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
# INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
# BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
# TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
# ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
# TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
# THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.
#

set -xe
if [ $# -le 0 ]; then
	echo "Error: no enough arguments"; exit 1
fi

DOWNLOAD_DIR="$1"
mkdir -p "$DOWNLOAD_DIR"
shift

download () {
	TARGET_FILE="$DOWNLOAD_DIR/$2"
	if [ -f "$TARGET_FILE" ]; then
		echo "$TARGET_FILE already exists"
	else
		if ! python3 -c 'import gdown'; then
			python3 -m pip install gdown
		fi
		URL="https://drive.google.com/uc?id=${1}&confirm=t"
		python3 -m gdown.cli "$URL" -O "$TARGET_FILE"
	fi
}

while [ "$#" -gt 0 ]; do
	case "$1" in
		debian11x86.qcow2)
			download "1T1Yw8cBa2zo1fWSZIkry0aOuz2pZS6Tl" "$1"
			;;
		debian11x64.qcow2)
			download "1WntdHCKNmuJ5I34xqriokMlSAC3KcqL-" "$1"
			;;
		*)
			echo "Error: unknown file $1"; exit 1
			;;
	esac
	shift 
done

