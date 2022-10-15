static void xxd(u32 start, u32 end) {
	if ((start & 0xf) != 0 || (end & 0xf) != 0) {
		HALT_ON_ERRORCOND(0);
		//printf("xxd assertion failed");
		//while (1) {
		//	asm volatile ("hlt");
		//}
	}
	for (u32 i = start; i < end; i += 0x10) {
		printf("XXD: %08x: ", i);
		for (u32 j = 0; j < 0x10; j++) {
			if (j & 1) {
				printf("%02x", (unsigned)*(unsigned char*)(uintptr_t)(i + j));
			} else {
				printf(" %02x", (unsigned)*(unsigned char*)(uintptr_t)(i + j));
			}
		}
		printf("\n");
	}
}

