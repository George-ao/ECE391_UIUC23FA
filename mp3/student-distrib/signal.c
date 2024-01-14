#include "signal.h"
#include "types.h"
#include "lib.h"
// #include "signal_linkage.S"


/* 
 * signal_init
 *   DESCRIPTION: Initialize the signal handler
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
void signal_init(){
    int i;
    for(i = 0; i < SIGNAL_NUM; i++){
        // signal_handlers[i] = NULL;
    }
}


/* 
 * send_signal
 *   DESCRIPTION: Send a signal to a process
 *   INPUTS: pid -- the process id
 *           signum -- the signal number
 *   OUTPUTS: none
 *   RETURN VALUE: 0 on success, -1 on failure
 *   SIDE EFFECTS: none
 */
void send_signal(int signum){
    cli();
    int32_t pid = get_pid();
    process_control_block_t* pcb = get_pcb_by_pid(pid);
    pcb->signals[signum] = 1;
    sti();
}



/* 
 * sig_handler
 *   DESCRIPTION: The signal handler
 *   INPUTS: signum -- the signal number
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
void sig_handler(){
    int32_t signum = -1;
    int32_t i, user_esp;
    int32_t pid = get_pid();
    process_control_block_t* pcb = get_pcb_by_pid(pid);
    for (i = 0; i < SIGNAL_NUM; i++){
        if (pcb->signals[i] == 1){
            signum = i;
            break;
        }
    }
    if (signum < 0 || signum >= SIGNAL_NUM){
        return;
    }
    if (pcb->sigaction[signum] == NULL){
        printf("Invalid signal handler!\n");
        return;
    }
    hw_context_t* ker_hw_context = (hw_context_t*)(get_kernel_stack_bottom() - HW_CONTEXT_SIZE);
    user_esp = ker_hw_context->ESP;
    
    if (user_esp < KER_BOTTOM) return;      // if interrupted from kernel, do nothing
    print_sig_info(signum);
    if (pcb->sigaction[signum] == sig_kill || pcb->sigaction[signum] == sig_ignore) {
        ((void(*)())(pcb->sigaction[signum]))();
        return;
    }
    /* set up code on stack to do sigreturn() */
    memcpy((void*)(user_esp - SIGRET_CODE_SIZE), (void*)call_sig_return, SIGRET_CODE_SIZE);
    /* copy hardware context */
    memcpy((void*)(user_esp - SIGRET_CODE_SIZE-HW_CONTEXT_SIZE), (void*)ker_hw_context, HW_CONTEXT_SIZE);
    /* set up signal number */
    memcpy((void*)(user_esp - SIGRET_CODE_SIZE-HW_CONTEXT_SIZE-4), (void*)(&signum), 4);
    /* set up return address */
    memcpy((void*)(user_esp - SIGRET_CODE_SIZE-HW_CONTEXT_SIZE-8), (void*)(user_esp-SIGRET_CODE_SIZE), 4);

    /* set esp to stack top as above */
    ker_hw_context->ESP = user_esp - SIGRET_CODE_SIZE - HW_CONTEXT_SIZE - 8;
    /* set eip to handler */
    ker_hw_context->EIP = (uint32_t)(pcb->sigaction[signum]);
    pcb->signals[signum] = 0;
}



/* 
 * kill
 *   DESCRIPTION: Send a signal to a process
 *   INPUTS: pid -- the process id
 *           signum -- the signal number
 *   OUTPUTS: none
 *   RETURN VALUE: 0 on success, -1 on failure
 *   SIDE EFFECTS: none
 */
void sig_kill(){
    // process_control_block_t* pcb = get_pcb();
    // pcb->sa_mask[signum] = 0;
    // cb->signals[signum] = 1;
    halt(0);
}



/* 
 * sig_ignore
 *   DESCRIPTION: Ignore a signal
 *   INPUTS: signum -- the signal number
 *   OUTPUTS: none
 *   RETURN VALUE: 0 on success, -1 on failure
 *   SIDE EFFECTS: none
 */
void sig_ignore()
{
    return;
}


/* 
 * sig_default
 *   DESCRIPTION: Default signal handler
 *   INPUTS: signum -- the signal number
 *   OUTPUTS: none
 *   RETURN VALUE: 0 on success, -1 on failure
 *   SIDE EFFECTS: none
 */
void sig_default()
{
    return;
}


/*
 * print_sig_info
 *  DESCRIPTION: Print the signal information
 *  INPUTS: signum -- the signal number
 *  OUTPUTS: none
 *  RETURN VALUE: none
 *  SIDE EFFECTS: none
*/
void print_sig_info(int signum){
    switch(signum){
        case 0:
            printf("Program received a SIG_DIV_ZERO, kill program: Divide by zero!\n");
            break;
        case 1:
            printf("Program received a SIG_SEGFAULT, kill program: Segmentation fault!\n");
            break;
        case 2:
            printf("Program received a SIG_INTERRUPT, kill program: Interrupt!\n");
            break;
        case 3:
            // printf("Program received a SIG_ALARM, kill program: Alarm!\n");
            break;
        case 4:
            // printf("Program received a SIG_USER1, kill program: User defined signal 1!\n");
            break;
        default:            // should not reach here
            break;
    }
}
