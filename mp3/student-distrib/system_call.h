#ifndef _SYSTEM_CALL_H
#define _SYSTEM_CALL_H

#include "lib.h"
#include "types.h"
#include "file_system.h"
#include "rtc.h"
#include "terminal.h"

#define MAX_FD_ENTRIES  8
#define PROCESS_COUNT   6
#define BUF_SIZE        128
#define SIGNAL_NUM      5

#define SHELL_PHYS_ADDR 0x00800000
#define OTHER_PHYS_ADDR 0x00C00000
#define USER_VIRT_ADDR  0x08000000
#define USER_PROGRAM_VIRT_ADDR  0x08048000
#define USER_STACK 0x08400000

typedef struct fops_table_t {
    int32_t (*fopen)(const uint8_t* filename);
    int32_t (*fclose)(int32_t fd);
    int32_t (*fread)(int32_t fd, void* buf, int32_t nbytes);
    int32_t (*fwrite)(int32_t fd, const void* buf, int32_t nbytes);
} fops_table_t;

typedef struct file_descriptor_t {
    fops_table_t* fops_table_ptr;
    uint32_t inode;
    uint32_t file_position;
    uint32_t flags;
} file_descriptor_t;


extern uint32_t process_ids[PROCESS_COUNT];
extern file_descriptor_t file_descriptor_table[MAX_FD_ENTRIES];
extern int32_t shell_count;
void fd_operations_table_init();

typedef struct process_control_block
{
    uint32_t pid_prev;
    uint32_t pid_now;
    int32_t esp_inuse;
    int32_t ebp_inuse;
    int32_t ebp_sched;
    int32_t esp_sched;
    int32_t terminal_num;
    int32_t user_video_indicator;
    int8_t argument[BUF_SIZE];
    int8_t signals[SIGNAL_NUM];
    int8_t sa_mask[SIGNAL_NUM];
    void*  sigaction[SIGNAL_NUM];
    file_descriptor_t fds[8];
} process_control_block_t;

int32_t halt(uint8_t status);
int32_t execute(const uint8_t* command);
int32_t read(int32_t fd, void* buf, int32_t nbytes);
int32_t write(int32_t fd, const void* buf, int32_t nbytes);
int32_t open(const uint8_t* filename);
int32_t close(int32_t fd);
int32_t getargs(uint8_t* buf, int32_t nbytes);
int32_t vidmap(uint8_t** screen_start);
int32_t set_handler(int32_t signum, void* handler_address);
int32_t sigreturn(void);
int32_t malloc (int32_t size);
int32_t free (void* ptr);
int32_t ioctl(unsigned long cmd, unsigned long arg);



int32_t get_pid(void);
int32_t get_kernel_stack_bottom(void);
int32_t get_terminal_num(int32_t pid);



process_control_block_t* get_pcb(void);
process_control_block_t* get_pcb_by_pid(int32_t pid);


#endif
