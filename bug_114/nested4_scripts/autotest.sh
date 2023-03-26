#!/bin/bash
set -xe
set -o pipefail

SCRIPT_DIR="$(dirname "$(realpath $BASH_SOURCE)")"
HOST="hp.lxy"
# TODO
XMHF_BUILD="XMHF-build64"

# pal_bench64L2 is not committed to git. If this fails, compile pal_bench64L2.
# Otherwise, remove pal_bench64L2 from the scp command below.
[ -f "$SCRIPT_DIR/pal_bench64L2" ]

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
			"cd VirtualBox\ VMs/$2; nohup sudo /home/dev/qemu/bios-qemu.sh --q35 -d $2.vmdk +1 --ssh 1121 -display none $3 > /dev/null 2> /dev/null &"
	else
		timeout 1m sshpass -p dev ssh -p "$1" "$HOST" \
			"nohup sudo ./bios-qemu.sh --q35 -d $2.vmdk +1 --ssh 1122 -display none $3 > /dev/null 2> /dev/null &"
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
		u0)
			edit_grub
			;;
		u3|u3t)
			edit_grub XMHF-compare
			;;
		u6|u6t)
			edit_grub XMHF64-compare0
			;;
		u9|u9t)
			edit_grub XMHF64-compare3
			;;
		0)
			# 2048
			edit_grub debian_light_2048
			;;
		1x|1xt)
			# 2048 + XMHF
			edit_grub "$XMHF_BUILD" debian_light_2432
			# TODO: edit_grub "$XMHF_BUILD" debian_light_2944
			;;
		1b|1k|1w)
			# 3072
			edit_grub debian_light_3072
			;;
		2xk|2xkt)
			# 3072 + XMHF
			edit_grub "$XMHF_BUILD" debian_light_4480
			# TODO: edit_grub "$XMHF_BUILD" debian_light_3968
			;;
		2bk|2kk|2wk)
			# 4096
			edit_grub debian_light_4992
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
	case "$CONF" in
		u0|u3|u6|u9|u3t|u6t|u9t)
			;;
		*)
			timeout 1m ssh "$HOST" "sudo systemctl stop gdm"
			sleep 1m
			;;
	esac

	# Start VM
	case "$CONF" in
		u0|u3|u6|u9|u3t|u6t|u9t)
			BENCH_PORT="22"
			;;
		0|1x|1xt)
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
		1k|2xk|2xkt)
			start_k 22 debian_light "-m 2048M"
			BENCH_PORT="1121"
			;;
		2kk)
			start_k 22 debian_light_2 "-m 3072M --fwd 1122 1122"
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
	timeout 5m sshpass -p dev scp -P "$BENCH_PORT" \
		"$SCRIPT_DIR/$CONF.sh" \
		"$SCRIPT_DIR/../palbench"*".sh" \
		"$SCRIPT_DIR/../sysbench"*".sh" \
		"$SCRIPT_DIR/pal_bench64L2" \
		"$SCRIPT_DIR/iozone3_489-1_amd64.deb" \
		"$HOST:"
	sleep 1m

	timeout 5m sshpass -p dev ssh -p "$BENCH_PORT" "$HOST" \
		"[ -f no-ins-iozone ] || sudo apt-get install ./iozone3_489-1_amd64.deb"
	sleep 1m

	timeout 2h sshpass -p dev ssh -p "$BENCH_PORT" "$HOST" "./$CONF.sh" | \
		tee "$(mktemp "$(date "+auto_%Y%m%d%H%M%S_${CONF}_XXXXXXX")")"

	# Stop VM
	case "$CONF" in
		u0|u3|u6|u9|u3t|u6t|u9t)
			;;
		0|1x|1xt)
			;;
		1b|1k|1w|2xk|2xkt)
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

