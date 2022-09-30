# Measure bss size of all object files

from subprocess import check_output
import os

files = [
	# 'xmhf-runtime/xmhf-debug/lib.a',
	'xmhf-runtime/xmhf-tpm/tpm-interface.o',
	'xmhf-runtime/xmhf-tpm/arch/x86/tpm-x86.o',
	'xmhf-runtime/xmhf-tpm/arch/x86/svm/tpm-x86svm.o',
	'xmhf-runtime/xmhf-tpm/arch/x86/vmx/tpm-x86vmx.o',
	'xmhf-runtime/xmhf-dmaprot/dmap-interface-earlyinit.o',
	'xmhf-runtime/xmhf-dmaprot/arch/x86/dmap-x86-earlyinit.o',
	'xmhf-runtime/xmhf-dmaprot/arch/x86/svm/dmap-svm.o',
	'xmhf-runtime/xmhf-dmaprot/arch/x86/vmx/dmap-vmx-earlyinit.o',
	'xmhf-runtime/xmhf-dmaprot/arch/x86/vmx/dmap-vmx-internal-common.o',
	'xmhf-runtime/xmhf-dmaprot/arch/x86/vmx/dmap-vmx-internal.o',
	'xmhf-runtime/xmhf-baseplatform/bplt-interface.o',
	'xmhf-runtime/xmhf-baseplatform/arch/x86/bplt-x86.o',
	'xmhf-runtime/xmhf-baseplatform/arch/x86/bplt-x86-pci.o',
	'xmhf-runtime/xmhf-baseplatform/arch/x86/bplt-x86-acpi.o',
	'xmhf-runtime/xmhf-baseplatform/arch/x86/bplt-x86-amd64-smplock.o',
	'xmhf-runtime/xmhf-baseplatform/arch/x86/bplt-x86-addressing.o',
	'xmhf-runtime/xmhf-baseplatform/arch/x86/bplt-x86-cpu.o',
	'xmhf-runtime/xmhf-baseplatform/arch/x86/vmx/bplt-x86vmx.o',
	'xmhf-runtime/xmhf-baseplatform/arch/x86/vmx/bplt-x86vmx-mtrrs.o',
]

for i in files:
	p = os.path.join('xmhf/src/xmhf-core', i)
	if os.path.exists(p):
		size = 0
		for j in check_output(['readelf', '-SW', p]).decode().split('\n'):
			if 'PROGBITS' in j and '.text' in j:
				size += int(j.split('PROGBITS', 1)[1].split()[2], 16)
		print(size, p, sep='\t')

