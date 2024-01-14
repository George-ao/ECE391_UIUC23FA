#ifndef _IDT_LINKAGE_H
#define _IDT_LINKAGE_H

#include "idt_handler.h"
#include "rtc.h"
#include "keyboard.h"
#include "scheduler.h"
#include "PIT.h"
#include "signal.h"

#ifndef ASM
// exceptions
extern void division_error_linkage();
extern void debug_linkage();
extern void nmi_linkage();
extern void breakpoint_linkage();
extern void overflow_linkage();
extern void bound_range_exceeded_linkage();
extern void invalid_opcode_linkage();
extern void device_not_avail_linkage();
extern void double_fault_linkage();
extern void coprocessor_seg_overrun_linkage();
extern void invalid_task_state_seg_linkage();
extern void seg_not_present_linkage();
extern void stack_seg_linkage();
extern void general_protection_linkage();
extern void page_fault_linkage();
extern void reserved_linkage();
extern void floating_point_linkage();
extern void alignment_check_linkage();
extern void machine_check_linkage();
extern void simd_floating_point_linkage();
// interrupts
extern void PIT_linkage();
extern void keyboard_linkage();
extern void rtc_linkage();
// system call
extern void sys_call_linkage();
#endif

#endif
