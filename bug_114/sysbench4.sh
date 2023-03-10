#!/bin/bash
# result: test result
# Use script to keep full history

set -xe
set -o pipefail

RESD="$PWD"
TMPD="`mktemp -d`"
pushd "$TMPD"
FLAGS=()
FILEIO_FLAGS=("--validate")

echo "$@" | tee "$RESD/result"
lsb_release -a 2> /dev/null | grep "^Description" | tee -a "$RESD/result"

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

run_dummy___ () {
	sysbench "${FLAGS[@]}" "$@"
	sleep 1
}

grep_events () {
	grep 'total number of events' "$@" | cut -d : -f 2- | grep -oE '[^ ].*'
}

run_sysbench () {
	sysbench "${FLAGS[@]}" "$@" | tee out
	printf '%-40s\t' "`echo "$@"`" | tee -a "$RESD/result"
	echo
	grep_events out | tee -a "$RESD/result"
	sleep 1
}

main () {
	scale_cpu_max
	run_dummy___ cpu run
	run_sysbench cpu run
	run_dummy___ memory run --memory-total-size=0
	run_sysbench memory run --memory-total-size=0
	run_dummy___ fileio "${FILEIO_FLAGS[@]}" prepare
	run_dummy___ fileio "${FILEIO_FLAGS[@]}" run --file-test-mode=seqrd
	run_sysbench fileio "${FILEIO_FLAGS[@]}" run --file-test-mode=seqrd
	run_sysbench fileio "${FILEIO_FLAGS[@]}" run --file-test-mode=seqwr
	run_dummy___ fileio "${FILEIO_FLAGS[@]}" cleanup
	#run_dummy___ fileio "${FILEIO_FLAGS[@]}" prepare
	#run_sysbench fileio "${FILEIO_FLAGS[@]}" run --file-test-mode=seqrewr
	#run_dummy___ fileio "${FILEIO_FLAGS[@]}" cleanup
	#iozone -a -s 4g -i 0 -i 1
}

main
