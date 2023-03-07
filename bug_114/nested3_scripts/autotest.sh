#!/bin/bash
set -xe

SCRIPT_DIR="$(dirname "$(realpath $BASH_SOURCE)")"
HOST="hp.lxy"
# TODO
XMHF_BUILD="XMHF-build32"

edit_grub () {
	timeout 1m ssh "$HOST" \
		"sudo grub-editenv /boot/grub/grubenv set next_entry=$1 next_entry_2=$2"
}

# start_k 22 debian_light "-m 2048M"
# start_k 22 debian_light_2 "-m 2560M --fwd 1122 1122"
# start_k 1121 debian_light "-m 2048M"
start_k () {
	if [ "$1" == "22" ]; then
		timeout 1m ssh "$HOST" \
			"cd VirtualBox\ VMs/$2; nohup sudo /home/dev/qemu/bios-qemu.sh --q35 --drive-directsync $2.vmdk +1 --ssh 1121 -display none $3 > /dev/null 2> /dev/null &"
	else
		timeout 1m sshpass -p dev ssh -p "$1" "$HOST" \
			"nohup sudo ./bios-qemu.sh --q35 --drive-directsync $2.vmdk +1 --ssh 1122 -display none $3 > /dev/null 2> /dev/null &"
	fi
	sleep 5m
}

# start_b 22 debian_light
# start_b 22 debian_light_2
start_b () {
	timeout 1m sshpass -p dev ssh -p "$1" "$HOST" \
		"VBoxManage startvm $2 --type headless"
	sleep 5m
}

# start_w 22 debian_light
# start_w 22 debian_light_2
start_w () {
	timeout 1m sshpass -p dev ssh -p "$1" "$HOST" \
		"vmrun -T ws start /media/dev/Last/vmware/$2/$2.vmx nogui"
	sleep 5m
}

stop_v () {
	timeout 1m sshpass -p dev ssh -p "$1" "$HOST" \
		"nohup bash -c 'sleep 10; sudo init 0' > /dev/null 2> /dev/null &"
	sleep 1m
}

run_test () {
	CONF="$1"

	# Reset disk
	timeout 1m ssh "$HOST" "./reset_all.sh"

	# Configure GRUB
	case "$CONF" in
		0)
			edit_grub debian_light_2048
			;;
		1x)
			edit_grub "$XMHF_BUILD" debian_light_2432
			;;
		1b|1k|1w)
			edit_grub debian_light_2560
			;;
		2xk)
			edit_grub "$XMHF_BUILD" debian_light_2944
			;;
		2bk|2kk|2wk)
			edit_grub debian_light_3072
			;;
		*)
			false
			;;
	esac

	# Reboot
	timeout 1m ssh "$HOST" \
		"nohup bash -c 'sleep 10; sudo init 6' > /dev/null 2> /dev/null &"
	sleep 5m

	# Stop gdm
	timeout 1m ssh "$HOST" "sudo systemctl stop gdm"
	sleep 1m

	# Start VM
	case "$CONF" in
		0|1x)
			BENCH_PORT="22"
			;;
		1b)
			start_b 22 debian_light
			BENCH_PORT="1121"
			;;
		2bk)
			start_b 22 debian_light_2
			start_k 1121 debian_light "-m 2048M"
			BENCH_PORT="1122"
			;;
		1k|2xk)
			start_k 22 debian_light "-m 2048M"
			BENCH_PORT="1121"
			;;
		2kk)
			start_k 22 debian_light_2 "-m 2560M --fwd 1122 1122"
			start_k 1121 debian_light "-m 2048M"
			BENCH_PORT="1122"
			;;
		1w)
			start_w 22 debian_light
			BENCH_PORT="1221"
			;;
		2wk)
			start_w 22 debian_light_2
			start_k 2221 debian_light "-m 2048M"
			BENCH_PORT="2222"
			;;
		*)
			false
			;;
	esac

	# TODO
	timeout 5m sshpass -p dev scp -P "$BENCH_PORT" "$SCRIPT_DIR/0.sh" "$HOST:"
	sleep 1m

	timeout 1h sshpass -p dev ssh -p "$BENCH_PORT" "$HOST" "./$CONF.sh" | \
		tee "$(mktemp "$(date "+auto_%Y%m%d%H%M%S_${CONF}_XXXXXXX")")"

	# Stop VM
	case "$CONF" in
		0|1x)
			;;
		1b|1k|1w|2xk)
			stop_v "$BENCH_PORT"
			;;
		2bk|2kk)
			stop_v 1122
			stop_v 1121
			;;
		2wk)
			stop_v 2222
			stop_v 2221
			;;
		*)
			false
			;;
	esac

	sleep 10m
}

for i in "$@"; do
	run_test "$i";
done

