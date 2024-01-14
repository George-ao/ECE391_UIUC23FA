#include "idt_handler.h"

// exceptions
// print exceptions to report errors
// void division_error_handler()       {cli(); printf("Oops! Exception: Division Error\n");           halt(255); sti();}
// void debug_handler()                {cli(); printf("Oops! Exception: Debug\n");                    halt(255); sti();}
// void nmi_handler()                  {cli(); printf("Oops! Exception: Non-maskable Interrupt\n");   halt(255); sti();}
// void breakpoint_handler()           {cli(); printf("Oops! Exception: Breakpoint\n");               halt(255); sti();}
// void overflow_handler()             {cli(); printf("Oops! Exception: Overflow\n");                 halt(255); sti();}
// void bound_range_exceeded_handler() {cli(); printf("Oops! Exception: Bound Range Exceeded\n");     halt(255); sti();}
// void invalid_opcode_handler()       {cli(); printf("Oops! Exception: Invalid Opcode\n");           halt(255); sti();}
// void device_not_avail_handler()     {cli(); printf("Oops! Exception: Device Not Available\n");     halt(255); sti();}
// void double_fault_handler()         {cli(); printf("Oops! Exception: Double Fault\n");             halt(255); sti();}
// void coprocessor_seg_overrun_handler()  {cli(); printf("Oops! Exception: Coprocessor Segment Overrun\n"); halt(255); sti();}
// void invalid_task_state_seg_handler()   {cli(); printf("Oops! Exception: Invalid Task State Segment\n");  halt(255); sti();}
// void seg_not_present_handler()      {cli(); printf("Oops! Exception: Segment Not Present\n");      halt(255); sti();}
// void stack_seg_handler()            {cli(); printf("Oops! Exception: Stack Segment Fault\n");      halt(255); sti();}
// void general_protection_handler()   {cli(); printf("Oops! Exception: General Protection Fault\n"); halt(255); sti();}
// void page_fault_handler()           {cli(); printf("Oops! Exception: Page Fault\n");               halt(255); sti();}
// void reserved_handler()             {cli(); printf("Oops! Exception: Reserved\n");                 halt(255); sti();}
// void floating_point_handler()       {cli(); printf("Oops! Exception: Floating Point Exception\n"); halt(255); sti();}
// void alignment_check_handler()      {cli(); printf("Oops! Exception: Alignment Check\n");          halt(255); sti();}
// void machine_check_handler()        {cli(); printf("Oops! Exception: Machine Check\n");            halt(255); sti();}
// void simd_floating_point_handler()  {cli(); printf("Oops! Exception: SIMD Floating Point\n");      halt(255); sti();}

void division_error_handler()           {cli(); send_signal(SIG_DIV_ZERO);  sti();}
void debug_handler()                    {cli(); send_signal(SIG_SEGFAULT);  sti();}
void nmi_handler()                      {cli(); send_signal(SIG_SEGFAULT);  sti();}
void breakpoint_handler()               {cli(); send_signal(SIG_SEGFAULT);  sti();}
void overflow_handler()                 {cli(); send_signal(SIG_SEGFAULT);  sti();}
void bound_range_exceeded_handler()     {cli(); send_signal(SIG_SEGFAULT);  sti();}
void invalid_opcode_handler()           {cli(); send_signal(SIG_SEGFAULT);  sti();}
void device_not_avail_handler()         {cli(); send_signal(SIG_SEGFAULT);  sti();}
void double_fault_handler()             {cli(); send_signal(SIG_SEGFAULT);  sti();}
void coprocessor_seg_overrun_handler()  {cli(); send_signal(SIG_SEGFAULT);  sti();}
void invalid_task_state_seg_handler()   {cli(); send_signal(SIG_SEGFAULT);  sti();}
void seg_not_present_handler()          {cli(); send_signal(SIG_SEGFAULT);  sti();}
void stack_seg_handler()                {cli(); send_signal(SIG_SEGFAULT);  sti();}
void general_protection_handler()       {cli(); send_signal(SIG_SEGFAULT);  sti();}
void page_fault_handler()               {cli(); send_signal(SIG_SEGFAULT);  sti();}
void reserved_handler()                 {cli(); send_signal(SIG_SEGFAULT);  sti();}
void floating_point_handler()           {cli(); send_signal(SIG_SEGFAULT);  sti();}
void alignment_check_handler()          {cli(); send_signal(SIG_SEGFAULT);  sti();}
void machine_check_handler()            {cli(); send_signal(SIG_SEGFAULT);  sti();}
void simd_floating_point_handler()      {cli(); send_signal(SIG_SEGFAULT);  sti();}





// void sys_call_handler()         {clear(); printf("System Call! Wait for checkpoint 3 and 4!"); while(1);}
