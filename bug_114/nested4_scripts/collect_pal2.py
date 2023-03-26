import sys
from collections import defaultdict

B = 'BEGIN_PAL'
E = 'END_PAL'

def read_file(i):
	a = open(i).read().split('\n')
	assert a.count(B) == 1
	assert a.count(E) == 1
	b = a[a.index(B) + 1: a.index(E)]
	c = {
		'file': i.split('/')[-1],
		'conf': b[0].split(':')[0],
		'data': [],
	}
	for i in b[2:]:
		i0, i1, i2 = i.split()
		c['data'].append((i0, int(i1), int(i2)))
	return c

def avg(*vs):
	return sum(vs) / len(vs)

def filter_records(records, conf, name):
	for f, c, n, v in records:
		if conf is not None and conf != c:
			continue
		if name is not None and name != n:
			continue
		yield v

def latex_table(header, data, file=sys.stdout):
	col_num = len(data[0])
	row_num = len(data)
	print(r'\begin{tabular}{ l%s }\hline' % (' | r' * (col_num - 1)), file=file)
	print(r'%s \\ \hline' % (' & '.join(header)), file=file)
	for i in data:
		print(r'%s \\' % (' & '.join(i)), file=file)
	print(r'\end{tabular}', file=file)

def main():
	records = []	# (file, conf, name, value)
	for i in sys.argv[1:]:
		c = read_file(i)
		for j0, j1, j2 in c['data']:
			records.append((c['file'], c['conf'], j0, j2 / j1))
			# print(*records[-1], sep='\t')
	confs = sorted(set(map(lambda x: x[1], records)))
	names = set(map(lambda x: x[2], records))
	alarms = filter_records(records, None, 'alarm')
	avg_alarms = avg(*alarms)
	if confs == ['u3', 'u6', 'u9']:
		header = ['Event', 'XMHF', '\\XMHF64', '\\XMHF64, O3']
	elif confs == ['1xt', '2xkt']:
		header = ['Event', '1x', '2xk']
	else:
		raise ValueError('Unknown confs %s' % repr(confs))
	table = []
	assert names == {'unmar', 'mar', 'reg', 'unreg', 'alarm'}
	for n1, n2 in [('reg', 'Registration'), ('mar', 'Invocation'),
					('unmar', 'Termination'), ('unreg', 'Unregistration')]:
		table.append(['PAL ' + n2])
		for c in confs:
			x = avg(*filter_records(records, c, n1))
			table[-1].append('%.1f' % (x / avg_alarms * 1000000))
	latex_table(header, table)

if __name__ == '__main__':
	main()

