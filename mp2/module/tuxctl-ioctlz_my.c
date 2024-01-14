/* tuxctl-ioctl.c
 *
 * Driver (skeleton) for the mp2 tuxcontrollers for ECE391 at UIUC.
 *
 * Mark Murphy 2006
 * Andrew Ofisher 2007
 * Steve Lumetta 12-13 Sep 2009
 * Puskar Naha 2013
 */

#include <asm/current.h>
#include <asm/uaccess.h>

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/file.h>
#include <linux/miscdevice.h>
#include <linux/kdev_t.h>
#include <linux/tty.h>
#include <linux/spinlock.h>

#include "tuxctl-ld.h"
#include "tuxctl-ioctl.h"
#include "mtcp.h"

#define debug(str, ...) \
	printk(KERN_DEBUG "%s: " str, __FUNCTION__, ## __VA_ARGS__)


//mp2.2

static int ack = 1;
static spinlock_t button_lock;
static unsigned button_state;
static char led_buffer[4] = {0, 0, 0, 0};		//led buffer for 4 led
static unsigned long led_store = 0;				//led argument
//turn display to hex			 				0    1     2     3     4     5     6     7     8     9     A     B     C     D     E     F
const static unsigned char display_hex[16] ={0xE7, 0x06, 0xCB, 0x8F, 0x2E, 0xAD, 0xED, 0x86, 0xEF, 0xAF, 0xEE, 0x6D, 0xE1, 0x4F, 0xE9, 0xE8};	//7-segment display
/************************ Protocol Implementation *************************/

//reset handler: restore the controller state (value displayed on LEDs, button interrupt generation, etc.)
void reset_handler(struct tty_struct* tty)
{
	tux_init(tty);
	// char cmd_buffer[6] = {MTCP_LED_SET, 0xF, 0, 0, 0, 0};
	// int i;
	// for(i=0; i<4; i++)								//four led
	// {
	// 	cmd_buffer[i+2] = led_buffer[i];			//restore state of led
	// }
	// tuxctl_ldisc_put(tty, cmd_buffer, 6);			//send command to tux
	// return;						
	tux_set_led(tty, led_store);
	return;					
}
//reset handler: restore the controller state (value displayed on LEDs, button interrupt generation, etc.)
// void reset_handler(struct tty_struct* tty)
// {
// 	int i;
// 	// char cmd_buffer[8] = {MTCP_BIOC_ON, MTCP_LED_USR, MTCP_LED_SET, 0xF, 0, 0, 0, 0};;		//cmd buffer
// 	char cmd_buffer[6] = {MTCP_LED_SET, 0xF, 0, 0, 0, 0};
	
// 	for(i=0; i<4; i++)								//four led
// 	{
// 		cmd_buffer[i+2] = led_buffer[i];			//restore state of led
// 	}
// 	if(ack == 0)	return ;						//need response ack
// 	// ack = 0;										//reset ack to 0
// 	tuxctl_ldisc_put(tty, cmd_buffer, 6);			//send command to tux
// 	return;										// the ioctl call should return 0
// }


//bioc event handler
void bioc_event_handler(unsigned b, unsigned c)
{
// ;		byte 1  +-7-----4-+-3-+-2-+-1-+---0---+
// ;			| 1 X X X | C | B | A | START |
// ;			+---------+---+---+---+-------+
// ;		byte 2  +-7-----4-+---3---+--2---+--1---+-0--+
// ;			| 1 X X X | right | down | left | up |
// ;			+---------+-------+------+------+----+
	unsigned long flags;
	unsigned bit_l, bit_d;

	// if(b == 0x80 && c == 0x80)	return;	
	spin_lock_irqsave(&button_lock, flags);	//acquire spinlock
	//store in button_state: R L D U C B A S
	
	button_state = b & 0xF;	//CBAS

	button_state |= (c & 0xF) << 4;	//RDLU, switch LD so we get what we want
	bit_l = c & 0x2;	//left bit
	bit_d = c & 0x4;	//down bit
	button_state &= 0x9F;	//clear LD
	button_state |= (bit_l << 5);	//set left bit
	button_state |= (bit_d << 3);	//set down bit
	spin_unlock_irqrestore(&button_lock, flags);	//release spinlock
	
}
/* tuxctl_handle_packet()
 * IMPORTANT : Read the header for tuxctl_ldisc_data_callback() in 
 * tuxctl-ld.c. It calls this function, so all warnings there apply 
 * here as well.
 * 
 * "This is how responses to polling the buttons
 * and ACK's for setting the LEDs will be transmitted to the tuxctl driver."
 * 
 * IMPORTANT: This function is called from an interrupt context, so it 
 *            cannot acquire any semaphores or otherwise sleep, or access
 *            the 'current' pointer. It also must not take up too much time.
 */
void tuxctl_handle_packet (struct tty_struct* tty, unsigned char* packet)
{
    unsigned a, b, c;

    a = packet[0]; /* Avoid printk() sign extending the 8-bit */
    b = packet[1]; /* values when printing them. */
    c = packet[2];

	//tux->pc
	switch (a)
	{
		case MTCP_ACK:		//Generated when the Button Interrupt-on-change mode is enabled and a button is either pressed or released.
			ack = 1;
			return;
		case MTCP_BIOC_EVENT:
			bioc_event_handler(b, c);
			return;
		case MTCP_RESET:
			reset_handler(tty);
			return;
		default:
			return;
	}

    /*printk("packet : %x %x %x\n", a, b, c); */
}





// Initializes any variables associated with the driver and returns 0
int tux_init(struct tty_struct* tty)
{
	unsigned char cmd_buffer[2];	//command
	ack = 0;	//initialize ack to 0
	cmd_buffer[0] = MTCP_BIOC_ON;	//enable button interrupt-on-change
	cmd_buffer[1] = MTCP_LED_USR;	//set LED to user mode
	tuxctl_ldisc_put(tty, cmd_buffer, 2);	//send command to tux
	button_state = 0x0;	//initialize button state to 0x0
	button_lock = SPIN_LOCK_UNLOCKED;	//initialize spinlock
	return 0;
}

// Takes a pointer to a 32-bit integer. Returns -EINVAL error if this pointer is not valid. Otherwise,
// sets the bits of the low byte corresponding to the currently pressed button
int tux_buttons(unsigned long* arg)
{
	unsigned long flags;
	if(arg == NULL)	//check if pointer is valid
		return -EINVAL;
	spin_lock_irqsave(&button_lock, flags);		//acquire spinlock
	*arg = button_state;	//set bits of low byte corresponding to currently pressed button
	// *arg = button_state;
	spin_unlock_irqrestore(&button_lock, flags);	//release spinlock
	return 0;
}

int tux_set_led(struct tty_struct* tty, unsigned long arg)
{
	int i;
	char cmd_buffer[6] = {MTCP_LED_SET, 0xF, 0, 0, 0, 0};		//cmd buffer
	int display_num = arg & 0xFFFF;			//display low 16 bits on 7-segment display
	int led_turn_on = (arg >> 16) & 0xF;	//low 4 bits of the third byte indicate which LEDs to turn on
	int decimal_point = (arg >> 24) & 0xF;	//low 4 bits of the fourth byte specify whether the corresponding decimal point should be turned on
	
	led_store = arg;	//store arg
	//update led_buffer according to display_num
	uint32_t bitmask = 0xF;								//mask for the last 4 bits
	for(i=0; i<4; i++)									//four led
	{
		led_buffer[i] = display_hex[display_num & bitmask];	//get the corresponding hex value
		display_num = display_num >> 4;						//shift right 4 bits
	}
	//update cmd_buffer according to led_turn_on and decimal_point
	for(i=0; i<4; i++)								//four led
	{
		if(led_turn_on & (0x1 << i))				//check if the corresponding LED should be turned on
		{
			cmd_buffer[i+2] = led_buffer[i];		//add 2 for the first two bytes in cmd_buffer
		}
		if(decimal_point & (0x1 << i))				//check if the corresponding decimal point should be turned on
		{
			cmd_buffer[i+2] |= 0x10;				//the fouth bit is dp
		}
	}
	if(ack == 0)	return 0;						//need response ack
	tuxctl_ldisc_put(tty, cmd_buffer, 6);			//send command to tux
	ack = 0;										//reset ack to 0
	return 0;										// the ioctl call should return 0
}

/******** IMPORTANT NOTE: READ THIS BEFORE IMPLEMENTING THE IOCTLS ************
 *                                                                            *
 * The ioctls should not spend any time waiting for responses to the commands *
 * they send to the controller. The data is sent over the serial line at      *
 * 9600 BAUD. At this rate, a byte takes approximately 1 millisecond to       *
 * transmit; this means that there will be about 9 milliseconds between       *
 * the time you request that the low-level serial driver send the             *
 * 6-byte SET_LEDS packet and the time the 3-byte ACK packet finishes         *
 * arriving. This is far too long a time for a system call to take. The       *
 * ioctls should return immediately with success if their parameters are      *
 * valid.                                                                     *
 *                                                                            *
 ******************************************************************************/
int 
tuxctl_ioctl (struct tty_struct* tty, struct file* file, 
	      unsigned cmd, unsigned long arg)
{
    switch (cmd) {
	case TUX_INIT:
		return tux_init(tty);
	case TUX_BUTTONS:
		return tux_buttons((unsigned long*)arg);
	case TUX_SET_LED:
		return tux_set_led(tty, arg);
	case TUX_LED_ACK:
		return -EINVAL;
	case TUX_LED_REQUEST:
		return -EINVAL;
	case TUX_READ_LED:
		return -EINVAL;
	default:
	    return -EINVAL;
    }
}
