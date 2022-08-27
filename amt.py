#!/usr/bin/python3
# Follow amtterm output and record to a file

import sys
import argparse
import select
from subprocess import Popen

amt_sh_process = None

def connect_amt_sh(amt_sh):
	global amt_sh_process
	p = Popen([amt_sh], bufsize=0, stdin=-1, stdout=-1, stderr=-1)
	amt_sh_process = p
	rlist = [p.stdout, p.stderr]
	while rlist:
		rl, wl, xl = select.select(rlist, [], [])
		assert rl and not wl and not xl
		for i in rl:
			data = i.read(1)
			# Check EOF
			if not data:
				rlist.remove(i)
				continue
			# Yield data to caller
			if i is p.stdout:
				yield 1, data
			else:
				assert i is p.stderr
				yield 2, data
	p.kill()
	p.wait()
	amt_sh_process = None

def main():
	parser = argparse.ArgumentParser()
	parser.add_argument('amt_sh')
	parser.add_argument('out_name')
	args = parser.parse_args()
	out_file = open(args.out_name, 'wb')
	cur_line_stdout = bytearray()
	cur_line_stderr = bytearray()
	reset_flag = False
	try:
		while True:
			for fd, data in connect_amt_sh(args.amt_sh):
				text = data.decode(errors='replace')
				assert len(data) == 1
				if fd == 1:
					print(text, end='', flush=True)
					if data == b'\0':
						continue
					elif data == b'\n':
						cur_line_stdout.clear()
					else:
						cur_line_stdout.append(data[0])
					out_file.write(data)
					out_file.flush()
					if reset_flag and (cur_line_stdout ==
						b'eXtensible Modular Hypervisor Framework' or
						cur_line_stdout == b'Lightweight Hypervisor'):
						reset_flag = False
						# Truncate current file
						out_file.truncate(0)
						out_file.close()
						out_file = open(args.out_name, 'wb')
						out_file.write(cur_line_stdout)
				else:
					assert fd == 2
					print('\033[31m%s\033[0m' % text, end='', flush=True)
					if data == b'\0':
						continue
					elif data == b'\n':
						cur_line_stderr.clear()
					else:
						cur_line_stderr.append(data[0])
					if (cur_line_stderr == b'The system is powered on.' or
						cur_line_stderr == b'The system is powered off.'):
						reset_flag = True
				continue

				if (cur_line == 'eXtensible Modular Hypervisor Framework' or
					cur_line == 'Lightweight Hypervisor'):
					# Truncate current file
					out_file.truncate(0)
					out_file.close()
					out_file = open(args.out_name, 'wb')
					out_file.write(cur_line)
				out_file.flush()
				print(c, end='', flush=True)
	finally:
		if amt_sh_process is not None:
			try:
				amt_sh_process.kill()
			except Exception:
				pass

if __name__ == '__main__':
	main()

