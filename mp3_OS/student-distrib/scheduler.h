#ifndef _SCHEDULER_H
#define _SCHEDULER_H

#include "lib.h"

#define MAX_NUM 3
#define INITIALIZATION_REQUIRED -256

void scheduler_init();
void switch_schedule();
extern int32_t scheduler_queue[MAX_NUM];

#endif
