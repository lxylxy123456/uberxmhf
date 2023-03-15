import sys
from collections import defaultdict

B = 'BEGIN_PAL'
E = 'END_PAL'

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
	data = defaultdict(list)
	for i in b[2:]:
		i0, i1 = i.split()
		data[i0.strip()].append(int(i1))
	for k, v in data.items():
		d.append(k)
		c.append(avg(*v))
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
	t = [['name', 'ru', 'rcu', 'c']]
	for k, v in aggregate.items():
		d = dict(zip(rs[4:], v))
		t.append([k, d['f_ru:'], d['f_rcu:'], d['f_c:']])
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
	print()
	print_csv(div_table(tt, '1x', ['2xk']))
	print()

if __name__ == '__main__':
	main()

