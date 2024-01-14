#include <stdint.h>

#include "ece391support.h"
#include "ece391syscall.h"

int main ()
{
    int32_t fd, cnt;
    uint8_t buf[128];
    uint8_t buf2[128];
    uint8_t buf3[128];
    // uint8_t buf4[128];
    uint32_t i, j, bytes_write;
    i = 0;
    if (0 != ece391_getargs (buf, 128)) 
    {
        ece391_fdputs (1, (uint8_t*)"could not read arguments\n");
	    return 3;
    }

    //buf2 is the file name
    //buf3 is the string to write

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
    // ece391_fdputs (1, buf);
    // ece391_fdputs (1, '\n');
    // ece391_fdputs (1, buf2);
    // ece391_fdputs (1, '\n');
    // ece391_fdputs (1, buf3);
    //open the file
    if (-1 == (fd = ece391_open (buf2))) 
    {
        ece391_fdputs (1, (uint8_t*)"file not exists\n");
	    return 2;
    }

    if (j==0) return 0;
    bytes_write = j;
    //write to the file
    //read the first file to a buf
    if (0 != (cnt = ece391_write (fd, buf3, bytes_write))) 
    {
        if (-1 == cnt) 
        {
	    ece391_fdputs (1, (uint8_t*)"file write failed\n");
	    return 3;
        }
        //wrtie success
        if (cnt >= 0) return 0;
    }
    return 0;
}

