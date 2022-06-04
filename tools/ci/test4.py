'''
	Test XMHF using QEMU, but do not use SSH
'''

from subprocess import Popen, check_call
import argparse, subprocess, time, os, random, socket, threading

println_lock = threading.Lock()

def parse_args():
	parser = argparse.ArgumentParser()
	parser.add_argument('--subarch', required=True)
	parser.add_argument('--qemu-image', required=True)
	parser.add_argument('--smp', type=int, default=2)
	parser.add_argument('--work-dir', required=True)
	parser.add_argument('--windows-dir', default='tools/ci/windows/')
	parser.add_argument('--no-display', action='store_true')
	parser.add_argument('--verbose', action='store_true')
	parser.add_argument('--watch-serial', action='store_true')
	args = parser.parse_args()
	return args

def println(*args):
	with println_lock:
		print('{', *args, '}')

def link_qemu(args):
	assert os.path.exists(args.qemu_image)
	tmp_image = os.path.join(args.work_dir, 'windows.qcow2')
	check_call(['rm', '-f', tmp_image])
	check_call(['qemu-img', 'create', '-f', 'qcow2', '-b', args.qemu_image,
				'-F', 'qcow2', tmp_image])
	return tmp_image

def spawn_qemu(args, xmhf_img, windows_image, serial_file):
	bios_bin = os.path.join(args.windows_dir, 'bios.bin')
	windows_grub_img = os.path.join(args.windows_dir, 'grub_windows.img')
	pal_demo_img = os.path.join(args.work_dir, 'pal_demo.img')
	qemu_args = [
		'qemu-system-x86_64', '-m', '512M',
		'--drive', 'media=disk,file=%s,format=raw,index=0' % xmhf_img,
		'--drive', 'media=disk,file=%s,format=raw,index=1' % windows_grub_img,
		'--drive', 'media=disk,file=%s,format=qcow2,index=2' % windows_image,
		'--drive', 'media=disk,file=%s,format=raw,index=3' % pal_demo_img,
		'--bios', bios_bin,
		'-smp', str(args.smp), '-cpu', 'Haswell,vmx=yes', '--enable-kvm',
		'-serial', 'file:%s' % serial_file,
	]
	if args.no_display:
		qemu_args.append('-display')
		qemu_args.append('none')
	popen_stderr = { 'stderr': -1 }
	if args.verbose:
		del popen_stderr['stderr']
		print(' '.join(qemu_args))
	p = Popen(qemu_args, stdin=-1, stdout=-1, **popen_stderr)
	return p

def main():
	args = parse_args()
	windows_image = link_qemu(args)
	serial_file = os.path.join(args.work_dir, 'serial')
	xmhf_img = os.path.join(args.work_dir, 'grub/c.img')
	p = spawn_qemu(args, xmhf_img, windows_image, serial_file)

	# Simple workaround to watch serial output
	if args.watch_serial:
		threading.Thread(target=os.system, args=('tail -F %s' % serial_file,),
						daemon=True).start()

	try:
		# TODO: read and analyze serial output
		time.sleep(60)
	finally:
		p.kill()
		p.wait()

	# Test serial output
	println('Test XMHF banner in serial')
	check_call(['grep', 'eXtensible Modular Hypervisor', serial_file])
	println('Test E820 in serial')
	check_call(['grep', 'e820', serial_file])

	println('TEST PASSED')
	return 0

if __name__ == '__main__':
	exit(main())

