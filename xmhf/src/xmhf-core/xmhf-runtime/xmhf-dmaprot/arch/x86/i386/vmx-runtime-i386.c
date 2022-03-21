#include <xmhf.h>


// On 32bit machine, we always return 0 - 4G as the machine physical address range, no matter how many memory is installed
bool vmx_get_machine_paddr_range(spa_t* machine_base_spa, spa_t* machine_limit_spa)
{
    // Sanity checks
	if(!machine_base_spa || !machine_limit_spa)
		return false;

    *machine_base_spa = 0;
    *machine_limit_spa = ADDR_4GB;

    return true;
}