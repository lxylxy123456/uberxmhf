import os
assert os.system('mkdir -p tmp') == 0

files = os.listdir('./tools/ci/boot/grub/i386-pc/')

def test(file_set):
	print(*file_set, file=open('mods.txt', 'w'))
	if os.system('python3 -u ./tools/ci/grub.py --subarch i386 --xmhf-bin . '
				'--work-dir ./tmp/ --boot-dir ./tools/ci/boot'
				' &> /dev/null'
				) != 0:
		return 'ERROR'
	if os.system('python3 -u ./tools/ci/test3.py --subarch i386 '
				'--work-dir ./tmp/ --skip-reset-qemu --no-display'
				' &> /dev/null'
				) == 0:
		return 'GOOD'
	else:
		return 'BAD'

print(test([]))
print(test(files))

