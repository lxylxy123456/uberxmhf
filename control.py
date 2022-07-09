import os
import time
assert os.system('mkdir -p tmp') == 0

# make sure tmp/grub/c.img and tmp/pal_demo.img are present

files = os.listdir('./tools/ci/boot/grub/i386-pc/')

def test(file_set):
	print(*file_set, file=open('mods.txt', 'w'))
	if os.system('python3 -u ./tools/ci/grub.py --subarch i386 --xmhf-bin . '
				'--work-dir ./tmp/ --boot-dir ./tools/ci/boot'
				' > /dev/null 2> /dev/null'
				) != 0:
		return 'ERROR'
	if os.system('python3 -u ./tools/ci/test4.py --guest-subarch i386 '
				'--qemu-image /var/lib/jenkins/workspace/xmhf64-windows/'
				'cache/win7x86-j.qcow2'
				'--work-dir ./tmp/ --qemu-timeout 60 '
				'--no-display'
				' > /dev/null 2> /dev/null'
				) == 0:
		return 'GOOD'
	else:
		return 'BAD'

assert test([]) == 'BAD'
assert test(files) == 'GOOD'

for i in files:
	try:
		s = files.copy()
		s.remove(i)
		result = test(s)
		print('no', i, result, sep='\t')
		print('no', i, result, sep='\t', file=open('result.txt', 'a'))
	except:
		pass
	time.sleep(3)

