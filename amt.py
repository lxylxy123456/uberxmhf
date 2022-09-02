#!/usr/bin/python3
# Follow amtterm output and record to a file

import sys
import argparse
import threading
import queue
from subprocess import Popen

amt_sh_process = None

def connect_amt_sh(amt_sh):
	global amt_sh_process
	p = Popen([amt_sh], bufsize=0, stdin=-1, stdout=-1, stderr=-1,
			  errors='replace')
	amt_sh_process = p
	end_count = 0
	data_queue = queue.Queue()

	def output_reader(fileno, file, data_queue):
		while True:
			data = file.read(1)
			data_queue.put((fileno, data))
			if not data:
				break

	t1 = threading.Thread(target=output_reader, args=(1, p.stdout, data_queue),
						  daemon=True)
	t2 = threading.Thread(target=output_reader, args=(2, p.stderr, data_queue),
						  daemon=True)
	t1.start()
	t2.start()

	while end_count < 2:
		fileno, data = data_queue.get()
		if not data:
			end_count += 1
		else:
			for i in data:
				yield fileno, i

	t1.join()
	t2.join()
	p.kill()
	p.wait()
	amt_sh_process = None

def main():
	parser = argparse.ArgumentParser()
	parser.add_argument('amt_sh')
	parser.add_argument('out_name')
	args = parser.parse_args()
	out_file = open(args.out_name, 'w')
	cur_line_stdout = ''
	cur_line_stderr = ''
	reset_flag = False
	try:
		while True:
			for fd, data in connect_amt_sh(args.amt_sh):
				assert len(data) == 1
				if fd == 1:
					print(data, end='', flush=True)
					if data == '\0':
						continue
					elif data == '\n':
						cur_line_stdout = ''
					else:
						cur_line_stdout += data
					out_file.write(data)
					out_file.flush()
					if reset_flag and (cur_line_stdout ==
						'eXtensible Modular Hypervisor Framework' or
						cur_line_stdout == 'Lightweight Hypervisor'):
						reset_flag = False
						# Truncate current file
						out_file.truncate(0)
						out_file.close()
						out_file = open(args.out_name, 'w')
						out_file.write(cur_line_stdout)
				else:
					assert fd == 2
					print('\033[31m%s\033[0m' % data, end='', flush=True)
					if data == '\0':
						continue
					elif data == '\n':
						cur_line_stderr = ''
					else:
						cur_line_stderr += data
					if (cur_line_stderr == 'The system is powered on.' or
						cur_line_stderr == 'The system is powered off.'):
						reset_flag = True
						cur_line_stdout = ''
	finally:
		if amt_sh_process is not None:
			try:
				amt_sh_process.kill()
			except Exception:
				pass

if __name__ == '__main__':
	main()

