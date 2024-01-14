#ifndef _KEYBOARD_H
#define _KEYBOARD_H
#include "types.h"
#include "i8259.h"
#include "lib.h"
#include "terminal.h"
#include "signal.h"

#define KEYBOARD_IRQ 1
#define KEYBOARD_DATA 0x60

#define SCANCODE_SET_SIZE 59

#define PRESSED_F1 0x3B
#define PRESSED_F2 0x3C
#define PRESSED_F3 0x3D
#define PRESSED_F4 0x3E


#define PRESSED_U 0x48 // NUMPAD 8
#define PRESSED_D 0x50 // NUMPAD 2
#define PRESSED_L 0x4B // NUMPAD 4
#define PRESSED_R 0x4D // NUMPAD 6


#define PRESSED_BACKSPACE 0x0E
#define PRESSED_ENTER 0x1C

#define PRESSED_CTRL 0x1D
#define RELEASE_CTRL 0x9D

#define PRESSED_LSHIFT 0x2A
#define RELEASE_LSHIFT 0xAA

#define PRESSED_RSHIFT 0x36
#define RELEASE_RSHIFT 0xB6

#define PRESSED_ALT 0x38
#define RELEASE_ALT 0xB8


#define PRESSED_CAPSLOCK 0x3A
#define PRESSED_NUMLOCK 0x45

// handler for keyboard
extern void keyboard_handler(); 
// initilize the keyboard
extern void keyboard_init(); 
// helper function for checking whether we're printing a alphabet or not (use for shift/capslock cases)
int alphabet_check(uint8_t scancode);


// uint32_t last_modify_length = 0;
#endif
