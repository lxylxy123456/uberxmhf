import os, sys, re, pickle
from subprocess import Popen, check_output

# CWD needs to be XMHF directory
assert(os.path.exists('CHANGELOG.md'))

def main():
	# Find files
	files = check_output(['find', '-name', '*.[Sch]']).decode().split('\n')
	noarch_files = []
	arch32_files = []
	arch64_files = []
	content_map = {}
	for i in sorted(filter(bool, files)):
		if 'x86_64' in i:
			arch64_files.append(i)
		elif 'x86' in i and \
			i != './xmhf/third-party/libtomcrypt/testprof/x86_prof.c':
			arch32_files.append(i)
		else:
			noarch_files.append(i)
		content_map[i] = open(i).read()
	assert len(arch32_files) == len(arch64_files)
	# Find symbols
	symbols64 = set()
	for i32, i64 in zip(arch32_files, arch64_files):
		assert(i32.replace('x86', 'x86_64') == i64)
		content64 = content_map[i64]
		found = re.findall('\w+x86_64\w+', content64)
		assert 'x86_64vmx' not in found
		symbols64.update(found)
	print('Number of symbols:', len(symbols64))
	for i in symbols64:
		assert i.count('x86_64') == 1
		# Note: there are places where one name is subset of another, e.g.
		#       'dbg_x86_64_uart_putc' and 'dbg_x86_64_uart_putc_bare'
	# Replace symbols
	for i in content_map:
		print('Replace', i)
		c = content_map[i]
		for i64 in symbols64:
			i32 = i64.replace('x86_64', 'x86')
			c, n = re.subn(r'\b%s\b' % re.escape(i64), i32, c)
			if i in arch32_files:
				if n != 0:
					print('Warning', i)
		content_map[i] = c
	# Commit
	for i in content_map:
		print('Commit', i)
		open(i, 'w').write(content_map[i])
	# import pdb; pdb.set_trace()

if __name__ == '__main__':
	main()

