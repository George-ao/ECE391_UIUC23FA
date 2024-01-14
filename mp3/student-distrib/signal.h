#ifndef SIGNAL_H
#define SIGNAL_H

#include "types.h"
#include "lib.h"
#include "system_call.h"

#define HW_CONTEXT_SIZE 16*4
#define SIGRET_CODE_SIZE 3*4

#define SIG_DIV_ZERO    0
#define SIG_SEGFAULT    1
#define SIG_INTERRUPT   2
#define SIG_ALARM       3
#define SIG_USER1       4

#define KER_BOTTOM      0x00800000

typedef struct hw_context{
    uint32_t ECX;
    uint32_t EDX;
    uint32_t ESI;
    uint32_t EDI;
    uint32_t EBP;
    uint32_t EAX;
    uint32_t DS;
    uint32_t ES;
    uint32_t FS;
    uint32_t IRQ;
    uint32_t error_code;
    uint32_t EIP;
    uint32_t CS;
    uint32_t EFLAGS;
    uint32_t ESP;
    uint32_t SS;
}hw_context_t;


void signal_init();

// int signal(int signum, void* handler);
void send_signal(int signum);
extern void sig_handler();
void sig_kill();
void sig_ignore();
void print_sig_info(int signum);
extern void call_sig_return(void);

#endif
