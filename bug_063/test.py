from subprocess import Popen, check_call
import argparse, subprocess, time, os, random, socket, threading

TEST_STAT_BOOTING = 0
TEST_STAT_STARTED = 1
TEST_STAT_STOPPED = 2

SSH_TIMEOUT = 2
BOOT_TIMEOUT = 150
TEST_TIMEOUT = 30
HALT_TIMEOUT = 10

def parse_args():
	parser = argparse.ArgumentParser()
	parser.add_argument('--subarch', required=True)
	parser.add_argument('--xmhf-bin', help='If non-zero, update XMHF binary')
	parser.add_argument('--qemu-image', required=True)
	parser.add_argument('--nbd', type=int, required=True)
	parser.add_argument('--smp', type=int, default=2)
	parser.add_argument('--work-dir', required=True)
	parser.add_argument('--no-display', action='store_true')
	parser.add_argument('--sshpass', help='Password for ssh')
	args = parser.parse_args()
	return args

def copy_bin(args, src, dst):
	'''
	Copy XMHF files from src to dst
	If src is a directory, src/init-x86-subarch.bin etc should exist
	If src is a file, not implemented (intended to be GitHub Action's output)
	dst/boot should exist
	'''
	if not os.path.isdir(src):
		raise NotImplementedError
	assert os.path.isdir(dst)
	files = [
		'init-x86-%s.bin' % args.subarch,
		'hypervisor-x86-%s.bin.gz' % args.subarch,
	]
	for i in files:
		assert os.path.exists(os.path.join(src, i))
		assert os.path.exists(os.path.join(dst, i))
	for i in files:
		check_call(['sudo', 'cp', os.path.join(src, i), dst])

def update_grub(args):
	'''
	Mount QEMU disk, update XMHF files and GRUB
	'''
	check_call('sudo modprobe nbd max_part=8'.split())
	dev_nbd = '/dev/nbd%d' % args.nbd
	check_call(['sudo', 'qemu-nbd', '--connect=' + dev_nbd, args.qemu_image])
	try:
		dir_mnt = os.path.join(args.work_dir, 'mnt')
		check_call(['mkdir', '-p', dir_mnt])
		check_call(['sudo', 'fdisk', dev_nbd, '-l'])
		check_call(['sudo', 'mount', dev_nbd + 'p1', dir_mnt])
		try:
			if args.xmhf_bin:
				copy_bin(args, args.xmhf_bin, os.path.join(dir_mnt, 'boot'))
			check_call(['sudo', 'chroot', dir_mnt, 'grub-editenv',
						'/boot/grub/grubenv', 'set',
						'next_entry=XMHF-%s' % args.subarch])
			check_call(['sync'])
		finally:
			for i in range(5):
				try:
					check_call(['sudo', 'umount', dir_mnt])
				except subprocess.CalledProcessError:
					time.sleep(1)
					continue
				break
			else:
				raise Exception('Cannot umounts')
	finally:
		check_call(['sudo', 'qemu-nbd', '--disconnect', dev_nbd])

def get_port():
	s = socket.socket(socket.AF_INET, socket.SOCK_STREAM, 0)
	for i in range(10000):
		num = random.randrange(1024, 65536)
		try:
			s.bind(('127.0.0.1', num))
		except OSError:
			continue
		s.close()
		return num
	else:
		raise RuntimeError('Cannot get port')

def spawn_qemu(args, serial_file):
	qemu_args = [
		'qemu-system-x86_64', '-m', '512M',
		'--drive', 'media=disk,file=%s,index=2' % args.qemu_image,
		'-device', 'e1000,netdev=net0',
		'-netdev', 'user,id=net0,hostfwd=tcp::%d-:22' % ssh_port,
		'-smp', str(args.smp), '-cpu', 'Haswell,vmx=yes', '--enable-kvm',
		'-serial', 'file:%s' % serial_file,
	]
	if args.no_display:
		qemu_args.append('-display')
		qemu_args.append('none')
	p = Popen(qemu_args, stdin=-1, stdout=-1, stderr=-1)
	return p

