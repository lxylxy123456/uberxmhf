# Measure bss size of all object files

from subprocess import check_output

sizes = []
for i in check_output(['find', '-type', 'f', '-name', '*.o']).decode().split():
	size = 0
	for j in check_output(['readelf', '-SW', i]).decode().split('\n'):
		if 'NOBITS' in j:
			size += int(j.split('NOBITS', 1)[1].split()[2], 16)
	if size:
		sizes.append((size, i))

sizes.sort()
for i, j in sizes:
	print(i, j, sep='\t')

