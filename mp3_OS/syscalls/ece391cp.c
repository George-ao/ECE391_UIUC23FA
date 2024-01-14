#include <stdint.h>

#include "ece391support.h"
#include "ece391syscall.h"

int main ()
{
    int32_t fd1, fd2, cnt;
    uint8_t buf[1024];
    uint8_t buf2[1024];
    uint8_t buf3[1024];
    uint8_t buf4[10000];
    uint32_t i, j;
    i = 0;
    if (0 != ece391_getargs (buf, 1024)) 
    {
        ece391_fdputs (1, (uint8_t*)"could not read arguments\n");
	    return 3;
    }
    //sepate the two file name in the buf into buf2 and buf3
    while (buf[i] != ' ')
    {
        buf2[i] = buf[i];
        i++;
    }
    buf2[i] = '\0';
    i++;
    j=0;
    while (buf[i] != '\0')
    {
        buf3[j] = buf[i];
        i++;
        j++;
    }
    buf3[j] = '\0';

    //
    // ece391_fdputs (1, buf2);
    // ece391_fdputs (1, buf3);
    // ece391_fdputs (1, buf4);
    //
    //open both files
    if (-1 == (fd1 = ece391_open (buf2))) 
    {
        ece391_fdputs (1, (uint8_t*)"file not exists\n");
	    return 2;
    }
    if (-1 == (fd2 = ece391_open (buf3))) 
    {
        ece391_fdputs (1, (uint8_t*)"file not exists\n");
	    return 2;
    }
    //read the first file to a buf
    if (0 != (cnt = ece391_read (fd1, buf4, 10000))) 
    {
        if (-1 == cnt) 
        {
	    ece391_fdputs (1, (uint8_t*)"file read failed\n");
	    return 3;
        }
        //write to the second file
        if (-1 == ece391_write (fd2, buf4, cnt)) 
        {
            ece391_fdputs (1, (uint8_t*)"file write failed\n");
            return 3;
    }
	}


    return 0;
}

