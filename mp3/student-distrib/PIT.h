#ifndef _PIT_H
#define _PIT_H

#include "types.h"
#include "i8259.h"
#include "lib.h"

#define PIT_IRQ 0
#define PIT_DATA_PORT 0x40
#define PIT_COMMAND_PORT 0x43
#define PIT_MODE 0x36
#define PIT_FREQ 1193182
#define PIT_MASK 0xFF


void PIT_init();
void PIT_handler();

#endif
