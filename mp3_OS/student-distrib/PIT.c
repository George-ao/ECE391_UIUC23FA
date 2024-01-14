#include "PIT.h"
#include "scheduler.h"

/* 
 * PIT_init
 *   DESCRIPTION: Initialize the PIT
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: Initialize the PIT
 */
void PIT_init(){
    outb(PIT_MODE, PIT_COMMAND_PORT); // set the PIT mode
    outb(PIT_FREQ & PIT_MASK, PIT_DATA_PORT); // set the PIT frequency
    outb(PIT_FREQ >> 8, PIT_DATA_PORT); // set the PIT frequency
    enable_irq(PIT_IRQ); // enable the PIT IRQ
}


/* 
 * PIT_handler
 *   DESCRIPTION: Handler for PIT
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: Send EOI to PIT IRQ
 */
void PIT_handler(){
    send_eoi(PIT_IRQ);
    cli();
    switch_schedule();
    sti();
}
