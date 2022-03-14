import os, sys, re, pickle, tempfile
from subprocess import Popen, check_output

# CWD needs to be XMHF directory
assert(os.path.exists('CHANGELOG.md'))

commit_flag = False

def find_files():
	# Find files
	files = check_output(['find', '-name', '*.[Sch]']).decode().split('\n')
	noarch_files = []
	arch32_files = []
	arch64_files = []
	content_map = {}
	for i in sorted(filter(bool, files)):
		if 'x86_64' in i and not i.endswith('.lds.S'):
			arch64_files.append(i)
		elif 'x86' in i and \
			i != './xmhf/third-party/libtomcrypt/testprof/x86_prof.c' and \
			'xmhf-dmaprot' not in i and \
			not i.endswith('.lds.S'):
			arch32_files.append(i)
		else:
			noarch_files.append(i)
		content_map[i] = open(i).read()
	assert len(arch32_files) == len(arch64_files)
	return noarch_files, arch32_files, arch64_files, content_map

def phase_1(noarch_files, arch32_files, arch64_files, content_map):
	# Find symbols
	global commit_flag
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
		# Note: there are places where one name is substring of another, e.g.
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
	commit_flag = True

def strip_content(content):
	'''
	Remove spaces at end of line, remove extra new lines at end of file
	'''
	lines = content.split('\n')
	for index, i in enumerate(lines):
		lines[index] = i.rstrip()
	while lines and lines[-1] == '':
		assert lines.pop() == ''
	lines.append('')
	return '\n'.join(lines)

def call_diff(c32, c64, *args):
	f32 = tempfile.TemporaryFile()
	f64 = tempfile.TemporaryFile()
	f32.write(c32.encode())
	f64.write(c64.encode())
	f32.seek(0)
	f64.seek(0)
	n32 = f32.fileno()
	n64 = f64.fileno()
	d32, d64 = '/dev/fd/%d' % n32, '/dev/fd/%d' % n64
	p = Popen(['diff', d32, d64, *args], pass_fds=(n32, n64), stdout=-1)
	return p.communicate()[0].decode()

def merge_file(c32, c64, macro):
	'''
	Merge files using macro, return new content
	'''
	args = [
		'--old-group-format=START_64_CODE\n%<',
		'--new-group-format=START_32_CODE\n%>',
		'--unchanged-group-format=START_COMMON_CODE\n%=',
	]
	diff = call_diff(c64, c32, *args)
	state = 0
	ans = []
	def change_state(new_state):
		if state == new_state:
			return new_state
		if state == 32:
			if new_state == 32:
				raise Exception('Not changing state')
			elif new_state == 64:
				raise Exception('Not handled case')
			else: # new_state == 0
				ans.append('#endif /* __X86_64__ */')
		elif state == 64:
			if new_state == 32:
				ans.append('#else /* !__X86_64__ */')
			elif new_state == 64:
				raise Exception('Not changing state')
			else: # new_state == 0
				ans.append('#endif /* __X86_64__ */')
		else: # state == 0
			if new_state == 32:
				ans.append('#ifndef __X86_64__')
			elif new_state == 64:
				ans.append('#ifdef __X86_64__')
			else: # new_state == 0
				raise Exception('Not changing state')
		return new_state
	for i in diff.split('\n'):
		if i == 'START_32_CODE':
			state = change_state(32)
		elif i == 'START_64_CODE':
			state = change_state(64)
		elif i == 'START_COMMON_CODE':
			state = change_state(0)
		else:
			ans.append(i)
	state = change_state(0)
	return '\n'.join(ans)

def phase_2(noarch_files, arch32_files, arch64_files, content_map):
	'''
	Use __X86_64__ to make different files the same
	'''
	global commit_flag
	# Instead of using Python's library, let's try to use diffutils' diff
	MACRO = '__X86_64__'
	ALT_MACRO = '__XMHF_X86_64__'
	for i32, i64 in zip(arch32_files, arch64_files):
		c32 = strip_content(content_map[i32])
		c64 = strip_content(content_map[i64])
		assert ALT_MACRO not in c32
		assert ALT_MACRO not in c64
