#include "system_call.h"
#include "page.h"
#include "x86_desc.h"
#include "terminal.h"
#include "scheduler.h"
#include "signal.h"

file_descriptor_t file_descriptor_table[MAX_FD_ENTRIES];
uint32_t process_ids[PROCESS_COUNT] = {0};
// static int32_t shell_count = 0;

#define MB_EIGHT 0x800000 
#define KB_EIGHT 0x2000

int32_t shell_count = 0;
static fops_table_t rtc_fops_table;
static fops_table_t dir_fops_table;
static fops_table_t file_fops_table;
static fops_table_t stdin_fops_table;
static fops_table_t stdout_fops_table;




/* 
 * bad_call: return -1 for opening or closing stdin/stdout, and for reading from stdout and writing to stdin
 * Input: none (as the input of the system can be ignored)
 * Output: none
 * Return value: -1 for function failure (as is described in the document)
 * Side effect: return the correct signifier for error
*/
int32_t bad_call(){
    return -1;
}


/* 
 * fd_operations_table_init: initialize the fd_operation_table
 * Input: none
 * Output: none
 * Return value: none
 * Side effect: set the correct content of the OCRW of the fd tables
*/
void fd_operations_table_init() {
    file_fops_table.fopen  = file_open;
    file_fops_table.fclose = file_close;
    file_fops_table.fread  = file_read;
    file_fops_table.fwrite = file_write;

    dir_fops_table.fopen  = dir_open;
    dir_fops_table.fclose = dir_close;
    dir_fops_table.fread  = dir_read;
    dir_fops_table.fwrite = dir_write;

    rtc_fops_table.fopen  = rtc_open;
    rtc_fops_table.fclose = rtc_close;
    rtc_fops_table.fread  = rtc_read;
    rtc_fops_table.fwrite = rtc_write;

    stdin_fops_table.fopen  = bad_call;     // you might not open, close or write to stdin
    stdin_fops_table.fclose = bad_call;
    stdin_fops_table.fread  = terminal_read;
    stdin_fops_table.fwrite = bad_call;

    stdout_fops_table.fopen  = bad_call;    // you might not open, close or read from stdout
    stdout_fops_table.fclose = bad_call;
    stdout_fops_table.fread  = bad_call;
    stdout_fops_table.fwrite = terminal_write;

    file_descriptor_table[0].fops_table_ptr = &stdin_fops_table;
    file_descriptor_table[1].fops_table_ptr = &stdout_fops_table;
    file_descriptor_table[0].flags = 1;
    file_descriptor_table[1].flags = 1;
}



/* ------- */


/* 
 * halt: halting a certain process
 * Input: status - signify whether the process is ended normally
 * Output: "cannot halt pid=0 (first shell), restarting" if halting the first shell
 * Return value: 0
 * Side effect: halt a certain process, close the FDs and restore the parent data
*/
int32_t halt(uint8_t status)
{
    uint32_t return_value = (uint32_t)status; // initialize as interrupt happens to halt
    if(return_value == 255) return_value = 256;
    uint32_t i;
    process_control_block_t* pcb_now= get_pcb();
    uint32_t pid_current = pcb_now->pid_now;
    process_ids[pid_current]=0; 
    if(pid_current==0 || pid_current==1 || pid_current==2){
        printf("cannot halt first shell, restarting\n");
        process_ids[pid_current]=0;
        uint8_t* process= (uint8_t *)"shell"; // we choose to reboot the shell
        execute(process);
    }
   
    // close relevant FDs
    for (i=2; i < MAX_FD_ENTRIES; ++i){
        if(!pcb_now->fds[i].flags)
            continue; // if the file is not open
        close(i);
        // close the file
    }

    // restore parent paging
    // shell_page_init((uint32_t*)SHELL_PHYS_ADDR, (uint32_t*)USER_VIRT_ADDR);
    process_control_block_t * pcb_prev= (process_control_block_t *)(MB_EIGHT-(pcb_now->pid_prev+1)*KB_EIGHT); // get the parent pcb (here we must have a prev)
    uint32_t prev_pid=pcb_prev->pid_now;
    page_init_by_idx(prev_pid);
    page_video_unmount(pid_current);
    scheduler_queue[pcb_now->terminal_num] = pcb_now->pid_prev; // remove the pid from the scheduler queue

    // prepare context switch
    tss.esp0 = MB_EIGHT-prev_pid*KB_EIGHT-4;
    tss.ss0 = KERNEL_DS;
    // jump to execute return
    asm volatile(
        " movl %0, %%eax ; \
          movl %1, %%ebp ; \
          movl %2, %%esp ; \
          leave          ; \
          ret            "
        :
        : "r" ((int32_t)return_value), \
          "r" (pcb_now->ebp_inuse), \
          "r" (pcb_now->esp_inuse) \
        : "eax", "ebp", "esp");
        
    return 0;
}


