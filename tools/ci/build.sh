#!/bin/bash
# Usage: build.sh subarch [arguments [...]]
# subarch: i386 or amd64
# arguments:
#   --drt: enable DRT (--disable-drt)
#   --dmap: enable DMAP (--disable-dmap)
#   --no-dbg: do not use QEMU debug workarounds (--enable-debug-qemu)
#   --no-ucode: disable Intel microcode update (--enable-update-intel-ucode)
#   --app APP: set hypapp, default is "hypapps/trustvisor" (--with-approot)
#   --mem MEM: if amd64, set physical memory, default is 0x140000000 (5GiB)
#   --no-x2apic: hide x2APIC to workaround a bug (--enable-hide-x2apic)
#   --lhv-opt OPT: modify __LHV_OPT__ (u64) (--with-lhv-opt)
#   --no-rt-bss: skip runtime bss in image (--enable-skip-runtime-bss)
#                (Note: in LHV, this is always set)
#   --no-bl-hash: skip bootloader hashing (--enable-skip-bootloader-hash)
#                 (Note: in LHV, this is always set)
#   fast: equivalent to --no-rt-bss --no-bl-hash (For running XMHF quickly)
#   release: equivalent to --drt --dmap --no-dbg (For GitHub actions)
#   debug: ignored (For GitHub actions)
#   O0: ignored (For GitHub actions)
#   O3: enable -O3 optimization, etc. (For GitHub actions)
#   circleci: enable --enable-optimize-nested-virt workaround for Circle CI
#   -n: print autogen.sh and configure options, do not run them

set -e

# Information about compiler's platform
LINUX_BASE=""					# DEB or RPM
LINUX_BIT=$(getconf LONG_BIT)	# 32 or 64

# Information about XMHF
#APPROOT="hypapps/trustvisor"
APPROOT="hypapps/helloworld"
SUBARCH=""
DRT="n"
DMAP="n"
QEMU="y"
#UCODE="y"
UCODE="n"
AMD64MEM="0x140000000"
DRY_RUN="n"
CIRCLE_CI="n"
NO_X2APIC="n"
NO_RT_BSS="y"	# Always set in LHV
NO_BL_HASH="y"	# Always set in LHV
OPT=""
LHV_OPT="0"

# Determine LINUX_BASE (may not be 100% correct all the time)
if [ -f "/etc/debian_version" ]; then
	# DEB-based Linux (e.g. Debian, Ubuntu)
	LINUX_BASE="DEB"
else if [ -f "/etc/redhat-release" ]; then
	# RPM-based Linux (e.g. Fedora)
	LINUX_BASE="RPM"
else
	echo 'Error: build.sh does not know about OS it is running on.'; exit 1
fi; fi

# Check LINUX_BIT
case "$LINUX_BIT" in
	32)
		;;
	64)
		;;
	*)
		echo 'Error: unknown result from `getconf LONG_BIT`'; exit 1
		;;
esac

# Determine SUBARCH
case "$1" in
	i386)
		SUBARCH="i386"
		;;
	amd64)
		SUBARCH="amd64"
		;;
	*)
		echo 'Error: subarch incorrect, should be i386 or amd64'; exit 1
		;;
esac
shift

# Process other arguments
while [ "$#" -gt 0 ]; do
	case "$1" in
		--drt)
			DRT="y"
			;;
		--dmap)
			DMAP="y"
			;;
		--no-dbg)
			QEMU="n"
			;;
		--no-ucode)
			UCODE="n"
			;;
		--app)
			APPROOT="$2"
			shift
			;;
		--mem)
			AMD64MEM="$2"
			shift
			;;
		--lhv-opt)
			LHV_OPT="$2"
			shift
			;;
		--no-x2apic)
			NO_X2APIC="y"
			;;
		--no-rt-bss)
			NO_RT_BSS="y"
			;;
		--no-bl-hash)
			NO_BL_HASH="y"
			;;
		fast)
			NO_RT_BSS="y"
			NO_BL_HASH="y"
			;;
		release)
			# For GitHub actions
			DRT="y"
			DMAP="y"
			QEMU="n"
			;;
		debug)
			# For GitHub actions
			;;
		O0)
			# For GitHub actions
			;;
		O3)
			# For GitHub actions
			OPT="O3"
			;;
		circleci)
			CIRCLE_CI="y"
			;;
		-n)
			DRY_RUN="y"
			;;
		*)
			echo "Error: unknown argument $1"; exit 1
			;;
	esac
	shift 
done

# Build configure arguments
CONF=()
CONF+=("--with-approot=$APPROOT")
CONF+=("--enable-debug-symbols")

if [ "$SUBARCH" == "i386" ]; then
	# Building i386 XMHF
	if [ "$LINUX_BASE" == "DEB" -a "$LINUX_BIT" == "64" ]; then
		# Building on amd64 Debian
		CONF+=("CC=i686-linux-gnu-gcc")
		CONF+=("LD=i686-linux-gnu-ld")
	fi
else if [ "$SUBARCH" == "amd64" ]; then
	# Building amd64 XMHF
	CONF+=("--with-target-subarch=amd64")
	CONF+=("--with-amd64-max-phys-addr=$AMD64MEM")
	if [ "$LINUX_BASE" == "DEB" -a "$LINUX_BIT" == "32" ]; then
		# Building on i386 Debian
		CONF+=("CC=x86_64-linux-gnu-gcc")
		CONF+=("LD=x86_64-linux-gnu-ld")
	fi
else
	echo 'Error: unexpected $SUBARCH'; exit 1
fi; fi

if [ "$DRT" == "n" ]; then
	CONF+=("--disable-drt")
fi

if [ "$DMAP" == "n" ]; then
	CONF+=("--disable-dmap")
fi

if [ "$QEMU" == "y" ]; then
	CONF+=("--enable-debug-qemu")
fi

if [ "$UCODE" == "y" ]; then
	CONF+=("--enable-update-intel-ucode")
fi

if [ "$OPT" == "O3" ]; then
	# -Wno-stringop-overflow is set due to a compile bug in GCC 10 / GCC 11
	# Reported in https://gcc.gnu.org/bugzilla/show_bug.cgi?id=105100
	CONF+=("--with-opt=-O3 -Wno-stringop-overflow")
fi

if [ "$NO_X2APIC" == "y" ]; then
	CONF+=("--enable-hide-x2apic")
fi

if [ "$NO_RT_BSS" == "y" ]; then
	CONF+=("--enable-skip-runtime-bss")
fi

if [ "$NO_BL_HASH" == "y" ]; then
	CONF+=("--enable-skip-bootloader-hash")
fi

if [ "$CIRCLE_CI" == "y" ]; then
	CONF+=("--enable-optimize-nested-virt")
fi

CONF+=("--with-lhv-opt=$LHV_OPT")

# Output configure arguments, if `-n`
if [ "$DRY_RUN" == "y" ]; then
	set +x
	echo $'\n'"./autogen.sh; ./configure ${CONF[@]@Q}"$'\n'
	exit 0
fi

# Build
set -x
./autogen.sh
./configure "${CONF[@]}"
make -j "$(nproc)"

