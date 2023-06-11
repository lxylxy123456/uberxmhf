#!/bin/bash
# alias.sh - some bash functions that speed up XMHF development.
# Copyright (C) 2023  Eric Li
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

SCRIPT_DIR="$(realpath "$(dirname $BASH_SOURCE)")"
echo . "$SCRIPT_DIR/$(basename $BASH_SOURCE)"
echo . "$SCRIPT_DIR/$(basename $BASH_SOURCE)" | xsel --clipboard

if ! (echo "$PATH" | grep "^$SCRIPT_DIR/bin" > /dev/null); then
	PATH="$SCRIPT_DIR/bin:$PATH"
fi

al () {
	. "$SCRIPT_DIR/$(basename $BASH_SOURCE)"
}

linksrc () {
	"$SCRIPT_DIR/mylinksrc.sh" "$@"
}

build_grub () {
	if [ ! -f "init-x86-$1.bin" ]; then echo ERROR 1; return 1; fi
	if [ ! -f "hypervisor-x86-$1.bin.gz" ]; then echo ERROR 2; return 1; fi
	BUILD_GRUB_SUBARCH="$1"
	shift 1
	python3 ./tools/ci/grub.py \
		--subarch "$BUILD_GRUB_SUBARCH" \
		--xmhf-bin "." \
		--work-dir "." \
		--boot-dir "./tools/ci/boot/" \
		"$@"
	rm -rf "./deb/"
	rm -rf "./grub/b.img"
}

build_uefi () {
	mkdir -p grub
	if ! dd if=/dev/zero of=grub/fat.img conv=sparse bs=1M count=512; then
		echo ERROR 1; return 1
	fi
	if ! mformat -i grub/fat.img ::; then echo ERROR 2; return 1; fi
	if ! mmd -i grub/fat.img ::/EFI; then echo ERROR 3; return 1; fi
	if ! mmd -i grub/fat.img ::/EFI/BOOT; then echo ERROR 4; return 1; fi
	if ! cat > grub/startup.nsh << FOE
# Run XMHF
FS1:
load \EFI\BOOT\init-x86-amd64.efi

# Sleep 1
stall 1000000

# Load Debian
FS2:
for %i in \EFI\debian\grubx64.efi \EFI\BOOT\BOOTX64.EFI
	if exists %i then
		%i
		exit
	endif
endfor
# \EFI\BOOT\BOOTX64.EFI
FOE
	then
		echo ERROR 5; return 1
	fi
	if ! cat > grub/init-x86-amd64.efi.conf << FOE
argv0 serial=115200,8n1,0x3f8 boot_drive=0x80
\EFI\BOOT\hypervisor-x86-amd64.bin
\EFI\BOOT\SINIT.bin
FOE
	then
		echo ERROR 6; return 1
	fi
	for i in init-x86-amd64.efi hypervisor-x86-amd64.bin grub/startup.nsh \
			 grub/init-x86-amd64.efi.conf; do
		if ! mcopy -i grub/fat.img "$i" ::/EFI/BOOT; then
			echo ERROR 7 "$i"; return 1
		fi
	done
	if ! mcopy -i grub/fat.img "$SCRIPT_DIR/$(basename $BASH_SOURCE)" \
		::/EFI/BOOT/SINIT.bin; then
		echo ERROR 7.5; return 1
	fi
	if ! fallocate -d grub/fat.img; then echo ERROR 8; return 1; fi
	# If BOOTX64.EFI is present and startup.nsh is not, will run automatically.
}

gr () {
	if [ -f init-x86-amd64.efi ]; then
		build_uefi "$@" && echo SUCCESS
	else
		if [ -f init-x86-i386.bin ]; then
			build_grub i386 "$@" && echo SUCCESS
		else
			build_grub amd64 "$@" && echo SUCCESS
		fi
	fi
}

# Start a builder docker container in CWD
builder () {
	echo './build.sh i386 debug'
	echo './build.sh amd64 debug'
	echo './build_pal_demo.sh all'
	echo 'cd hypapps/trustvisor/pal_demo'
	local VOLUMES=()
	VOLUMES+=("-v" "$PWD:$PWD")
	while [ "$#" -gt 0 ]; do
		if [ "$1" != "--" ]; then
			VOLUMES+=("-v" "$(realpath "$1"):$(realpath "$1")")
			shift 1
		else
			shift 1
			break
		fi
	done
	# Build container: `cd docker; docker build -t xmhf-compile .`
	docker run --rm -it -w "$PWD" "${VOLUMES[@]}" xmhf-compile "$@"
}

# Open a file based on its name suffix
o () {
	find -path "*/$1" -printf '%p\n' -exec gedit {} \;
}

