import re, sys, bisect
from subprocess import check_output

def read_symbols(exe):
	ans = []
	for line in check_output(['nm', exe]).decode().split('\n'):
		if not line:
			continue
		addr, name = re.fullmatch('([0-9a-f]{16}) \w ([\w\.]+)', line).groups()
		addr = int(addr, 16)
		ans.append((addr, 0, name))
	ans.sort()
	return ans

def lookup_symbol(symbols, addr):
	'''
		Return print_str, func_name, func_offset.
		e.g. ('<main+10>', 'main', 10)
	'''
	index = bisect.bisect(symbols, (addr, 1))
	if index > 0:
		offset = addr - symbols[index - 1][0]
		if offset < 0x10000000:
			name = symbols[index - 1][2]
			return '<%s+%d>' % (name, offset), name, offset
	return '???', None, None

def process_cpu(symbols):
	ans = None
	valid_state = False
	while True:
		line = yield ans
		ans = line
		if line.startswith(': unhandled exception'):
			# Reset state
			valid_state = True
			has_error_code = False
			esp = None
			ebp = None
			frame_no = 0
			intercept_esp = None
			continue
		if not valid_state:
			continue
		if line.startswith(': error code:'):
			has_error_code = True
			continue
		if 'EBP=' in line:
			ebp, = re.search('EBP=(0x[0-9a-f]+)', line).groups()
			ebp = int(ebp, 16)
			old_ebp = ebp
		if line.startswith('  Stack('):
			matched = re.fullmatch('Stack\((0x[0-9a-f]+)\) \-\> (0x[0-9a-f]+)',
									line.strip())
			addr, value = matched.groups()
			addr = int(addr, 16)
			value = int(value, 16)
			if esp is None:
				esp = addr # - has_error_code * 4
			offset = addr - esp
			exception_dict = {
				-4: 'Error code',
				0x04: 'CS', 0x8: 'EFLAGS', # 0xc: 'ESP', 0x10: 'SS',
			}
			if offset == 0:
				ans += ' EIP #0\t'
			elif offset in exception_dict:
				ans += ' %s\t' % exception_dict[offset]
			elif addr == ebp:
				ans += ' EBP #%d\t' % frame_no
				old_ebp = ebp
				ebp = value
				frame_no += 1
			elif addr == old_ebp + 4:
				func_ptn, func_name, func_offset = lookup_symbol(symbols, value)
				ans += ' EIP #%d\t' % frame_no
				if func_name == 'xmhf_parteventhub_arch_x86vmx_entry':
					intercept_esp = addr + 0x4
			elif intercept_esp is not None:
				regs = ['edi','esi','ebp','esp','ebx','edx','ecx','eax']
				ans += ' guest %s\t' % (regs[(addr - intercept_esp) // 0x4])
			else:
				ans += '\t\t'
			func_ptn, _, _ = lookup_symbol(symbols, value)
			ans += '%s' % func_ptn

def main():
	cpus = []
	symbols = read_symbols(sys.argv[2])
	# import pdb; pdb.set_trace()
	for line in open(sys.argv[1]).read().split('\n'):
		matched = re.match('\[(\d\d)\](.+)', line)
		if matched:
			cpu, content = matched.groups()
			cpu = int(cpu)
			while len(cpus) <= cpu:
				cpus.append(process_cpu(symbols))
				cpus[-1].send(None)
			new_content = cpus[cpu].send(content)
			print('[%02d]%s' % (cpu, new_content))
		else:
			print(line)

if __name__ == '__main__':
	main()

