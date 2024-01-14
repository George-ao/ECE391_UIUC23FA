#ifndef _IDT_H
#define _IDT_H

#include "idt_linkage.h"
#include "x86_desc.h"

// enumerator for the exception, interrupt and system call entries
enum idt_entries{
    // exceptions
    DIVISION_ERROR = 0x00,
    DEBUG = 0x01,
    NMI = 0x02,
    BREAKPOINT = 0x03,
    OVERFLOW = 0x04,
    BOUND_RANGE_EXCEEDED = 0x05,
    INVALID_OPCODE = 0x06,
    DEVICE_NOT_AVAIL = 0x07,
    DOUBLE_FAULT = 0x08,
    COPROCESSOR_SEG_OVERRUN = 0x09,
    INVALID_TASK_STATE_SEG = 0x0A,
    SEG_NOT_PRESENT = 0x0B,
    STACK_SEG = 0x0C,
    GENERAL_PROTECTION = 0x0D,
    PAGE_FAULT = 0x0E,
    RESERVED = 0x0F,
    FLOATING_POINT = 0x10,
    ALIGNMENT_CHECK = 0x11,
    MACHINE_CHECK = 0X12,
    SIMD_FLOATING_POINT = 0x13,
    // interrupts
    PIT = 0x20,
    KEYBOARD = 0x21,
    RTC = 0x28,
    // system call
    SYS_CALL = 0x80
};

// initialize idt entries
extern void idt_init();

#endif
