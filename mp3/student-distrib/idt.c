#include "idt.h"

static void exceptn_idt_entry(uint8_t index);
static void interpt_idt_entry(uint8_t index);
static void syscall_idt_entry(uint8_t index);

/* 
 * idt_init: initialize the Interrupt Descriptor Table 
 * Input: none
 * Output: none
 * Side effect: clear all the entries and set the exception, interrupts and system calls
*/
void idt_init(){
    int i;
    // initialize all idt to 0
    for(i = 0; i < NUM_VEC; i++){
        idt[i].val[0] = 0;
        idt[i].val[1] = 0;
    }
    // set idt entries
    exceptn_idt_entry(DIVISION_ERROR);
    exceptn_idt_entry(DEBUG);
    exceptn_idt_entry(NMI);
    exceptn_idt_entry(BREAKPOINT);
    exceptn_idt_entry(OVERFLOW);
    exceptn_idt_entry(BOUND_RANGE_EXCEEDED);
    exceptn_idt_entry(INVALID_OPCODE);
    exceptn_idt_entry(DEVICE_NOT_AVAIL);
    exceptn_idt_entry(DOUBLE_FAULT);
    exceptn_idt_entry(COPROCESSOR_SEG_OVERRUN);
    exceptn_idt_entry(INVALID_TASK_STATE_SEG);
    exceptn_idt_entry(SEG_NOT_PRESENT);
    exceptn_idt_entry(STACK_SEG);
    exceptn_idt_entry(GENERAL_PROTECTION);
    exceptn_idt_entry(PAGE_FAULT);
    exceptn_idt_entry(RESERVED);
    exceptn_idt_entry(FLOATING_POINT);
    exceptn_idt_entry(ALIGNMENT_CHECK);
    exceptn_idt_entry(MACHINE_CHECK);
    exceptn_idt_entry(SIMD_FLOATING_POINT);
    SET_IDT_ENTRY(idt[DIVISION_ERROR], division_error_linkage);
    SET_IDT_ENTRY(idt[DEBUG], debug_linkage);
    SET_IDT_ENTRY(idt[NMI], nmi_linkage);
    SET_IDT_ENTRY(idt[BREAKPOINT], breakpoint_linkage);
    SET_IDT_ENTRY(idt[OVERFLOW], overflow_linkage);
    SET_IDT_ENTRY(idt[BOUND_RANGE_EXCEEDED], bound_range_exceeded_linkage);
    SET_IDT_ENTRY(idt[INVALID_OPCODE], invalid_opcode_linkage);
    SET_IDT_ENTRY(idt[DEVICE_NOT_AVAIL], device_not_avail_linkage);
    SET_IDT_ENTRY(idt[DOUBLE_FAULT], double_fault_linkage);
    SET_IDT_ENTRY(idt[COPROCESSOR_SEG_OVERRUN], coprocessor_seg_overrun_linkage);
    SET_IDT_ENTRY(idt[INVALID_TASK_STATE_SEG], invalid_task_state_seg_linkage);
    SET_IDT_ENTRY(idt[SEG_NOT_PRESENT], seg_not_present_linkage);
    SET_IDT_ENTRY(idt[STACK_SEG], stack_seg_linkage);
    SET_IDT_ENTRY(idt[GENERAL_PROTECTION], general_protection_linkage);
    SET_IDT_ENTRY(idt[PAGE_FAULT], page_fault_linkage);
    SET_IDT_ENTRY(idt[RESERVED], reserved_linkage);
    SET_IDT_ENTRY(idt[FLOATING_POINT], floating_point_linkage);
    SET_IDT_ENTRY(idt[ALIGNMENT_CHECK], alignment_check_linkage);
    SET_IDT_ENTRY(idt[MACHINE_CHECK], machine_check_linkage);
    SET_IDT_ENTRY(idt[SIMD_FLOATING_POINT], simd_floating_point_linkage);
    // interrupts
    interpt_idt_entry(PIT);
    interpt_idt_entry(KEYBOARD);
    interpt_idt_entry(RTC);
    SET_IDT_ENTRY(idt[PIT], PIT_linkage);
    SET_IDT_ENTRY(idt[KEYBOARD], keyboard_linkage);
    SET_IDT_ENTRY(idt[RTC], rtc_linkage);
    // system call
    syscall_idt_entry(SYS_CALL);
    SET_IDT_ENTRY(idt[SYS_CALL], sys_call_linkage);
    // load IDT
    lidt(idt_desc_ptr);
}

/* 
 * exceptn_idt_entry: initialize exceptions in IDT with dpl=0 (highest priority) 
 * Input: index
 * Output: none
 * Side effect: provide a universal initialization for exceptions in IDT
*/
static void exceptn_idt_entry(uint8_t index){
    idt[index].reserved3 = 1;
    idt[index].dpl = 0;
    
}

/* 
 * interpt_idt_entry: initialize interrupt in IDT with dpl=0 (highest priority) 
 * Input: index
 * Output: none
 * Side effect: provide a universal initialization for interrupt in IDT
*/
static void interpt_idt_entry(uint8_t index){
    idt[index].reserved3 = 0;
    idt[index].dpl = 0;
    
}

/* 
 * syscall_idt_entry: initialize system calls in in IDT with dpl=3 (lowest priority) 
 * Input: index
 * Output: none
 * Side effect: provide a universal initialization for system calls in IDT
*/
static void syscall_idt_entry(uint8_t index){
    idt[index].reserved3 = 1;
    idt[index].dpl = 3;
   
}