/* 
 * execute: execute a certain process
 * Input: command - the command including file name and command
 * Output: none
 * Return value: 0
 * Side effect: execute a certain process and set the corresponding PCBs correctly
*/
int32_t execute(const uint8_t* command)
{
    cli();
    int32_t pid, i, j, length;
    uint8_t buf[4] = {'\0'};
    uint8_t argument[BUF_SIZE]={'\0'};
    uint8_t filename[32] = {'\0'};
    // uint8_t start_addr[4] = {'\0'};

    /* read the file name of the program to be executed */
    length = strlen((int8_t*)command);
    for (i = 0; i < length; i++) 
    {
        if ((command[i] == ' ')||(command[i] == '\0')) 
        {
            i++;
            break;
        }
        filename[i] = command[i];
    }

    if (strncmp("shell", (int8_t*)filename, 5) == 0) shell_count++;
    /* parse the argument */
    for (j=0; i < length; i++,j++) 
    {
        if ((i>=length)||(command[i] == '\0'))
            break;
        argument[j] = command[i];
    } 

    /* check file validity */
    dentry_t file_dentry;
    if (read_dentry_by_name(filename, &file_dentry) == -1) return -1;   
    if (read_data(file_dentry.inode_num, 0, buf, 4) != 4) return -1;

    /* check if the file is executable */
    if (buf[0] != 0x7f || buf[1] != 0x45 || buf[2] != 0x4c || buf[3] != 0x46)   // check for ELF header
        return -1;
    uint32_t start_addr;
    read_data(file_dentry.inode_num, 24, (uint8_t*)(&start_addr), 4);           // read the starting address of the program

    /* find a free pid */
    for(pid=0; pid<PROCESS_COUNT; pid++) {
        if((process_ids[pid])&&(pid == PROCESS_COUNT-1)) { // then no more pcb can be occupied
            printf("max 6 programs!\n");
            return -1;
        }
        else if(!process_ids[pid]) {            // if the pcb is not in use
            process_ids[pid]=1 - process_ids[pid];
            break;
        }
    }
    process_ids[pid] = 1;                       // set the current pcb to be in use

    /* map user page */
    page_init_by_idx((uint32_t)pid);            // set up the pages
    
    /* copy the program to virtual address */
    read_data(file_dentry.inode_num, 0, (uint8_t*)USER_PROGRAM_VIRT_ADDR, USER_STACK-USER_PROGRAM_VIRT_ADDR);
    
    /* create PCB */
    process_control_block_t* pcb_inuse = (process_control_block_t *)(MB_EIGHT-(pid+1)*KB_EIGHT); // pid is the first free pid
    pcb_inuse->pid_now = pid;                   // set the current pcb id and enable the process array
    pcb_inuse->user_video_indicator = 0;        // set user_bideo_indicator to 0
    
    /* initialize the file_descriptor_table for stdin and stdout */
    pcb_inuse->fds[0].fops_table_ptr = &stdin_fops_table;
    pcb_inuse->fds[1].fops_table_ptr = &stdout_fops_table;
    pcb_inuse->fds[0].flags = 1;
    pcb_inuse->fds[1].flags = 1;

    /* store the parent pid */
    if(pid!=0 && pid!=1 && pid!=2) pcb_inuse->pid_prev = get_pid(); // get_pid() is pid of the parent
    else pcb_inuse->pid_prev = INITIALIZATION_REQUIRED;             // set a impossible value for taking the parent of the first shell

    /* store the terminal number that the process is running on */
    pcb_inuse->terminal_num = get_terminal_num(pid);
    
    /* add the pid to the scheduler run-queue */
    // int scheduler_id = get_curr_scheduler();
    // scheduler_queue[scheduler_id] = pid;
    scheduler_queue[get_terminal_num(pid)] = pid;

    /* set up sigactions */
    for (i = 0; i < SIGNAL_NUM; i++){
        pcb_inuse->signals[i] = 0;
        pcb_inuse->sa_mask[i] = 0;
    }
    pcb_inuse->sigaction[0] = sig_kill;
    pcb_inuse->sigaction[1] = sig_kill;
    pcb_inuse->sigaction[2] = sig_kill;
    pcb_inuse->sigaction[3] = sig_ignore;
    pcb_inuse->sigaction[4] = sig_ignore;

    /* store the arguments */
    memcpy(pcb_inuse->argument, argument, BUF_SIZE);    // get the argument

    /* store current esp & ebp */
    register uint32_t reg_ebp asm("ebp");
    pcb_inuse->ebp_inuse= reg_ebp;                      // set the ebp and esp for current pcb
    register uint32_t reg_esp asm("esp");
    pcb_inuse->esp_inuse= reg_esp;
    
    /* fill in TSS, prepare for context switch */
    tss.esp0 = (MB_EIGHT-(pid)*KB_EIGHT) - 4;           // store the current esp and ss
    tss.ss0  = KERNEL_DS;

    sti();
    
    /* push IRET manually, Context Switch */
    asm volatile (
       "pushl %0 ;\
        pushl %1 ;\
        pushfl   ;\
        pushl %2 ;\
        pushl %3 ;\
        IRET  ;   "
        :
        : "r" (USER_DS), \
          "r" (USER_STACK-sizeof(uint32_t)), \
          "r" (USER_CS), \
          "r" (start_addr)
		: "memory" );

    return 0;
}


