'''
Remove the __NESTED_VIRTUALIZATION__ macro from all files
'''

import os, re

STR_IT = '#ifdef __NESTED_VIRTUALIZATION__'
STR_ET = '#endif /* __NESTED_VIRTUALIZATION__ */'
STR_IF = '#ifndef __NESTED_VIRTUALIZATION__'
STR_EF = '#endif /* !__NESTED_VIRTUALIZATION__ */'

def replace_file(f):
	lines = []
	state = 0
	for i in open(f).read().split('\n'):
		if '__NESTED_VIRTUALIZATION__' in i:
			if i == STR_IT:		assert state == 0; state = 1
			elif i == STR_IF:	assert state == 0; state = 2
			elif i in (STR_ET, STR_EF):	assert state in (1, 2); state = 0
				# Workaround typos in endif
			elif i == STR_ET:	assert state == 1; state = 0
			elif i == STR_EF:	assert state == 2; state = 0
			else:				print('Unknown:', repr(i)); assert 0
		else:
			if state != 1:
				lines.append(i)
	open(f, 'w').write('\n'.join(lines))

def main():
	assert os.path.exists('CHANGELOG.md')
	assert os.path.exists('xmhf/src')
	assert os.system('LANG=C git status |'
					' grep "nothing to commit, working tree clean" > /dev/null'
					) == 0
	for p, d, f in os.walk('xmhf/src/xmhf-core/xmhf-runtime'):
		for i in f:
			if i.endswith('.h') or i.endswith('.c'):
				replace_file(os.path.join(p, i))

if __name__ == '__main__':
	main()

