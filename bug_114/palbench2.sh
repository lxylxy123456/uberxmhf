#!/bin/bash
# pal: test result
# Use script to keep full history

set -xe
set -o pipefail

RESD="$PWD"
TMPD="`mktemp -d`"
# cd pal-tmp/; rm *; script -c '../palbench.sh ../pal_bench32 "old XMHF i386 O0, Ubuntu 12.04 x86"' plog
# cd pal-tmp/; rm *; script -c '../palbench.sh ../pal_bench64 "XMHF64 i386 O, Ubuntu 12.04 x86"' plog
FLAGS=()

[ "$#" -ge 1 ]
PAL_BENCH="$1"
[ -f "$PAL_BENCH" ]
shift

echo "$@" | tee "$RESD/pal"
lsb_release -a 2> /dev/null | grep "^Description" | tee -a "$RESD/pal"

scale_cpu_max () {
	if [ ! -f /sys/devices/system/cpu/cpu0/cpufreq/scaling_min_freq ]; then
		return 0
	fi
	for i in /sys/devices/system/cpu/cpu*/cpufreq/; do
		cat "$i/scaling_max_freq" | sudo tee "$i/scaling_min_freq"
	done
	grep . /sys/devices/system/cpu/cpu*/cpufreq/scaling_m*_freq
	if [ ! -f /sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_cur_freq ]; then
		return 0
	fi
	sudo grep . /sys/devices/system/cpu/cpu*/cpufreq/cpuinfo_cur_freq
}

run_dummy () {
	"$PAL_BENCH" "$@"
	sleep 1
}

run_pal__ () {
	"$PAL_BENCH" "$@" | tee -a "$RESD/pal"
	sleep 1
}

main () {
	scale_cpu_max
	run_dummy 0 30
	for i in {1..5}; do
		run_pal__ 0 30
		run_pal__ 500 500
	done
	run_pal__ 0 30
}

main
