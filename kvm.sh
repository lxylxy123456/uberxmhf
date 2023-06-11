#!/bin/bash
# kvm.sh - run QEMU/KVM with less arguments.
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

# Run QEMU/KVM with less arguments.
# Note: this program used to be called bios-qemu.sh, which is not open source.
# Arguments:
#  -m 1G: set memory
#  --qb32: use qemu-system-i386
#  --ssh 2222: forward host port 2222 to guest port 22
#  --fwd 3333 25: forward host port 3333 to guest port 25
#  --smb DIR: share DIR in host using samba
#  --gdb 4444: use host port 4444 for GDB server
#  -smp 4: use 4 CPUs
#  -smp cpus=4,sockets=2: use 4 CPUs for Windows
#  -cpu Haswell,vmx=yes: set guest CPU model
#  --no-kvm: disable KVM
#  --win-bios: use bug_031/bios.bin for BIOS
#  --uefi: use /usr/share/OVMF/OVMF_CODE.fd for BIOS
#  --q35: Use Q35
#  --iommu: Enable Intel IOMMU (will enable --q35 and --win-bios)
#  -d name.qcow2 '': use drive name.qcow2
#  -d name.img -a: use drive name-a.img
#  -d name.qcow2 -a: use drive name-a.qcow2
#  -d name.img +a: use drive name+a.qcow2, if not exist, create from name.img
#  -d name.qcow2 +a: use drive name+a.qcow2, if not exist, create from name.qcow2
#  --drive-directsync ...: similar to -d, but do not cache
#  -c name.iso: use cdrom name.iso

set -xe

SCRIPT_DIR="$(realpath "$(dirname $BASH_SOURCE)")"

process_drive () {
	RAW_FILE="$1"
	[ -f "$RAW_FILE" ]
	RAW_FILE_REL="$(basename "$RAW_FILE")"
	RAW_FILE_NAME="${RAW_FILE%.*}"
	RAW_FILE_EXT="${RAW_FILE##*.}"
	NAME_MOD="$2"
	DRIVE_FILE="${RAW_FILE_NAME}${NAME_MOD}.qcow2"
	if [ ! -f "$DRIVE_FILE" ]; then
		if [ "${NAME_MOD:0:1}" == "+" ]; then
			case "$RAW_FILE_EXT" in
				img) RAW_FILE_FMT="raw"; ;;
				qcow2) RAW_FILE_FMT="qcow2"; ;;
				vdi) RAW_FILE_FMT="vdi"; ;;
				vmdk) RAW_FILE_FMT="vmdk"; ;;
				*) false; ;;
			esac
			qemu-img create -f qcow2 -b "$RAW_FILE_REL" -F "$RAW_FILE_FMT" \
				"$DRIVE_FILE"
		fi
		[ -f "$DRIVE_FILE" ]
	fi
}

MEM="1G"
QEMU_BIN="qemu-system-x86_64"
SSH_PORT=""
HOSTFWD=""
GDB_PORT=""
SMP="4"
CPU="Haswell,vmx=yes"
KVM="y"
WIN_BIOS="n"
UEFI="n"
Q35="n"
IOMMU="n"
DRIVE_ARGS=()
DRIVE_INDEX=0
EXTRA_ARGS=()

add_hostfwd () {
	HOSTFWD="${HOSTFWD},hostfwd=tcp::$1-:$2"
}

while [ "$#" -gt 0 ]; do
	case "$1" in
		-m) MEM="$2"; shift; ;;
		--qb32) QEMU_BIN="qemu-system-i386"; ;;
		--ssh) add_hostfwd "$2" 22; SSH_PORT="$2"; shift; ;;
		--fwd) add_hostfwd "$2" "$3"; shift 2; ;;
		--smb) HOSTFWD="${HOSTFWD},smb=$2"; shift; ;;
		--gdb) GDB_PORT="$2"; shift; ;;
		-smp) SMP="$2"; shift; ;;
		-cpu) CPU="$2"; shift; ;;
		--no-kvm) KVM="n"; ;;
		--win-bios) WIN_BIOS="y"; ;;
		--uefi) UEFI="y"; ;;
		--q35) Q35="y"; ;;
		--iommu) Q35="y"; IOMMU="y"; WIN_BIOS="y"; ;;
		-d|--drive)
			process_drive "$2" "$3"
			# $DRIVE_FILE is set by process_drive
			DRIVE_ARGS+=("-drive")
			DRIVE_ARGS+=("media=disk,file=$DRIVE_FILE,index=$DRIVE_INDEX")
			DRIVE_INDEX="$(($DRIVE_INDEX+1))"
			shift 2
			;;
		--drive-directsync)
			process_drive "$2" "$3"
			# $DRIVE_FILE is set by process_drive
			DRIVE_ARGS+=("-drive")
			CDS="cache=directsync"
			DRIVE_ARGS+=("media=disk,file=$DRIVE_FILE,index=$DRIVE_INDEX,$CDS")
			DRIVE_INDEX="$(($DRIVE_INDEX+1))"
			shift 2
			;;
		-c|--cdrom)
			DRIVE_ARGS+=("-drive")
			DRIVE_ARGS+=("media=cdrom,file=$2,index=$DRIVE_INDEX")
			DRIVE_INDEX="$(($DRIVE_INDEX+1))"
			shift 1
			;;
		*)
			EXTRA_ARGS+=("$1")
			;;
	esac
	shift 
done

ARGS=()
ARGS+=("-m" "$MEM")
if [ -n "$HOSTFWD" ]; then
	ARGS+=("-device" "e1000,netdev=net0")
	ARGS+=("-netdev" "user,id=net0${HOSTFWD}")
fi
if [ -n "$GDB_PORT" ]; then ARGS+=("-gdb" "tcp::${GDB_PORT}"); fi
ARGS+=("-smp" "$SMP")
ARGS+=("-cpu" "$CPU")
if [ "$KVM" == "y" ]; then ARGS+=("-enable-kvm"); fi
if [ "$WIN_BIOS" == "y" ]; then
	ARGS+=("-bios" "$SCRIPT_DIR/bug_031/bios.bin");
fi
if [ "$UEFI" == "y" ]; then
	ARGS+=("-bios" "/usr/share/OVMF/OVMF_CODE.fd");
	# TODO: without this, will get "Start PXE over IPv4"
	ARGS+=("-net" "none");
fi
if [ "$Q35" == "y" ]; then ARGS+=("-machine" "q35"); fi
if [ "$IOMMU" == "y" ]; then ARGS+=("-device" "intel-iommu"); fi
ARGS+=("-serial" "stdio")
ARGS+=("${DRIVE_ARGS[@]}")
ARGS+=("${EXTRA_ARGS[@]}")

set +x
if [ -n "$SSH_PORT" ]; then
	echo "ssh -p $SSH_PORT 127.0.0.1" >&2
fi
if [ -n "$GDB_PORT" ]; then
	echo "gdb --ex 'target remote :::$GDB_PORT'" >&2
fi
set -x

"$QEMU_BIN" "${ARGS[@]}" < /dev/null