/* 
 * read: read from files
 * Input: fd - index in file descriptor
 *        buf - the buffer to load the data
 *        nbytes - the length of the buffer
 * Output: none
 * Return value: the bytes read (or -1 for error)
 * Side effect: read from files (but not from stdout)
*/
int32_t read(int32_t fd, void* buf, int32_t nbytes)
{
    int32_t pid = get_pid();
    process_control_block_t* pcb = get_pcb_by_pid(pid);
    //printf("read\n");
    if (fd == 1) return -1; // can't read stdout
    if (fd < 0 || fd >= MAX_FD_ENTRIES)         // invalid fd
        return -1;
    if (pcb->fds[fd].flags == 0)   // fd not in use
        return -1;
    return pcb->fds[fd].fops_table_ptr->fread(fd, buf, nbytes);
}


/* 
 * write: write to files
 * Input: fd - index in file descriptor
 *        buf - the buffer to load the data
 *        nbytes - the length of the buffer
 * Output: none
 * Return value: the bytes written (or -1 for error)
 * Side effect: write to files (but not from stdin)
*/
int32_t write(int32_t fd, const void* buf, int32_t nbytes)
{
    //if (fd==0) return -1; // can't write to stdin
    int32_t pid = get_pid();
    process_control_block_t* pcb = get_pcb_by_pid(pid);
    if (fd <= 0 || fd >= MAX_FD_ENTRIES)         // invalid fd 
        return -1;
    if (pcb->fds[fd].flags == 0)   // fd not in use
        return -1;
    return pcb->fds[fd].fops_table_ptr->fwrite(fd, buf, nbytes);
}


/* 
 * open: open files
 * Input: filename - the file to be opened
 * Output: none
 * Return value: fd if successful, otherwise -1
 * Side effect: open files
*/
int32_t open(const uint8_t* filename)
{
    dentry_t dentry;
    int32_t i;
    int32_t pid = get_pid();
    process_control_block_t* pcb = get_pcb_by_pid(pid);
    if (read_dentry_by_name(filename, &dentry) == -1)   // return -1 if file not found
        return -1;
    // traverse file descriptor table to find an available entry
    // FLAG
    for (i = 2; i < MAX_FD_ENTRIES; i++) 
    {
        if (pcb->fds[i].flags == 0) 
        {
            pcb->fds[i].flags = 1;
            break;
        }
    }
    if (i == MAX_FD_ENTRIES)    // return -1 if no available entry
        return -1;
    
    // FILE OPERATIONS TABLE
    switch (dentry.file_type) 
    {
        case 0:     // RTC file
            pcb->fds[i].fops_table_ptr = &rtc_fops_table;
            break;
        case 1:     // Directory file
            pcb->fds[i].fops_table_ptr = &dir_fops_table;
            break;
        case 2:     // Regular file
            pcb->fds[i].fops_table_ptr = &file_fops_table;
            break;
        default:
            return -1;
    }

    // call fopen()
    pcb->fds[i].fops_table_ptr->fopen(filename);

    // INODE
    if (dentry.file_type == 2)
        pcb->fds[i].inode = dentry.inode_num;
    else
        pcb->fds[i].inode = 0;

    // FILE POSITION
    pcb->fds[i].file_position = 0;

    return i;
}


