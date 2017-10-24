#include <linux/kernel.h>
#include "vmx_common.h"

void vmx_switch_update_cr0_mask (bool enable, unsigned long mask);

void handle_cpu_monitor_req (cpu_event_params_t *params)
{
	unsigned long mask;
	switch (params->cpu_reg) {
		case CPU_REG_CR0:
			vmx_switch_update_cr0_mask(params->enable, params->mask);
		break;
	}
}

void handle_kernel_hardening_hypercall (u64 params)
{
	cpu_event_params_t *vmcall_params = (cpu_event_params_t *)params;
	
	switch (vmcall_params->req_id) {
		case CPU_MONITOR_REQ:
			handle_cpu_monitor_req(vmcall_params);
		break;
		case MSR_MONITOR_REQ:

		break;
		default:
		break;
	}
}
