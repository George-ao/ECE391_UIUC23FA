#ifndef _TERMINAL_H
#define _TERMINAL_H

#include "system_call.h"
#include "types.h"
#include "lib.h"


#define SCREEN_SIZE 80*25
// char keyboard_buf[BUF_SIZE];
// volatile int buf_cnt;
// int32_t read_flag;
extern void terminal_init();
extern int32_t terminal_open(const uint8_t* filename);
extern int32_t terminal_read(int32_t fd, void* buf, int32_t nbytes);
extern int32_t terminal_write(int32_t fd, const void* buf, int32_t nbytes);
extern int32_t terminal_close(int32_t fd); // do nothing and return 0

#endif
