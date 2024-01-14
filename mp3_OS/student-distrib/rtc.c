#include "rtc.h"


volatile int32_t rtc_interrupt_occurred[MAX_NUM];
static int32_t current_freq_cnt[MAX_NUM];
static int16_t expected_freq[MAX_NUM];
static int16_t highest_rate = 6;     // highest rate limited
static int32_t highest_freq = 1024;   // highest frequency = 32768 >> (hightest_rate - 1) = 1024

/* 
 * rtc_init: Enable (unmask) the specified IRQ and set a initial rate
 * Input: none
 * Output: none
 * Return value: none
 * Side effect: turn on bit 6 of register B and set the RATE (15) to register A
*/
void rtc_init(){
    disable_irq(RTC_IRQ); // disable the RTC IRQ line
    outb(RTC_REG_B, RTC_PORT); // select register B, and disable NMI
    char prev = inb(RTC_DATA); // read register B, and the content of register B will be lost
    outb(RTC_REG_B, RTC_PORT); // set the index again
    outb(prev | 0x40, RTC_DATA); // turn on bit 6 of register B
    // read register C to make sure to get another interrupt
    outb(RTC_REG_C & 0X0F, RTC_PORT); // select register C (0x0C)
    inb(RTC_DATA); // read register C, and the content of register C will be lost
    // set rate
    outb(RTC_REG_A, RTC_PORT);		// set index to register A, disable NMI
    prev = inb(RTC_DATA);	// get initial value of register A
    outb(RTC_REG_A, RTC_PORT);		// reset index to A
    outb((prev & 0xF0) | highest_rate, RTC_DATA);  //write only our rate to A. Note, rate is the bottom 4 bits.
    // set values
    int i;
    for(i = 0; i < MAX_NUM; i++){
        rtc_interrupt_occurred[i] = 0;
        current_freq_cnt[i] = 0;
        expected_freq[i] = 1024;
    }
    enable_irq(RTC_IRQ);  // enable the RTC IRQ line
}

/* 
 * rtc_handler: handler for rtc
 * Input: none
 * Output: none
 * Return value: none
 * Side effect: call the test_interrupts to change the character throughout the screen (and sending EOI afterwards)
*/
void rtc_handler(){
    cli();
    current_freq_cnt[0]++;
    if(current_freq_cnt[0] % (highest_freq / expected_freq[0]) == 0){   // expected frequency is x times of highest frequency
        rtc_interrupt_occurred[0] = 1;
        current_freq_cnt[0] = 0;
    }
    current_freq_cnt[1]++;
    if(current_freq_cnt[1] % (highest_freq / expected_freq[1]) == 0){
        rtc_interrupt_occurred[1] = 1;
        current_freq_cnt[1] = 0;
    }
    current_freq_cnt[2]++;
    if(current_freq_cnt[2] % (highest_freq / expected_freq[2]) == 0){
        rtc_interrupt_occurred[2] = 1;
        current_freq_cnt[2] = 0;
    }
    // test_interrupts();
    // read register C after an IRQ8 to happen again
    outb(RTC_REG_C & 0x0F, RTC_PORT); // select register C (0x0C)
    inb(RTC_DATA); // read register C, and the content of register C will be lost
    send_eoi(RTC_IRQ);
    sti();
}

/* 
 * rtc_open: set the interrupt frequency to be 2 Hz
 * Input: filename - not used, should be ignored
 * Output: none
 * Return value: always 0
 * Side effect: set the interrupt frequency to be 2 Hz
*/
int32_t rtc_open(const uint8_t* filename){
    int32_t curr_scheduler = get_curr_scheduler();
    expected_freq[curr_scheduler] = 2;   // set the frequency to 2 Hz
    rtc_interrupt_occurred[curr_scheduler] = 0;
    return 0;
}

/* 
 * rtc_read: set a flag and return after an interrupt
 * Input: fd, buf, nbytes - not used, should be ignored
 * Output: none
 * Return value: always 0
 * Side effect: none
*/
int32_t rtc_read(int32_t fd, void* buf, int32_t nbytes){
    int32_t curr_scheduler = get_curr_scheduler();
    while(!rtc_interrupt_occurred[curr_scheduler]);  // wait for rtc interrupt
    rtc_interrupt_occurred[curr_scheduler] = 0;
    return 0;
}

/* 
 * rtc_write: set the rate of periodic interrupts
 * Input: fd, nbytes - not used, should be ignored
 *        buf - pointer to the interrupt frequency
 * Output: none
 * Return value: -1 if invalid frequency, 1 otherwise (read 1 byte)
 * Side effect: change the interrupt rate
*/
int32_t rtc_write(int32_t fd, const void* buf, int32_t nbytes){
    if(buf == NULL) return -1;
    cli();
    // read the interrupt frequency from buffer
    uint32_t write_freq = *((uint32_t*)buf);
    // check whether the number is valid and a power of two
    if(write_freq < 2 || write_freq > 1024 || (write_freq & (write_freq - 1))){
        sti();
        return -1;
    }
    // set the expected frequency and reset the count to 0
    int32_t curr_scheduler = get_curr_scheduler();
    expected_freq[curr_scheduler] = write_freq;
    // current_freq_cnt = 0;
    sti();
    return 1;
}

/* 
 * rtc_close: do nothing
 * Input: fd - not used, should be ignored
 * Output: none
 * Return value: always 0
 * Side effect: none
*/
int32_t rtc_close(int32_t fd){
    return 0;
}
