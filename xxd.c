static void xxd(uintptr_t start, uintptr_t end) {
	if ((start & 0xf) != 0 || (end & 0xf) != 0) {
		HALT_ON_ERRORCOND(0);
		//printf("xxd assertion failed");
		//while (1) {
		//	asm volatile ("hlt");
		//}
	}
	for (uintptr_t i = start; i < end; i += 0x10) {
		printf("XXD: %08lx:", i);
		for (uintptr_t j = 0; j < 0x10; j++) {
			if (j & 1) {
				printf("%02x", (unsigned)*(unsigned char*)(uintptr_t)(i + j));
			} else {
				printf(" %02x", (unsigned)*(unsigned char*)(uintptr_t)(i + j));
			}
		}
		printf("\n");
	}
}

