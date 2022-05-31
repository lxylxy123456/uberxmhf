import re

if __name__ == '__main__':
	a = open('xmhf/src/xmhf-core/include/arch/x86/_vmx.h').read().split('\n')
	b = open('xmhf/src/xmhf-core/xmhf-runtime/xmhf-baseplatform/arch/x86/vmx/'
			'bplt-x86vmx-data.c').read().split('\n')
	registered_fields = {}
	for i in b:
		if 'DECLARE_FIELD(0' in i:
			e, n = re.search('DECLARE_FIELD\((0x[0-9A-F]{4}), (\w+)\)',
								i).groups()
			registered_fields[n] = int(e, 16)
	declared_fields = {}
	for i in a:
		matched = re.fullmatch(
			'\s*(unsigned (?:int|long|long long)|u16|u32|u64|ulong_t)\s+'
			'(\w+);\s*', i)
		if matched:
			e, n = matched.groups()
			if n in registered_fields:
				encoding = registered_fields[n]
				expected_type = {0: 'u16', 2: 'u64', 4: 'u32',
								6: 'ulong_t'}[encoding >> 12]
				if expected_type == 'u64' and (
					n.endswith('_full') or n.endswith('_high')):
					assert n[:-5] in registered_fields
					expected_type = 'u32'
				if e != expected_type:
					print(n, e, expected_type)
#	print(registered_fields)

