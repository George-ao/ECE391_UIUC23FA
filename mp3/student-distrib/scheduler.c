#include "scheduler.h"
#include "system_call.h"
#include "x86_desc.h"
#include "page.h"


int32_t scheduler_queue[MAX_NUM];
int32_t pid_forward;


/* 
 * switch_schedule: switch to a scheduler
 * Input: none
 * Output: none
 * Return value: none
 * Side effect: execute a certain schedule and get the corresponding PCB
*/
void scheduler_init(){
    int32_t i;
    change_curr_scheduler(-1); // set the current scheduler_id to be -1, so that on the very first second we have scheduler_id=0
    pid_forward = 0;
    for(i=0; i<MAX_NUM; ++i)
        scheduler_queue[i] = INITIALIZATION_REQUIRED; // set all three scheduler to be unused
}

/* 
 * switch_schedule: switch to a scheduler
 * Input: none
 * Output: none
 * Return value: none
 * Side effect: execute a certain schedule and get the corresponding PCB
*/
void switch_schedule()
{
    process_control_block_t* scheduling_pcb = get_pcb(); // get stack info for scheduling
    register int32_t read_ebp asm ("ebp");
    register int32_t read_esp asm ("esp");
    scheduling_pcb->ebp_sched = read_ebp;
    scheduling_pcb->esp_sched = read_esp; 
    int32_t scheduler_curr = get_curr_scheduler();
    scheduler_curr++; // increment the counter for scheduler

    if(scheduler_curr == 3)
        scheduler_curr = 0;             // round robin
    change_curr_scheduler(scheduler_curr);
    pid_forward = scheduler_queue[scheduler_curr];

    if(pid_forward == INITIALIZATION_REQUIRED){ // if the first three are called (while the corresponding shell hasn't been onpened)
        // putc((uint8_t)scheduler_curr);
        switch_video_map_paging(shell_count);    // buffering
        int8_t* runshell = "shell";
        execute((uint8_t*)runshell);
    }
    page_init_by_idx(pid_forward);      // map user program
    switch_video_map_paging(pid_forward);

    // prepare for context switch (in new process)
    tss.ss0  = (uint16_t)KERNEL_DS;
    tss.esp0 = (uint32_t)(get_pcb_by_pid(pid_forward-1) - 4);
    process_control_block_t* pcb_forward = get_pcb_by_pid(pid_forward);

    // reload context
    int32_t ebp_next = pcb_forward->ebp_sched;
    int32_t esp_next = pcb_forward->esp_sched;
    asm volatile(
        " movl %0, %%ebp ; \
          movl %1, %%esp ; \
                         ; \
          leave          ; \
          ret            "
        :                                   \
        : "r" (ebp_next), "r" (esp_next)    \
        : "ebp", "esp");     

}
