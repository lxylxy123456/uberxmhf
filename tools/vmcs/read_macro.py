#
# The following license and copyright notice applies to all content in
# this repository where some other license does not take precedence. In
# particular, notices in sub-project directories and individual source
# files take precedence over this file.
#
# See COPYING for more information.
#
# eXtensible, Modular Hypervisor Framework 64 (XMHF64)
# Copyright (c) 2023 Eric Li
# All Rights Reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#
# Redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in
# the documentation and/or other materials provided with the
# distribution.
#
# Neither the name of the copyright holder nor the names of
# its contributors may be used to endorse or promote products derived
# from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
# CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
# INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
# BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
# TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
# ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
# TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
# THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.
#

'''
Read VMCS macro definitions, perform sanity check, and output VMCS fields as
CSV.

Cd to XMHF's directory, then run "python3 tools/vmcs/read_macro.py". This
script will read VMCS01 definition  (_vmx_vmcs_fields.h) and VMCS02 definitions
(vmcs12-fields.h). The 2 definition files must match. Then this file prints
the summary of VMCS definitions using CSV format to stdout.
'''

import sys, re, csv

F01 = 'xmhf/src/xmhf-core/include/arch/x86/_vmx_vmcs_fields.h'
F12 = ('xmhf/src/xmhf-core/xmhf-runtime/xmhf-nested/arch/x86/vmx/nested-x86vmx-'
		'vmcs12-fields.h')

def parse_macro(lines):
	# 'Virtual-processor identifier (VPID)'
	name = re.fullmatch('/\* (.+) \*/', lines[0]).groups()[0]
	# DECLARE_FIELD_16_RW(0x0000, control_vpid, (FIELD_PROP_CTRL), ..., UNUSED)
	macro = ' '.join(map(str.strip, lines[1:]))
	matched = re.fullmatch('DECLARE_FIELD_(16|64|32|NW)_(RO|RW)\((.+)\)', macro)
	# '16', 'RW', ...
	bits, write, args = matched.groups()
	# ['0x0000', 'control_vpid', ..., 'UNDEFINED']
	# TODO: currently ',' in macros is not supported
	lst = list(map(str.strip, args.split(',')))
	return name, bits, write, lst

def read_file(f):
	lines = open(f).read().split('\n')
	while len(lines) > 1:
		if lines[1].startswith('DECLARE_FIELD_'):
			end = 2
			while lines[end].strip() != 'UNDEFINED)':
				end += 1
			yield parse_macro(lines[:end + 1])
			lines = lines[end + 1:]
		else:
			lines.pop(0)

def read_files():
	for i, j in zip(read_file(F01), read_file(F12)):
		n1, b1, w1, l1 = i
		n2, b2, w2, l2 = j
		assert n1 == n2
		assert b1 == b2
		assert w1 == w2
		e1, c1, h1, un1 = l1
		e2, c2, p2, h2, u2, un2 = l2
		assert e1 == e2
		assert c1 == c2
		assert h1 == h2
		assert un1 == un2 and un1 == 'UNDEFINED'
		ps = []
		for k in re.fullmatch('\((.+)\)', p2).groups()[0].split(' | '):
			ps.append(re.fullmatch('FIELD_PROP_(\w+)', k).groups()[0])
		p = ', '.join(ps)
		yield n1, b1, w1, e1, c1, p, u2, h1

def main():
	w = csv.writer(sys.stdout)
	w.writerow(['SDM_name', 'bits', 'write', 'encoding', 'C_name', 'field_prop',
				'nested_special', 'exist'])
	for i in read_files():
		w.writerow(i)

if __name__ == '__main__':
	main()

