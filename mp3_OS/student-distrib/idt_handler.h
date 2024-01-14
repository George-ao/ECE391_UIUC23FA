#ifndef _IDT_HANDLER_H
#define _IDT_HANDLER_H
#include "lib.h"
#include "system_call.h"
#include "signal.h"

// exceptions handlers
extern void division_error_handler();
extern void debug_handler();
extern void nmi_handler();
extern void breakpoint_handler();
extern void overflow_handler();
extern void bound_range_exceeded_handler();
extern void invalid_opcode_handler();
extern void device_not_avail_handler();
extern void double_fault_handler();
extern void coprocessor_seg_overrun_handler();
extern void invalid_task_state_seg_handler();
extern void seg_not_present_handler();
extern void stack_seg_handler();
extern void general_protection_handler();
extern void page_fault_handler();
extern void reserved_handler();
extern void floating_point_handler();
extern void alignment_check_handler();
extern void machine_check_handler();
extern void simd_floating_point_handler();
extern void sys_call_handler();

#endif