/* 
 * close: close files
 * Input: fd - index in file descriptor
 * Output: none
 * Return value: 0 if succesful, otherwise -1
 * Side effect: close the file
*/
int32_t close(int32_t fd)
{
    int32_t pid = get_pid();
    process_control_block_t* pcb = get_pcb_by_pid(pid);
    if (fd < 2 || fd >= MAX_FD_ENTRIES)         // invalid fd
        return -1;
    if (pcb->fds[fd].flags == 0)   // fd not in use
        return -1;
    pcb->fds[fd].flags = 0;
    return pcb->fds[fd].fops_table_ptr->fclose(fd);
}


/* nbytes counts NULL */
/* 
 * getargs: get the argument of the current pcb to a user space buffer
 * Input: buf - the user space buffer, nbytes - the length (including NULL)
 * Output: none
 * Return value: 0 if succesful, otherwise -1
 * Side effect: extract the argument to user space 
*/
int32_t getargs(uint8_t* buf, int32_t nbytes)
{
    // check valid nbytes
    if(nbytes < 0 || buf == NULL) return -1;
    if(nbytes == 0) return 0;
    // get the content of the current esp
    // register int32_t read_esp asm ("esp");
    // get pcb
    process_control_block_t* pcb_now= get_pcb();
    if(pcb_now == NULL) return -1;
    // return -1 if no args
    if(pcb_now->argument[0] == '\0') return -1;
    // return -1 if nbytes is not enough for all arguments
    if(nbytes < BUF_SIZE && pcb_now->argument[nbytes - 1] != '\0') return -1;
    // set nbytes to the buf_size if it is too big
    if(nbytes > BUF_SIZE) nbytes = BUF_SIZE;
    // copy args into user buffer
    memcpy(buf, pcb_now->argument, nbytes);
    return 0;
}


/* 
 * vidmap: maps the text-mode vidmem into user space
 * Input: screen_start, the start of the address
 * Output: none
 * Return value: 0 if succesful, otherwise -1
 * Side effect: get a virtual mapping
*/
int32_t vidmap(uint8_t** screen_start)
{
    int32_t pid = get_pid();
    process_control_block_t* pcb = get_pcb_by_pid(pid);
    if(screen_start==NULL)
        return -1;
    if((screen_start<= (uint8_t**) (USER_STACK-4)) && (screen_start >=(uint8_t**) USER_VIRT_ADDR))
    {
        page_video_map(screen_start, 1);
        pcb->user_video_indicator = 1;
    }
    else 
        return -1;
    return 0;
}


/* 
 * set_handler: match the program with the signaling method
 * Input: signum - the number of the signal, handler_address - the address of the handler
 * Output: none
 * Return value: 0 if succesful, otherwise -1
 * Side effect: none
*/
int32_t set_handler(int32_t signum, void* handler_address)
{
    if(NULL == handler_address)
        return -1;

    process_control_block_t* pcb_now = get_pcb();
    pcb_now->sigaction[signum] = (uint32_t*)handler_address;
    return 0;
}


/* 
 * sigreturn: run after a signal is handled
 * Input: none
 * Output: none
 * Return value: -1
 * Side effect: none
*/
int32_t sigreturn(void)
{
    // register int32_t esp_kernel asm("esp");
    int32_t i;
    process_control_block_t* pcb_now = get_pcb();
    for (i = 0; i < SIGNAL_NUM; i++){
        pcb_now->sa_mask[i] = 0;
    }
    uint32_t* kernel_bottom = (uint32_t*)get_kernel_stack_bottom();
    hw_context_t* new_ker_hw_context = (hw_context_t*)(kernel_bottom - HW_CONTEXT_SIZE);    // tear down the stack frame
    // uint32_t* esp_kernel = tss.esp0;
    uint32_t* esp_user = (uint32_t*)new_ker_hw_context->ESP;
    /* skip: ebp, eip, signum */
    memcpy(new_ker_hw_context, esp_user+5, HW_CONTEXT_SIZE);             // 16 is the length of the hw_context
    return -1;
}