def send_ssh(args, ssh_port, status):
	'''
	status[0] = lock
	status[1] = TEST_STAT_BOOTING (0): test not started yet
	status[1] = TEST_STAT_STARTED (1): test started
	status[1] = TEST_STAT_STOPPED (2): test finished
	status[2] = time of last line
	status[3] = test result
	'''
	wordsize = { 'i386': 32, 'amd64': 64 }[args.subarch]
	# First echo may not be captured by p.stdout for unknown reason
	bash_script = ('echo TEST_START; '
					'echo TEST_START; '
					'./test_args%d 7 7 7; '
					'echo TEST_END; '
					'sudo init 0; '
					'true') % wordsize
	ssh_cmd = []
	if args.sshpass:
		ssh_cmd += ['sshpass', '-p', args.sshpass]
	ssh_cmd += ['ssh', '-p', str(ssh_port),
				'-o', 'ConnectTimeout=%d' % SSH_TIMEOUT,
				'-o', 'StrictHostKeyChecking=no',
				'-o', 'UserKnownHostsFile=/dev/null', '127.0.0.1',
				'bash', '-c', bash_script]
	while True:
		p = Popen(ssh_cmd, stdin=-1, stdout=-1, stderr=-1)
		while True:
			line = p.stdout.readline().decode()
			if not line:
				p.wait()
				assert p.returncode is not None
				if p.returncode == 0:
					# Test pass, but should not use this branch
					with status[0]:
						status[1] = TEST_STAT_STOPPED
						return
				break
			print('ssh:', line.rstrip())
			ls = line.strip()
			if ls == 'TEST_START':
				with status[0]:
					status[1] = TEST_STAT_STARTED
			elif ls == 'TEST_END':
				with status[0]:
					# Test pass, expected way
					status[1] = TEST_STAT_STOPPED
					return
			else:
				if_pass = line.strip() == 'Test pass'
				with status[0]:
					status[1] = TEST_STAT_STARTED
					status[2] = time.time()
					if if_pass:
						status[3] = 'PASS'
		time.sleep(1)
		print('ssh: retry SSH')

if __name__ == '__main__':
	args = parse_args()
	update_grub(args)
	ssh_port = get_port()
	print('Use ssh port', ssh_port)
	p = spawn_qemu(args, os.path.join(args.work_dir, 'serial'))
	ss = [threading.Lock(), TEST_STAT_BOOTING, 0, 'FAIL']
	ts = threading.Thread(target=send_ssh, args=(args, ssh_port, ss),
							daemon=True)
	ts.start()

	start_time = time.time()
	test_stat = 'unknown'
	test_result = 'FAIL'
	while True:
		state = [None]
		with ss[0]:
			state[1:] = ss[1:]
		print('main: MET = %d;' % int(time.time() - start_time),
				'state =', state)
		if state[1] == TEST_STAT_BOOTING:
			# Give the OS BOOT_TIMEOUT seconds to boot
			if time.time() - start_time > BOOT_TIMEOUT:
				test_stat = 'aborted_boot'
				test_result = 'ABORTED'
				break
		elif state[1] == TEST_STAT_STARTED:
			# TEST_TIMEOUT seconds without new message, likely failed
			if time.time() - state[2] > TEST_TIMEOUT:
				test_stat = 'aborted_test'
				test_result = state[3]
				break
		elif state[1] == TEST_STAT_STOPPED:
			# test completed
			test_stat = 'completed'
			test_result = state[3]
			# give the OS 10 seconds to shutdown; if wait() succeeds, calling
			# wait() again will still succeed
			try:
				p.wait(timeout=HALT_TIMEOUT)
			except subprocess.TimeoutExpired:
				pass
			break
		else:
			raise ValueError
		time.sleep(1)

	p.kill()
	p.wait()

	print(test_stat, test_result)
	with open(os.path.join(args.work_dir, 'result'), 'w') as f:
		print(test_stat, test_result, file=f)

	if test_result == 'PASS':
		exit(0)
	else:
		exit(1)

