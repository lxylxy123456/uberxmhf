import re
from collections import defaultdict

def split_tabs(line):
	a, b, c = re.fullmatch('([^\t]+)(\t+)([^\t]+)', line).groups()
	n = len(a) + 1
	while n % 4:
		n += 1
	n += 4 * (len(b) - 1)
	searched = re.search(' \((bug_\d{3})\)$', c)
	d = None
	if searched:
		c = c[:searched.start()]
		d, = searched.groups()
	return (a, n, c, d)

def main():
	lines = list(filter(bool, open('commits.txt').read().split('\n')))
	bucket = {}
	fb = defaultdict(list)	# { features_or_bugs: (commit, bug_no) }
	for line in reversed(lines):
		commit, tab, category, bug_no = split_tabs(line)
		if category == 'V':
			assert tab in bucket
			category = bucket[tab]
		else:
			bucket[tab] = category
		fb[category].append((commit, bug_no))
	for k, v in fb.items():
		print('#### `%s`' % k)
		print('```')
		for i, j in v:
			if j is None:
				j = ''
			print(i + ' ' * (80 - len(i)) + j)
		print('```')
		print()

if __name__ == '__main__':
	main()

