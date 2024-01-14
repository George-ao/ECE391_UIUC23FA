/* i8259.c - Functions to interact with the 8259 interrupt controller
 * vim:ts=4 noexpandtab
 */

#include "i8259.h"
#include "lib.h"

#define CMD_MASTER MASTER_8259_PORT
#define DATA_MASTER 1+MASTER_8259_PORT
#define CMD_SLAVE SLAVE_8259_PORT
#define DATA_SLAVE 1+SLAVE_8259_PORT

/* Interrupt masks to determine which interrupts are enabled and disabled */
uint8_t master_mask; /* IRQs 0-7  */
uint8_t slave_mask;  /* IRQs 8-15 */

/* 
 * i8259_init: initialize the PIC with port of slave and master set 
 * Input: none
 * Output: none
 * Side effect: enabling the port 2 on master PIC
*/

void i8259_init(void) {
    /* initialize the masks*/
    outb(0xFF, DATA_MASTER);
    outb(0xFF, DATA_SLAVE);
    /* sequencial initialization*/
    master_mask=0xFF;
    slave_mask=0xFF;
    /* ICW1:
    *   set the start initialization, the edge triggered mode
    *   the cascade mode, and 4 ICWs
    */
    outb(ICW1, CMD_MASTER);
    outb(ICW1, CMD_SLAVE);
    /* ICW2:
    *   high bits are the interrupt vector
    *
    */
    outb(ICW2_MASTER, DATA_MASTER);
    outb(ICW2_SLAVE, DATA_SLAVE);
    /* ICW3:
    *  connect the slave to the master
    *
    */
    outb(ICW3_MASTER, DATA_MASTER);
    outb(ICW3_SLAVE, DATA_SLAVE);
     /* ICW4: 
    *   set the bit 0 to 1 to make ISA=80x86 mode, instead of 8080 mode
    *   auto EOI mode when 1, normal EOI mode when 0
    */
    outb(ICW4, DATA_MASTER);
    outb(ICW4, DATA_SLAVE);
    enable_irq(2); // enable the port 2 on Master PIC 
    
}

/* 
 * enable_irq: Enable (unmask) the specified IRQ 
 * Input: irq_num: the irq number
 * Output: none
 * Side effect: unmask (enable) the specified irq
*/
void enable_irq(uint32_t irq_num) {
    if(irq_num<8){ // master PIC
        master_mask&=~(1<<irq_num);
        outb(master_mask, DATA_MASTER); // write to the data port
    }
    else{
        if(irq_num>15)
            return; // invalid irq_num
        slave_mask&=~(1<<(irq_num-8)); // slave PIC
        outb(slave_mask, DATA_SLAVE);
    }
    
}

/* 
 * disable_irq: Disable (mask) the specified IRQ 
 * Input: irq_num: the irq number
 * Output: none
 * Side effect: mask (disable) the specified irq
 */
void disable_irq(uint32_t irq_num) {
    if(irq_num<8){ // master PIC
        master_mask|=(1<<irq_num);
        outb(master_mask, DATA_MASTER); // write to the data port
    }
    else{
        if(irq_num>15)
            return; // invalid irq_num
        slave_mask|=(1<<(irq_num-8)); // slave PIC
        outb(slave_mask, DATA_SLAVE);
    }

}

/* 
 * send_eoi: Send end-of-interrupt signal for the specified IRQ, so that future interrupt can be run
 * Input: irq_num: the irq number
 * Output: none
 * Side effect: send the EOI signal to the PIC and allow the next interrupt
 */
void send_eoi(uint32_t irq_num) {
    if(irq_num<8){ // master PIC
        outb(EOI|irq_num, CMD_MASTER); // write to the command port
    }
    else{
        // slave PIC 
        if(irq_num>15)
            return; // invalid irq_num
        outb(EOI|(irq_num-8), CMD_SLAVE); // slave PIC
        outb(EOI|2, CMD_MASTER); // write to the command port
    }
}
