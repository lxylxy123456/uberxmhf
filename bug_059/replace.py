import os, sys, re, pickle, tempfile
from subprocess import Popen, check_output

# CWD needs to be XMHF directory
assert(os.path.exists('CHANGELOG.md'))

commit_flag = False

def find_files():
	# Find files
	files = check_output(['find', '-name', '*.[Sch]']).decode().split('\n')
	noarch_files = []
	content_map = {}
	for i in sorted(filter(bool, files)):
		noarch_files.append(i)
		content_map[i] = open(i).read()
	return noarch_files, content_map

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

def phase_1(noarch_files, content_map):
	global commit_flag
	AMD64_IF = '#ifdef __AMD64__'
	AMD64_ELSE = '#else /* !__AMD64__ */'
	AMD64_ENDIF = '#endif /* __AMD64__ */'
	OUT11 = '#ifdef __AMD64__'
	OUT12 = '#elif defined(__I386__)'
	OUT13A = '#else /* !defined(__I386__) && !defined(__AMD64__) */'
	OUT13B = '    #error "Unsupported Arch"'
	OUT13C = '#endif /* !defined(__I386__) && !defined(__AMD64__) */'
	OUT21 = '#ifdef __AMD64__'
	OUT22A = '#elif !defined(__I386__)'
	OUT22B = '    #error "Unsupported Arch"'
	OUT22C = '#endif /* !defined(__I386__) */'
	for i in content_map:
		lines = []
		state = []
		for line in content_map[i].split('\n'):
			if '__AMD64__' in line:
				assert line in (
					AMD64_IF, AMD64_ELSE, AMD64_ENDIF,
					# The following are ignored
					'// Ideally, should change __AMD64__ to __XMHF_AMD64__',
					'#elif defined(__AMD64__)')
				if line == AMD64_IF:
					assert len(state) == 0
					state.append([])
					continue
				elif line == AMD64_ELSE:
					assert len(state) == 1
					state.append([])
					continue
				elif line == AMD64_ENDIF:
					assert len(state) == 1 or len(state) == 2
					if len(state) == 2:
						# "In 1" and "Out 1" in README.md
						for j in [OUT11, *state[0], OUT12, *state[1], OUT13A,
									OUT13B, OUT13C]:
							lines.append(j)
					elif len(state) == 1:
						# "In 2" and "Out 2" in README.md
						for j in [OUT21, *state[0], OUT22A, OUT22B, OUT22C]:
							lines.append(j)
					else:
						print(state)
						assert 0
					state = []
					continue
			if state:
				state[-1].append(line)
			else:
				lines.append(line)
		assert len(state) == 0
		content_map[i] = '\n'.join(lines)
	commit_flag = True

def commit(content_map):
	# Commit
	for i in content_map:
		print('Commit', i)
		open(i, 'w').write(content_map[i])

def main():
	noarch_files, content_map = find_files()
	phase_1(noarch_files, content_map)
	if commit_flag:
		commit(content_map)
	# import pdb; pdb.set_trace()

if __name__ == '__main__':
	main()

