/*
 * SHV - Small HyperVisor for testing nested virtualization in hypervisors
 * Copyright (C) 2023  Eric Li
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef _LHV_VMCS_H_
#define _LHV_VMCS_H_

/*
a = open('./xmhf/src/xmhf-core/include/lhv-vmcs-template.h').read().split('\n')
import re
for i in a:
	if 'DECLARE_FIELD' in i:
		matched = re.fullmatch('    DECLARE_FIELD\((0x[0-9A-Fa-f]{4}), (\w+)\)',
								i)
		num, name = matched.groups()
		print('    #define VMCS_%s %s' % (name, num))
	else:
		print(i)
*/

    #define VMCS_info_vminstr_error 0x4400
    #define VMCS_info_vmexit_reason 0x4402
    #define VMCS_info_vmexit_interrupt_information 0x4404
    #define VMCS_info_vmexit_interrupt_error_code 0x4406
    #define VMCS_info_IDT_vectoring_information 0x4408
    #define VMCS_info_IDT_vectoring_error_code 0x440A
    #define VMCS_info_vmexit_instruction_length 0x440C
    #define VMCS_info_vmx_instruction_information 0x440E
    #define VMCS_info_exit_qualification 0x6400
    #define VMCS_guest_paddr 0x2400
    #define VMCS_info_guest_linear_address 0x640A

    // Control fields
    //16-bit control field
    #define VMCS_control_vpid 0x0000
    // Natural 32-bit Control fields
    #define VMCS_control_VMX_pin_based 0x4000
    #define VMCS_control_VMX_cpu_based 0x4002
    #define VMCS_control_VMX_seccpu_based 0x401E
    #define VMCS_control_exception_bitmap 0x4004
    #define VMCS_control_pagefault_errorcode_mask 0x4006
    #define VMCS_control_pagefault_errorcode_match 0x4008
    #define VMCS_control_CR3_target_count 0x400A
    #define VMCS_control_VM_exit_controls 0x400C
    #define VMCS_control_VM_exit_MSR_store_count 0x400E
    #define VMCS_control_VM_exit_MSR_load_count 0x4010
    #define VMCS_control_VM_entry_controls 0x4012
    #define VMCS_control_VM_entry_MSR_load_count 0x4014
    #define VMCS_control_VM_entry_interruption_information 0x4016
    #define VMCS_control_VM_entry_exception_errorcode 0x4018
    #define VMCS_control_VM_entry_instruction_length 0x401A
    #define VMCS_control_Task_PRivilege_Threshold 0x401C
    // Natural 64-bit Control fields
    #define VMCS_control_CR0_mask 0x6000
    #define VMCS_control_CR4_mask 0x6002
    #define VMCS_control_CR0_shadow 0x6004
    #define VMCS_control_CR4_shadow 0x6006
#ifndef __DEBUG_QEMU__
    #define VMCS_control_CR3_target0 0x6008
    #define VMCS_control_CR3_target1 0x600A
    #define VMCS_control_CR3_target2 0x600C
    #define VMCS_control_CR3_target3 0x600E
#endif /* !__DEBUG_QEMU__ */
    // Full 64-bit Control fields
    #define VMCS_control_IO_BitmapA_address 0x2000
    #define VMCS_control_IO_BitmapB_address 0x2002
    #define VMCS_control_MSR_Bitmaps_address 0x2004
    #define VMCS_control_VM_exit_MSR_store_address 0x2006
    #define VMCS_control_VM_exit_MSR_load_address 0x2008
    #define VMCS_control_VM_entry_MSR_load_address 0x200A
#ifndef __DEBUG_QEMU__
    #define VMCS_control_Executive_VMCS_pointer 0x200C
