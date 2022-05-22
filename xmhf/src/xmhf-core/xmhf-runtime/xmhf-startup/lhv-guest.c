#include <xmhf.h>
#include <lhv.h>

void lhv_guest_main(ulong_t cpu_id) {
	while (1) {
		printf("%d", cpu_id);
	}
}
