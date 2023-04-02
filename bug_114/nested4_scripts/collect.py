import sys
from collections import defaultdict

B = 'BEGIN_RESULT'
E = 'END_RESULT'

def read_file(i):
	a = open(i).read().split('\n')
	assert a.count(B) == 1
	assert a.count(E) == 1
	b = a[a.index(B) + 1: a.index(E)]
	c = []
	d = []
	c.append(i.split('/')[-1])
	d.append('file')
	c.append(b[0])
	d.append('type')
	c.append(b[1].split('\t', 2)[1])
	d.append(b[1].split('\t', 2)[0])
	c.append(b[0].split(':')[0])
	d.append('name')
	for i in b[2:]:
		i0, i1 = i.split('\t')
		d.append(i0.strip())
		c.append(int(i1))
	return d, c

def print_table(t):
	for i in range(len(t[0])):
		print(*map(lambda x: x[i], t), sep='\t')

def print_csv(t):
	# TODO: does not escape
	for i in range(len(t[0])):
		print(*map(lambda x: x[i], t), sep=',')

def avg(*vs):
	return sum(vs) / len(vs)

def avg_table(t):
	rs, cs = t[0], t[1:]
	data = defaultdict(list)	# name -> dicts
	for c in cs:
		d = dict(zip(rs, c))
		data[d['name']].append(c[4:])
	aggregate = {}
	for k, v in data.items():
		aggregate[k] = list(map(avg, *v))
	t = [['name', 'CPU', 'Memory', 'File Read', 'File Write']]
	for k, v in aggregate.items():
		d = dict(zip(rs[4:], v))
		t.append([k,
			d['cpu run --time=1000'],
			d['memory run --memory-total-size=0 --time=1000'],
			avg(
				d['iozone -s 8g -r 2048 read'],
				d['iozone -s 8g -r 4096 read'],
				d['iozone -s 8g -r 8192 read'],
			),
#			avg(
#				d['iozone -s 8g -r 2048 reread'],
#				d['iozone -s 8g -r 4096 reread'],
#				d['iozone -s 8g -r 8192 reread'],
#			),
			avg(
				d['iozone -s 8g -r 2048 write'],
				d['iozone -s 8g -r 4096 write'],
				d['iozone -s 8g -r 8192 write'],
			),
#			avg(
#				d['iozone -s 8g -r 2048 rewrite'],
#				d['iozone -s 8g -r 4096 rewrite'],
#				d['iozone -s 8g -r 8192 rewrite'],
#			),
		])
#	import pdb; pdb.set_trace()
	return t

def div_table(t, a, bs):
	rs, cs = t[0], t[1:]
	ans = []
	ans.append(rs)
	def find_index(x):
		for index, i in enumerate(cs):
			if i[0] == x:
				return index
		else:
			assert False
	a_index = find_index(a)
	for b in bs:
		b_index = find_index(b)
		ans.append([b])
		for i, j in zip(cs[b_index][1:], cs[a_index][1:]):
			ans[-1].append(i / j)
	return ans

def main():
	rs = None
	cs = []
	for i in sys.argv[1:]:
		r, c = read_file(i)
		if rs is None:
			rs = r
		else:
			assert rs == r
		assert len(c) == len(r)
		cs.append(c)
	assert rs is not None
	t = [rs, *cs]
	# print(rs, cs)
	print_table(t)
	tt = avg_table(t)
	print_table(tt)
	if tt[1][0] == '0':
		print()
		print_csv(div_table(tt, '0', ['1b', '1k', '1w', '1x']))
		print()
		print_csv(div_table(tt, '1k', ['2bk', '2kk', '2wk', '2xk']))
		print()
		print_csv(div_table(tt, '0', ['2bk', '2kk', '2wk', '2xk']))
		print()
	elif tt[1][0] == 'u0':
		print()
		print_csv(div_table(tt, 'u0', ['u3', 'u6', 'u9']))
		print()
	else:
		raise Exception('Unknown tt[1][0]: %s' % tt[1][0])

if __name__ == '__main__':
	main()

