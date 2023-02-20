'''
Read event logger output and print statistics. Sample usage:

tail -n +1 -F /tmp/amt | python3 el.py
'''

import re, yaml
from collections import defaultdict

def read_events():
	lines = defaultdict(list)
	while True:
		try:
			line = input().strip('\r')
		except EOFError:
			break
		matched = re.fullmatch('EL\[(\d+)\]: (.*)', line)
		if matched:
			cpu, content = matched.groups()
			cpu = int(cpu)
			if content == '---':
				yield cpu, yaml.load('\n'.join(lines[cpu]), yaml.Loader)
				del lines[cpu]
			else:
				lines[cpu].append(content)

if __name__ == '__main__':
	for _, i in read_events():
		timestamp = i['xmhf_dbg_log_event']['tsc']
		count_101 = 0
		count_201 = 0
		count_202 = 0
		count_ept02_full = 0
		count_ept02_miss = 0
		for ek, ev in i['xmhf_dbg_log_event']['events'].items():
			if ek in ('vmexit_cpuid', 'vmexit_rdmsr', 'vmexit_wrmsr',
					  'vmexit_xcph', 'vmexit_other'):
				count_101 += ev['count']
			elif ek in ('exception', 'inject_nmi'):
				pass
			elif ek == 'vmexit_201':
				count_201 += ev['count']
			elif ek == 'vmexit_202':
				count_202 += ev['count']
			elif ek == 'ept02_full':
				count_ept02_full += ev['count']
			elif ek == 'ept02_miss':
				count_ept02_miss += ev['count']
			else:
				raise Exception('Unknown event: %s' % repr(ek))
		print(timestamp, count_101, count_201, count_202, count_ept02_full,
			  count_ept02_miss, sep='\t')

