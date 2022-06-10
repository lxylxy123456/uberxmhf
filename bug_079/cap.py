import re
from itertools import zip_longest

def grouper(iterable, n, fillvalue=None) :
    "Collect data into fixed-length chunks or blocks"
    # grouper('ABCDEFG', 3, 'x') --> ABC DEF Gxx"
    args = [iter(iterable)] * n
    return zip_longest(*args, fillvalue=fillvalue)

if __name__ == '__main__':
	for i in grouper(open('cap.txt').read().split('\n'), 8):
		human_name, = re.fullmatch('/\* (.+) \*/', i[0]).groups()
		cap_name, num = re.fullmatch('#define VMX_(\w+) (\d+)', i[1]).groups()
		low_name, = re.fullmatch(
			'static inline bool _vmx_has_(\w+)\(VCPU \*vcpu\)', i[2]).groups()
		assert i[3] == '{'
		ctl_name, = re.fullmatch(
			'\treturn \(\(vcpu->vmx_([\w\[\]]+) >> 32\) &', i[4]).groups()
		cap_name2, = re.fullmatch(
			'\t\t\t\(1U \<\< VMX_(\w+)\)\);', i[5]).groups()
		assert i[6] == '}'
		assert i[7] == ''
		assert cap_name == cap_name2
		if ctl_name == 'msrs[INDEX_IA32_VMX_PROCBASED_CTLS2_MSR]':
			ctl_name = 'procbased_ctls2'

		# Export to cap.csv
		print(human_name, cap_name, num, low_name, ctl_name, sep=',')