#endif /* !__DEBUG_QEMU__ */
    #define VMCS_control_TSC_offset 0x2010
    #define VMCS_control_EPT_pointer 0x201A
    // Host-State fields
    // Natural 64-bit Host-State fields
    #define VMCS_host_CR0 0x6C00
    #define VMCS_host_CR3 0x6C02
    #define VMCS_host_CR4 0x6C04
    #define VMCS_host_FS_base 0x6C06
    #define VMCS_host_GS_base 0x6C08
    #define VMCS_host_TR_base 0x6C0A
    #define VMCS_host_GDTR_base 0x6C0C
    #define VMCS_host_IDTR_base 0x6C0E
    #define VMCS_host_SYSENTER_ESP 0x6C10
    #define VMCS_host_SYSENTER_EIP 0x6C12
    #define VMCS_host_RSP 0x6C14
    #define VMCS_host_RIP 0x6C16
    // Natural 32-bit Host-State fields
    #define VMCS_host_SYSENTER_CS 0x4C00
    // Natural 16-bit Host-State fields
    #define VMCS_host_ES_selector 0x0C00
    #define VMCS_host_CS_selector 0x0C02
    #define VMCS_host_SS_selector 0x0C04
    #define VMCS_host_DS_selector 0x0C06
    #define VMCS_host_FS_selector 0x0C08
    #define VMCS_host_GS_selector 0x0C0A
    #define VMCS_host_TR_selector 0x0C0C
    // Guest-State fields
    // Natural 64-bit Guest-State fields
    #define VMCS_guest_CR0 0x6800
    #define VMCS_guest_CR3 0x6802
    #define VMCS_guest_CR4 0x6804
    #define VMCS_guest_ES_base 0x6806
    #define VMCS_guest_CS_base 0x6808
    #define VMCS_guest_SS_base 0x680A
    #define VMCS_guest_DS_base 0x680C
    #define VMCS_guest_FS_base 0x680E
    #define VMCS_guest_GS_base 0x6810
    #define VMCS_guest_LDTR_base 0x6812
    #define VMCS_guest_TR_base 0x6814
    #define VMCS_guest_GDTR_base 0x6816
    #define VMCS_guest_IDTR_base 0x6818
    #define VMCS_guest_DR7 0x681A
    #define VMCS_guest_RSP 0x681C
    #define VMCS_guest_RIP 0x681E
    #define VMCS_guest_RFLAGS 0x6820
    #define VMCS_guest_pending_debug_x 0x6822
    #define VMCS_guest_SYSENTER_ESP 0x6824
    #define VMCS_guest_SYSENTER_EIP 0x6826
    #define VMCS_guest_PDPTE0 0x280A
    #define VMCS_guest_PDPTE1 0x280C
    #define VMCS_guest_PDPTE2 0x280E
    #define VMCS_guest_PDPTE3 0x2810
    // Natural 32-bit Guest-State fields
    #define VMCS_guest_ES_limit 0x4800
    #define VMCS_guest_CS_limit 0x4802
    #define VMCS_guest_SS_limit 0x4804
    #define VMCS_guest_DS_limit 0x4806
    #define VMCS_guest_FS_limit 0x4808
    #define VMCS_guest_GS_limit 0x480A
    #define VMCS_guest_LDTR_limit 0x480C
    #define VMCS_guest_TR_limit 0x480E
    #define VMCS_guest_GDTR_limit 0x4810
    #define VMCS_guest_IDTR_limit 0x4812
    #define VMCS_guest_ES_access_rights 0x4814
    #define VMCS_guest_CS_access_rights 0x4816
    #define VMCS_guest_SS_access_rights 0x4818
    #define VMCS_guest_DS_access_rights 0x481A
    #define VMCS_guest_FS_access_rights 0x481C
    #define VMCS_guest_GS_access_rights 0x481E
    #define VMCS_guest_LDTR_access_rights 0x4820
    #define VMCS_guest_TR_access_rights 0x4822
    #define VMCS_guest_interruptibility 0x4824
    #define VMCS_guest_activity_state 0x4826
#ifndef __DEBUG_QEMU__
    #define VMCS_guest_SMBASE 0x4828
#endif /* !__DEBUG_QEMU__ */
    #define VMCS_guest_SYSENTER_CS 0x482A
    // Natural 16-bit Guest-State fields
    #define VMCS_guest_ES_selector 0x0800
    #define VMCS_guest_CS_selector 0x0802
    #define VMCS_guest_SS_selector 0x0804
    #define VMCS_guest_DS_selector 0x0806
    #define VMCS_guest_FS_selector 0x0808
    #define VMCS_guest_GS_selector 0x080A
    #define VMCS_guest_LDTR_selector 0x080C
    #define VMCS_guest_TR_selector 0x080E
    // Full 64-bit Guest-State fields
    #define VMCS_guest_VMCS_link_pointer 0x2800
    #define VMCS_guest_IA32_DEBUGCTL 0x2802

#endif /* _LHV_VMCS_H_ */

