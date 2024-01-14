#ifndef _RTC_H
#define _RTC_H
#include "types.h"
#include "i8259.h"
#include "lib.h"

#define RTC_PORT 0x70 // RTC port to specify an index or register number and to disable NMI
#define RTC_DATA 0x71 // RTC port to read or write CMOS information
#define RTC_IRQ 8 // RTC IRQ number
#define RTC_REG_A 0x8A // three bytes of CMOS RAM that control the RTC's interrupts
#define RTC_REG_B 0x8B // which are initially at offset 0xA, 0xB, and 0xC
#define RTC_REG_C 0x8C // with NMI disabled, the RTC's registers can be read or written
#define VIDEO_MEM  0xB8000
#define NUM_COLS    80
#define NUM_ROWS    25

// checkpoint 1
extern void rtc_init(); // initialize RTC
extern void rtc_handler(); // RTC interrupt handler

// checkpoint 2
extern int32_t rtc_open(const uint8_t* filename); // set the interrupt rate to be 2 Hz
extern int32_t rtc_read(int32_t fd, void* buf, int32_t nbytes); // set a flag and wait for an interrupt
extern int32_t rtc_write(int32_t fd, const void* buf, int32_t nbytes); // set the rate of periodic interrupts
extern int32_t rtc_close(int32_t fd); // do nothing and return 0



#endif