#		if '__X86_64__' in c32 or '__X86_64__' in c64:
#			print(i32, i64)
#		if '__XMHF_X86_64__' in c32 or '__XMHF_X86_64__' in c64:
#			print(i32, i64)
		diff = call_diff(c32, c64, '--color=always')
		if diff:
			if not 'list all files':
				print('diff', i32, i64)
			todo = [
				'./xmhf/src/xmhf-core/include/arch/x86/_cmdline.h'
				'./xmhf/src/xmhf-core/include/arch/x86/_configx86.h'
				'./xmhf/src/xmhf-core/include/arch/x86/_div64.h'
				'./xmhf/src/xmhf-core/include/arch/x86/_msr.h'
				'./xmhf/src/xmhf-core/include/arch/x86/_paging.h'
				'./xmhf/src/xmhf-core/include/arch/x86/_processor.h'
				'./xmhf/src/xmhf-core/include/arch/x86/_txt_config_regs.h'
				'./xmhf/src/xmhf-core/include/arch/x86/_vmx.h'
				'./xmhf/src/xmhf-core/include/arch/x86/xmhf-baseplatform-arch-x86.h'
				'./xmhf/src/xmhf-core/include/arch/x86/xmhf-tpm-arch-x86.h'
				'./xmhf/src/xmhf-core/xmhf-runtime/xmhf-baseplatform/arch/x86/bplt-x86-addressing.c'
				'./xmhf/src/xmhf-core/xmhf-runtime/xmhf-baseplatform/arch/x86/bplt-x86-smp.c'
				'./xmhf/src/xmhf-core/xmhf-runtime/xmhf-baseplatform/arch/x86/bplt-x86-smplock.S'
				'./xmhf/src/xmhf-core/xmhf-runtime/xmhf-baseplatform/arch/x86/bplt-x86-smptrampoline.S'
				'./xmhf/src/xmhf-core/xmhf-runtime/xmhf-baseplatform/arch/x86/svm/bplt-x86svm-smp.c'
				'./xmhf/src/xmhf-core/xmhf-runtime/xmhf-baseplatform/arch/x86/vmx/bplt-x86vmx-smp.c'
			]
			show = [
			]
			commit = [
			]
			done = [
				'./xmhf/src/xmhf-core/xmhf-runtime/xmhf-baseplatform/arch/x86/vmx/bplt-x86vmx-vmcs.c'
				'./xmhf/src/xmhf-core/xmhf-runtime/xmhf-baseplatform/arch/x86/vmx/bplt-x86vmx-data.c'
				'./xmhf/src/xmhf-core/xmhf-runtime/xmhf-baseplatform/arch/x86/vmx/bplt-x86vmx.c'
				'./xmhf/src/xmhf-core/xmhf-runtime/xmhf-eventhub/arch/x86/vmx/peh-x86-safemsr.c'
				'./xmhf/src/xmhf-core/xmhf-runtime/xmhf-eventhub/arch/x86/vmx/peh-x86vmx-main.c'
				'./xmhf/src/xmhf-core/xmhf-runtime/xmhf-memprot/arch/x86/vmx/memp-x86vmx-data.c'
				'./xmhf/src/xmhf-core/xmhf-runtime/xmhf-memprot/arch/x86/vmx/memp-x86vmx.c'
				'./xmhf/src/xmhf-core/xmhf-runtime/xmhf-partition/arch/x86/vmx/part-x86vmx.c'
				'./xmhf/src/xmhf-core/xmhf-runtime/xmhf-smpguest/arch/x86/smpg-x86.c'
				'./xmhf/src/xmhf-core/xmhf-runtime/xmhf-smpguest/arch/x86/vmx/smpg-x86vmx.c'
				'./xmhf/src/xmhf-core/xmhf-runtime/xmhf-startup/arch/x86/rntm-x86-data.c'
				'./xmhf/src/xmhf-core/xmhf-runtime/xmhf-xcphandler/arch/x86/xcph-x86.c'
				'./xmhf/src/xmhf-core/xmhf-secureloader/arch/x86/sl-x86.c',
			]
			keep = [
				'./xmhf/src/xmhf-core/xmhf-runtime/xmhf-eventhub/arch/x86/vmx/peh-x86vmx-entry.S'
				'./xmhf/src/xmhf-core/xmhf-runtime/xmhf-partition/arch/x86/vmx/part-x86vmx-sup.S'
				'./xmhf/src/xmhf-core/xmhf-runtime/xmhf-xcphandler/arch/x86/xcph-stubs.S'
				'./xmhf/src/xmhf-core/xmhf-secureloader/arch/x86/sl-x86-entry.S',
				'./xmhf/src/xmhf-core/xmhf-secureloader/arch/x86/sl-x86-sup.S',
			]
			if i32 in show or i64 in show:
				print('ge', i32, i64)
				print(diff)
			if i32 in commit or i64 in commit:
				c3264 = merge_file(c32, c64, MACRO)
				content_map[i32] = c3264
				content_map[i64] = c3264
				commit_flag = True
		# import pdb; pdb.set_trace()

def commit(content_map):
	# Commit
	for i in content_map:
		print('Commit', i)
		open(i, 'w').write(content_map[i])

def main():
	noarch_files, arch32_files, arch64_files, content_map = find_files()
	# phase_1(noarch_files, arch32_files, arch64_files, content_map)
	phase_2(noarch_files, arch32_files, arch64_files, content_map)
	if commit_flag:
		commit(content_map)
	# import pdb; pdb.set_trace()

if __name__ == '__main__':
	main()

