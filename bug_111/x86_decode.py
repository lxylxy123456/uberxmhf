'''
gcc -m32 -c a.s -o a.32.o
gcc -c a.s -o a.64.o
objdump -d a.32.o
objdump -d a.64.o
'''

def decode(l, n):
	print('decode_%d_0x%x:' % (l, n))
	for i in range(l):
		print('\t.byte 0x%02x' % ((n >> (8 * i)) & 0xff))

if __name__ == '__main__':
	decode(2, 0x8b)
	decode(6, 0xc450000c7)
	decode(6, 0xc461000c7)
	decode(2, 0x1089)
	decode(6, 0xff5fd000878b)
	decode(6, 0xff5fd000b789)
	decode(7, 0xff5fd30025048b)
	decode(8, 0xff5fd30025048b40)
	decode(8, 0xff5fd30025048b41)