/* 
 * malloc: allocate a certain amount of memory
 * Input: size - the size of the memory
 * Output: none
 * Return value: the address of the allocated memory
 * Side effect: none
 */
int32_t malloc (int32_t size)
{
    // int32_t pid = get_pid();
    // process_control_block_t* pcb = get_pcb_by_pid(pid);
    if (size <= 0) return -1;
    int32_t i;
    for (i = 0; i < MAX_FD_ENTRIES; i++) {
        // if (pcb->malloc_array[i].flags == 0) {
        //     pcb->malloc_array[i].flags = 1;
        //     pcb->malloc_array[i].size = size;
        //     return (int32_t)(&(pcb->malloc_array[i].data));
        // }
    }
    return -1;
}


/* 
 * free: free a certain amount of memory
 * Input: ptr - the address of the memory
 * Output: none
 * Return value: 0 if successful, otherwise -1
 * Side effect: none
 */
int32_t free (void* ptr)
{
    // int32_t pid = get_pid();
    // process_control_block_t* pcb = get_pcb_by_pid(pid);
    if (ptr == NULL) return -1;
    int32_t i;
    for (i = 0; i < MAX_FD_ENTRIES; i++) {
        // if (ptr == (void*)(&(pcb->malloc_array[i].data))) {
        //     pcb->malloc_array[i].flags = 0;
        //     return 0;
        // }
    }
    return -1;
}


/* 
 * ioctl: perform device-specific operations
 * Input: cmd - the command to be performed
 *        arg - the pointer of the command
 * Output: none
 * Return value: 0 if successful, otherwise -1
 * Side effect: none
 */
int32_t ioctl(unsigned long cmd, unsigned long arg)
{
    switch (cmd)
    {
    case 0:
        print_color();
        break;
    case 1:
        return setcolor((uint8_t*) arg);
        break;
    default:
        break;
    }
    return -1;
    // return file_descriptor_table[fd].fops_table_ptr->ioctl(fd, cmd, arg);
}



/* ---------- HELPER FUNCTIONS BELOW ---------- */



/* 
 * get_pid: get the current pid
 * Input: none
 * Output: none
 * Return value: current pid
 * Side effect: none
*/
int32_t get_pid(void){
    register int32_t read_esp asm ("esp");
    return (MB_EIGHT-read_esp)/KB_EIGHT;
}

/* 
 * get_pcb: get the current pcb (pointer)
 * Input: none
 * Output: none
 * Return value: current pcb
 * Side effect: none
*/
process_control_block_t* get_pcb(void){
    int32_t pid = get_pid();
    return (process_control_block_t*) (MB_EIGHT-(pid+1)*KB_EIGHT);
}

/* 
 * get_pcb_by_pid: get the pcb by a given pid
 * Input: pid
 * Output: none
 * Return value: the pcb by a certain pid
 * Side effect: none
*/
process_control_block_t* get_pcb_by_pid(int32_t pid){
    if (pid == -1)
        return (process_control_block_t*)MB_EIGHT;
    return (process_control_block_t*) (MB_EIGHT-(pid+1)*KB_EIGHT);
}

/* 
 * get_terminal_num: get the terminal number of the current process
 * Input: pid
 * Output: none
 * Return value: current terminal_id
 * Side effect: none
*/
int32_t get_terminal_num(int32_t pid){
    int32_t terminal_pid = pid;
    while (terminal_pid != 0 && terminal_pid != 1 && terminal_pid != 2)
        terminal_pid = get_pcb_by_pid(terminal_pid)->pid_prev;
    return terminal_pid;

}


/* 
 * get_kernel_stack_bottom: get the kernel stack bottom
 * Input: none
 * Output: none
 * Return value: the kernel stack bottom
 * Side effect: none
*/
int32_t get_kernel_stack_bottom(void){
    int32_t pid = get_pid();
    return (MB_EIGHT - pid*KB_EIGHT - 4);
}
