#!/bin/bash
set -xe

HOST=hp.lxy

run_test () {
	if [ "$1" == 'x' ]; then
		ssh "$HOST" "sudo grub-editenv /boot/grub/grubenv set next_entry=XMHF-build64 next_entry_2=debian_light_2432"
	else
		ssh "$HOST" "sudo grub-editenv /boot/grub/grubenv set next_entry=debian_light_2048"
	fi
	ssh "$HOST" "sudo init 6" || true
	sleep 5m
	ssh "$HOST" sudo systemctl stop gdm
	sleep 1m
	if [ "$1" == 'x' ]; then
		ssh "$HOST" ./L1x.sh | tee "$(mktemp "$(date "+auto_%Y%m%d%H%M%S_XXXXXXX")")"
	else
		ssh "$HOST" ./L0.sh | tee "$(mktemp "$(date "+auto_%Y%m%d%H%M%S_XXXXXXX")")"
	fi
	sleep 10m
}

# TODO
for i in "$@"; do
	run_test "$i";
done

