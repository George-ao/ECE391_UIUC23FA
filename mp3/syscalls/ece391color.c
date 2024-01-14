#include <stdint.h>

#include "ece391support.h"
#include "ece391syscall.h"

int main ()
{
    uint8_t buf[1024];
    uint8_t buf_arg[2];
    // buf_arg[0] - 0 is background, 1 is text
    // buf_arg[1] - color
    if (0 != ece391_getargs (buf, 1024)) {
        ece391_ioctl(0, 0);
	    return 0;
    }
    if(buf[1]!=' '||buf[3]!='\0'){
        ece391_fdputs (1, (uint8_t*)"Color could not read the arguments\n");
        return 0;
    }
    
    buf_arg[0] = buf[0];
    buf_arg[1] = buf[2];

    if (0 != ece391_ioctl(1, (unsigned long)buf_arg)) {
        ece391_fdputs (1, (uint8_t*)"fail to change, please try again\n");
	    return 3;
    }
    
    return 0;
}
