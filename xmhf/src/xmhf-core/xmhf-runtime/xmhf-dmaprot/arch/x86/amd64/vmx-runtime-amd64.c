#include <xmhf.h>

// On 64-bit machine, the function queries the E820 map for the used memory region.
bool vmx_get_machine_paddr_range(spa_t* machine_base_spa, spa_t* machine_limit_spa)
{
    bool status2 = false;

    // Sanity checks
	if(!machine_base_spa || !machine_limit_spa)
		return false;

    // Get the base and limit used system physical address from the E820 map
    status2 = xmhf_baseplatform_x86_e820_paddr_range(machine_base_spa, machine_limit_spa);
    if (!status2)
    {
        printf("\n%s: Get system physical address range error! Halting!", __FUNCTION__);
        return false;
    }

    // 4K-align the return the address
    *machine_base_spa = PAGE_ALIGN_4K(*machine_base_spa);
    *machine_limit_spa = PAGE_ALIGN_UP4K(*machine_limit_spa);

    return true;
}