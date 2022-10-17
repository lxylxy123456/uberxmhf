ld_gc_demo.o: ld_gc_demo.c
	gcc -c -fdata-sections -ffunction-sections -o $@ $<

ld_gc_demo.out: ld_gc_demo.lds ld_gc_demo.o
	ld --gc-sections -T $< -o $@ ld_gc_demo.out ld_gc_demo.o

.PHONY: clean
clean:
	rm -rf ld_gc_demo.o ld_gc_demo.out

