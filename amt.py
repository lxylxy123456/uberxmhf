#!/usr/bin/python3
# Follow amtterm output and record to a file

import sys
import argparse
from subprocess import Popen

def main():
	parser = argparse.ArgumentParser()
	parser.add_argument('amt_sh')
	parser.add_argument('out_name')
	args = parser.parse_args()
	out_file = open(args.out_name, 'w')
	cur_line = ''
	try:
		while True:
			p = Popen([args.amt_sh], stdin=-1, stdout=-1)
			while True:
				try:
					c = p.stdout.read(1).decode()
				except UnicodeDecodeError:
					continue
				if not c:
					break
				elif c == '\0':
					continue
				elif c == '\n':
					cur_line = ''
				else:
					cur_line += c
				out_file.write(c)
				if (cur_line == 'eXtensible Modular Hypervisor Framework' or
					cur_line == 'Lightweight Hypervisor'):
					# Truncate current file
					out_file.truncate(0)
					out_file.close()
					out_file = open(args.out_name, 'w')
					out_file.write(cur_line)
				out_file.flush()
				print(c, end='', flush=True)
			p.kill()
			p.wait()
	finally:
		try:
			p.kill()
		except Exception:
			pass

if __name__ == '__main__':
	main()

