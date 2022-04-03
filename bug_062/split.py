import os, sys

def my_open(file_name):
	assert not os.path.exists(file_name)
	return open(file_name, 'w')

SEP = '------------------------------------=-=-=-=-=--------------'
EXP = '[00]: unhandled exception 3 (0x3), halting!'
name = sys.argv[1]
a = open(name).read().split(SEP)
assert EXP in a[-1]
a[-1], err = a[-1].split(EXP, 1)
err = EXP + err
for index, i in enumerate(a):
	my_open('%s_%02d' % (name, index)).write(i)

my_open('%s_err' % (name)).write(err)
