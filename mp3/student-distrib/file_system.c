#include "file_system.h"
#include "keyboard.h"


// global variables
boot_block_t*   boot_block_ptr;
dentry_t*       dentry_ptr;
inode_t*        inode_ptr;
data_block_t*   data_block_ptr;
// bitmap for data blocks
uint8_t bitmap[128];             // 59 data blocks
uint32_t bitmap_counter;
uint8_t inodemap[128];        // 64 inodes
uint32_t inodemap_counter;
uint32_t dir_num;           //change !!!
uint32_t in_num;            //not change !!!
uint32_t db_num;            //not change !!!

uint32_t last_modify_length = 0;
uint32_t search_list_index;
/* file_system_init
 *   DESCRIPTION: Initialize the file system
 *   INPUTS: start_addr -- the starting address of the file system
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
void file_system_init (uint32_t start_addr)
{
    uint32_t i, j, inode_num_, max_db;

    boot_block_ptr = (boot_block_t*)start_addr;
    dir_num = boot_block_ptr->num_dir_entries;  // de_num: 18
    in_num = boot_block_ptr->num_inodes;        // in_num: 64
    db_num = boot_block_ptr->num_data_blocks;   // db_num: 59

    dentry_ptr = (dentry_t*)(start_addr + DENTRY_OFFSET);
    inode_ptr = (inode_t*)(start_addr + BOOT_BLOCK_SIZE);
    data_block_ptr = (data_block_t*)(start_addr + BOOT_BLOCK_SIZE + INODE_SIZE * boot_block_ptr->num_inodes);
    // initialize inodemap
    for (i=0; i<in_num; i++)
    {
        inodemap[i] = 0;
        inodemap_counter = 0;
    }
    // initialize bitmap
    for (i=0; i<db_num; i++)
    {
        bitmap[i] = 0;
        bitmap_counter = 0;
    }
    for (i=0; i<dir_num; i++)
    {
        //get inode number
        inode_num_ = dentry_ptr[i].inode_num;
        //set inode bitmap to 1
        inodemap[inode_num_] = 1;
        inodemap_counter++;
        //get all data block number in inode and set bitmap to 1
        max_db = inode_ptr[inode_num_].length_in_B/BLOCK_SIZE;
        for (j=0; j<=max_db; j++)
        {
            bitmap[inode_ptr[inode_num_].data_blocks[j]] = 1;
            bitmap_counter++;
        }
    }

}


/* read_dentry_by_name
 *   DESCRIPTION: Read the directory entry by name
 *   INPUTS: fname -- the file name
 *           dentry -- the directory entry
 *   OUTPUTS: none
 *   RETURN VALUE: 0 on success, -1 on failure
 *   SIDE EFFECTS: none
 */
int32_t read_dentry_by_name (const uint8_t* fname, dentry_t* dentry)
{
    int i;
    if (fname == NULL || dentry == NULL)    // check for null pointers
        return -1;
    if (strlen((int8_t*)fname) > FILENAME_MAX_SIZE)        // check for invalid length
        return -1;

    for (i=0; i<boot_block_ptr->num_dir_entries; i++)
    {
        if (strncmp((int8_t*)fname, (int8_t*)boot_block_ptr->dir_entries[i].file_name, FILENAME_MAX_SIZE) == 0)
        {
            // dentry->file_name = boot_block_ptr->dir_entries[i].file_name;
            strncpy((int8_t*)dentry->file_name, (int8_t*)boot_block_ptr->dir_entries[i].file_name, FILENAME_MAX_SIZE);  //copy dentry
            dentry->file_type = boot_block_ptr->dir_entries[i].file_type;
            dentry->inode_num = boot_block_ptr->dir_entries[i].inode_num;
            return 0;
        }
    }
    // If the file name not found, return -1
    return -1;
}

int32_t return_dentry_index (const uint8_t* fname, dentry_t* dentry)
{
    int i;
    if (fname == NULL || dentry == NULL)    // check for null pointers
        return -1;
    if (strlen((int8_t*)fname) > FILENAME_MAX_SIZE)        // check for invalid length
        return -1;

    for (i=0; i<boot_block_ptr->num_dir_entries; i++)
    {
        if (strncmp((int8_t*)fname, (int8_t*)boot_block_ptr->dir_entries[i].file_name, FILENAME_MAX_SIZE) == 0)
        {
            return i;
        }
    }
    // If the file name not found, return -1
    return -1;
}
/* read_dentry_by_index
 *   DESCRIPTION: Read the directory entry by index
 *   INPUTS: index -- the index of the directory entry
 *           dentry -- the directory entry
 *   OUTPUTS: none
 *   RETURN VALUE: 0 on success, -1 on failure
 *   SIDE EFFECTS: none
 */
int32_t read_dentry_by_index (uint32_t index, dentry_t* dentry)
{
    // int i;
    if (index < 0 || index >= boot_block_ptr->num_dir_entries)      // check for invalid index
        return -1;
    if (dentry == NULL)                                             // check for null pointer
        return -1;
    
    // If valid parameters
    // dentry->file_name = boot_block_ptr->dir_entries[index].file_name;
    strncpy((int8_t*)dentry->file_name, (int8_t*)boot_block_ptr->dir_entries[index].file_name, FILENAME_MAX_SIZE);      //copy dentry
    dentry->file_type = boot_block_ptr->dir_entries[index].file_type;
    dentry->inode_num = boot_block_ptr->dir_entries[index].inode_num;
    
    return 0;
}

/* read_directory
 *   DESCRIPTION: Read the directory
 *   INPUTS: 
 *           buf -- the buffer to store the data
 *           index -- index of the dentry node
 *   OUTPUTS: none
 *   RETURN VALUE: 0
 *   SIDE EFFECTS: none
 */
int32_t read_directory (uint8_t* buf, uint32_t index)
{
    dentry_t dentry;
    int32_t i;
    int32_t count;
    if (index == boot_block_ptr->num_dir_entries)       // check for invalid index
        return 0;
    if (read_dentry_by_index(index, &dentry) == -1)
        return -1;                                      // return 0 if reached to the end
    count = 0;
    for(i=0; i<FILENAME_MAX_SIZE;i++)                             
    {
        if (dentry.file_name[i] == '\0')                // if the file name is shorter than 32 bytes, fill the rest with 0
        {
            buf[i]='\0';
            break;
        }
        buf[i]=dentry.file_name[i];
        count++;
    }
    return count;                                       // return 40 bytes, the full buffer size
    
    // uint32_t bitmask = 0xFF000000;                   // bitmask: get one byte each time  
    // for(i=32; i< 36;i++)                             // FILENAME_MAX_SIZE=32;  FILENAME_MAX_SIZE+FILETYPE_SIZE=36
    // {
    //     buf[i]=(dentry.file_type & bitmask) >> (24-8*(i-32));           // trick that traverse each byte from left to right
    //     bitmask = bitmask >> 8;                                         // right shift one byte
    // }
    // bitmask = 0xFF000000;                           // bitmask: get one byte each time  
    // for(i=36; i< 40; i++)                           // FILENAME_MAX_SIZE+FILETYPE_SIZE+LENGTH_SIZE =40
    // {
    //     buf[i]=(inode_ptr[dentry.inode_num].length_in_B & bitmask) >> (24-8*(i-36));        ////trick that traverse each byte from left to right
    //     bitmask = bitmask >> 8;                                        //right shift one byte
    // }
}

/* read_data
 *   DESCRIPTION: Read the data
 *   INPUTS: inode -- the inode number
 *           offset -- the offset of the data
 *           buf -- the buffer to store the data
 *           length -- the length of the data
 *   OUTPUTS: none
 *   RETURN VALUE: the number of bytes read
 *   SIDE EFFECTS: none
 */
int32_t read_data (uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t length)
{

    uint32_t start_block_idx, end_block_idx;
    uint32_t start_block_offset, end_block_offset;
    uint32_t counter, i;
    uint32_t read_limit;

    if (inode < 0 || inode >= boot_block_ptr->num_inodes)       // check for invalid inode
        return -1;
    if (buf == NULL)                                            // check for null pointer
        return -1;
    if (offset < 0 || offset > inode_ptr[inode].length_in_B)              // check for invalid offset
        return -1;
    if (offset == inode_ptr[inode].length_in_B)             // check for end of file
        return 0;

    if (offset+length > inode_ptr[inode].length_in_B)        // check for invalid length
        read_limit = inode_ptr[inode].length_in_B - offset;
    else
        read_limit = length;

    // If valid parameters
    start_block_idx = offset / BLOCK_SIZE;
    end_block_idx = (offset + read_limit - 1) / BLOCK_SIZE;
    start_block_offset = offset % BLOCK_SIZE;
    end_block_offset = (offset + read_limit - 1) % BLOCK_SIZE;

    for (i=start_block_idx; i<=end_block_idx; i++)
    {
        if (inode_ptr[inode].data_blocks[i] < 0 || inode_ptr[inode].data_blocks[i] >= boot_block_ptr->num_data_blocks)       // check for invalid data block
            // clean buffer
            return -1;
    }

    // If valid data blocks
    // if start block and end block are the same
    if (start_block_idx == end_block_idx)
    {
        // same_flag = 1;
        for (counter=0; counter<read_limit; counter++)
        {
            buf[counter] = data_block_ptr[inode_ptr[inode].data_blocks[start_block_idx]]
                            .data[start_block_offset + counter];
        }
        return counter;
    }
    // if start block and end block are different
    else
    {
        // same_flag = 0;
        for (counter=0; counter<BLOCK_SIZE-start_block_offset; counter++)
        {
            buf[counter] = data_block_ptr[inode_ptr[inode].data_blocks[start_block_idx]]
                            .data[start_block_offset + counter];
        }
    }

    // middle blocks
    for (i=start_block_idx+1; i<end_block_idx; i++)
    {
        memcpy(buf+counter, data_block_ptr[inode_ptr[inode].data_blocks[i]].data, BLOCK_SIZE);
        counter += BLOCK_SIZE;
    }

    // end block
    for (i=0; i<end_block_offset; i++)
    {
        buf[counter] = data_block_ptr[inode_ptr[inode].data_blocks[end_block_idx]]
                        .data[i];
        counter++;
    }
    
    return counter;
}


/* -------------------- System call functions -------------------- */

/* file_open
 *   DESCRIPTION: Open the file
 *   INPUTS: filename -- the file name
 *   OUTPUTS: none
 *   RETURN VALUE: 0 on success, -1 on failure
 *   SIDE EFFECTS: none
 */

int32_t file_open (const uint8_t* filename)
{
    dentry_t dentry_new;
    if (read_dentry_by_name(filename, &dentry_new) == -1)
        return -1;
    return 0;
}

/* file_close
 *   DESCRIPTION: Close the file
 *   INPUTS: fd -- the file descriptor
 *   OUTPUTS: none
 *   RETURN VALUE: 0 on success, -1 on failure
 *   SIDE EFFECTS: none
 */
int32_t file_close (int32_t fd)
{
    return 0;
}

/* file_read
 *   DESCRIPTION: Read the file
 *   INPUTS: fd -- the file descriptor
 *           buf -- the buffer to store the data
 *           nbytes -- the number of bytes to read
 *   OUTPUTS: none
 *   RETURN VALUE: the number of bytes read
 *   SIDE EFFECTS: none
 */
int32_t file_read (int32_t fd, void* buf, int32_t nbytes)
{
    int32_t pid = get_pid();
    process_control_block_t* pcb = get_pcb_by_pid(pid);
    int32_t bytes_read;
    bytes_read = read_data(pcb->fds[fd].inode, 
            pcb->fds[fd].file_position, buf, nbytes);

    if (bytes_read == -1)           // if read_data fails, return -1
        return -1;
    pcb->fds[fd].file_position += bytes_read;
    return bytes_read;              // otherwise return number of bytes read
}

/* file_write
 *   DESCRIPTION: Write the file
 *   INPUTS: fd -- the file descriptor
 *           buf -- the buffer to store the data
 *           nbytes -- the number of bytes to write
 *   OUTPUTS: none
 *   RETURN VALUE: -1
 *   SIDE EFFECTS: none
 */
int32_t file_write (int32_t fd, const void* buf, int32_t nbytes)
{
    //check if there is enough space to write
    int32_t pid = get_pid();
    process_control_block_t* pcb = get_pcb_by_pid(pid);
    int32_t bytes_write;
    bytes_write = write_data(pcb->fds[fd].inode, 
            (uint8_t*)buf, nbytes);

    if (bytes_write == -1)           // if read_data fails, return -1
        return -1;
    pcb->fds[fd].file_position += bytes_write;
    return bytes_write;              // otherwise return number of bytes read
}

//overwrite previous data-- write from the beginning of the file
int32_t write_data (uint32_t inode, uint8_t* buf, uint32_t length)
{
    //free the previous data block and inode
    int32_t i, j, used_db_num, length_written,inode_find, bytes_in_last_block;
//---------------------------------
    //free data block
    if(inode_ptr[inode].length_in_B != 0)
    {
        if(inode_ptr[inode].length_in_B % BLOCK_SIZE ==0 )
        {
            used_db_num = inode_ptr[inode].length_in_B/BLOCK_SIZE;
        }
        else 
        {
            used_db_num = inode_ptr[inode].length_in_B/BLOCK_SIZE + 1;
        }
            //update db bitmap
        for(i=0; i < used_db_num; i++)
        {
            bitmap[inode_ptr[inode].data_blocks[i]] = 0;
            bitmap_counter--;
        }
            //update inode legnth
        inode_ptr[inode].length_in_B = 0;
    }
//---------------------------------
    //write new data
    length_written = length;    
    if (length_written ==0) return 0;
    if(length_written % BLOCK_SIZE ==0 )
    {
        used_db_num = length_written/BLOCK_SIZE;
    }
    else 
    {
        used_db_num = length_written/BLOCK_SIZE + 1;
    }
    bytes_in_last_block = length_written % BLOCK_SIZE;
    if(bytes_in_last_block == 0) bytes_in_last_block = BLOCK_SIZE;

    //check there is an inode and an data block available and check there is dir entry available
    if(inodemap_counter >= in_num || bitmap_counter >= db_num || dir_num >= MAX_DIR_ENTRIES)
        return -1;
    //check enough db
    if(used_db_num > db_num - bitmap_counter)
        return -1;
    //start writing
    inode_find = inode;
    //write db except the last one
    for(i=0; i<used_db_num-1; i++)
    {
        //find db in each loop
        for(j=0; j<db_num; j++)
        {
            if(bitmap[j] == 0)
            {
                bitmap[j] = 1;
                bitmap_counter++;
                (inode_ptr[inode_find].data_blocks)[i] = j;
                break;
            }
        }
        // memcpy(data_block_ptr[inode_ptr[inode_find].data_blocks[i]].data, ((uint8_t*)buf)+i*BLOCK_SIZE, BLOCK_SIZE);
        for (j=0; j<BLOCK_SIZE; j++)
        {
            data_block_ptr[(inode_ptr[inode_find].data_blocks)[i]].data[j] = ((uint8_t*)buf)[i*BLOCK_SIZE+j];
        }
    }
    //write the last data block
    for(j=0; j<db_num; j++)
    {
        if(bitmap[j] == 0)
        {
            bitmap[j] = 1;
            bitmap_counter++;
            (inode_ptr[inode_find].data_blocks)[used_db_num-1] = j;
            break;
        }
    }
    for (i=0; i<bytes_in_last_block; i++)
    {
        data_block_ptr[(inode_ptr[inode_find].data_blocks)[used_db_num-1]].data[i] = ((uint8_t*)buf)[(used_db_num-1)*BLOCK_SIZE+i];
    }
    inode_ptr[inode_find].length_in_B = length_written;
    return length_written;
}

/* dir_open
 *   DESCRIPTION: Open the directory
 *   INPUTS: filename -- the file name
 *   OUTPUTS: none
 *   RETURN VALUE: 0 on success, -1 on failure
 *   SIDE EFFECTS: none
 */
int32_t dir_open (const uint8_t* filename)
{
    dentry_t dentry_new;
    if (read_dentry_by_name(filename, &dentry_new) == -1)
        return -1;
    return 0;
}

/* dir_close
 *   DESCRIPTION: Close the directory
 *   INPUTS: fd -- the file descriptor
 *   OUTPUTS: none
 *   RETURN VALUE: 0 on success, -1 on failure
 *   SIDE EFFECTS: none
 */
int32_t dir_close (int32_t fd)
{
    return 0;
}

/* dir_read
 *   DESCRIPTION: Read the directory
 *   INPUTS: fd -- the file descriptor
 *           buf -- the buffer to store the data
 *           nbytes -- the number of bytes to read
 *   OUTPUTS: none
 *   RETURN VALUE: the number of bytes read
 *   SIDE EFFECTS: none
 */
int32_t dir_read (int32_t fd, void* buf, int32_t nbytes)
{
    int32_t pid = get_pid();
    process_control_block_t* pcb = get_pcb_by_pid(pid);
    int32_t cnt;
    cnt = read_directory(buf, pcb->fds[fd].file_position);
    if (cnt == -1)
        return -1;
    pcb->fds[fd].file_position++;
    return cnt;
}

/* dir_write
 *   DESCRIPTION: Write the directory
 *   INPUTS: fd -- the file descriptor
 *           buf -- the buffer to store the data
 *           nbytes -- the number of bytes to write
 *   OUTPUTS: none
 *   RETURN VALUE: -1
 *   SIDE EFFECTS: none
 */
int32_t dir_write (int32_t fd, const void* buf, int32_t nbytes)
{
    int32_t i, j, cnt;
    //check there is an inode and an data block available and check there is dir entry available
    // int32_t pid = get_pid();
    // process_control_block_t* pcb = get_pcb_by_pid(pid);
    if(inodemap_counter >= in_num || bitmap_counter >= db_num || dir_num >= MAX_DIR_ENTRIES)
        return -1;
    //if buf contain more than 32 bytes, return -1 nbytes are not valid
    for (cnt = 0; cnt < 1024; cnt++)
    {
        if(((uint8_t*)buf)[cnt] == '\0')
            break;
        if(cnt == FILENAME_MAX_SIZE)
            return -1;
    }

    // if nbytes is -299, it means we want to remove the file
    if(nbytes == -299)
    {
        // remove the dir entry
        //if it is the last dir entry, just set the inode bitmap to 0
        dentry_t rm_dentry;
        int32_t rm_index;
        read_dentry_by_name(buf, &rm_dentry);
        rm_index = return_dentry_index(buf, &rm_dentry);
        if(rm_index == dir_num-1)
        {
            //update db bitmap
            for(i=0; i <= inode_ptr[rm_dentry.inode_num].length_in_B/BLOCK_SIZE; i++)
            {
                bitmap[inode_ptr[rm_dentry.inode_num].data_blocks[i]] = 0;
                bitmap_counter--;
            }
            //update inode bitmap
            inodemap[rm_dentry.inode_num] = 0;
            inodemap_counter--;
            dir_num--;
            // boot_block_ptr->num_dir_entries=dir_num;
            boot_block_ptr->num_dir_entries=dir_num;
            return 0;
        }
        //if it is not the last dir entry, move all the following dir entries one step forward
        //update db bitmap for rm one
        for(i=0; i <= inode_ptr[rm_dentry.inode_num].length_in_B/BLOCK_SIZE; i++)
        {
            bitmap[inode_ptr[rm_dentry.inode_num].data_blocks[i]] = 0;
            bitmap_counter--;
        }
        //update inode bitmap
        inodemap[rm_dentry.inode_num] = 0;
        inodemap_counter--;
        //move all the following dir entries one step forward
        dentry_t next_dentry;
        for(i=rm_index; i<dir_num; i++)
        {
            read_dentry_by_index(i+1, &next_dentry);
            dentry_ptr[i].inode_num = next_dentry.inode_num;
            dentry_ptr[i].file_type = next_dentry.file_type;
            for(j=0; j<32; j++)
            {
                dentry_ptr[i].file_name[j] = next_dentry.file_name[j];
            }
            //update next denrty
            read_dentry_by_index(i+1, &next_dentry);
        }
        dir_num--;
        boot_block_ptr->num_dir_entries=dir_num;       
        return 0;

        //if it is not the last dir entry, move the last dir entry to the rm dir entry
        // dentry_t last_dentry;
        // read_dentry_by_index(dir_num-1, &last_dentry);
        // //update db bitmap for rm one
        // for(i=0; i <= inode_ptr[rm_dentry.inode_num].length_in_B/BLOCK_SIZE; i++)
        // {
        //     bitmap[inode_ptr[rm_dentry.inode_num].data_blocks[i]] = 0;
        //     bitmap_counter--;
        // }
        // //update inode bitmap
        // inodemap[rm_dentry.inode_num] = 0;
        // inodemap_counter--;
        // dir_num--;
        // boot_block_ptr->num_dir_entries=dir_num;        
        // //set the rm one to the last one
        // dentry_ptr[rm_index].inode_num = last_dentry.inode_num;
        // dentry_ptr[rm_index].file_type = last_dentry.file_type;
        // for(j=0; j<cnt; j++)
        // {
        //     dentry_ptr[rm_index].file_name[j] = last_dentry.file_name[j];
        // }
        // return 0;
    }

    //find an available inode
    for(i=0; i<in_num; i++)
    {
        if(inodemap[i] == 0)
        {
            inodemap[i] = 1;
            inodemap_counter++;
            // pcb->fds[fd].inode = i;
            break;
        }
    }
    //initialize the inode
    inode_ptr[i].length_in_B = 0;
    //fill in the dentry with the file name and inode number and file type(2)
    dentry_ptr[dir_num].inode_num = i;
    dentry_ptr[dir_num].file_type = 2;
    //clear filename buffer
    for(j=0; j<32; j++)
    {
        dentry_ptr[dir_num].file_name[j] = '\0';
    }
    for(j=0; j<cnt; j++)
    {
        dentry_ptr[dir_num].file_name[j] = ((uint8_t*)buf)[j];
    }
    dir_num++;
    boot_block_ptr->num_dir_entries=dir_num;
    return 0;

}


int32_t tab_func(int32_t flag)
{

    if(flag == 0) last_modify_length =0;
    int32_t terminal_idx = get_curr_terminal();
    int32_t cmd_counter, counter, i, j;
    cmd_counter =0;
    char * buf = terminal[terminal_idx].keyboard_buf;    

    // if(terminal[terminal_idx].buf_cnt==0) return 0;


    if (terminal[terminal_idx].buf_cnt != terminal[terminal_idx].curr_cur) return 0;
    //delete
    int32_t delete_cnt=terminal[terminal_idx].buf_cnt;
    for(i=0; i < last_modify_length; i ++)
    {
        screen_backspace();
        terminal[terminal_idx].keyboard_buf[delete_cnt -1 -i] = 0;
        terminal[terminal_idx].buf_cnt--;
        terminal[terminal_idx].curr_cur--;
    }


    //
    counter = terminal[terminal_idx].buf_cnt;
    char cmd_buf[128];
    memset(cmd_buf,0,128);
    i = counter - 1;
    while (i >= 0 && buf[i] != ' ') {
        cmd_buf[cmd_counter] = buf[i];
        cmd_counter++;
        i--;
    } // count backwards

    // Reverse the characters in cmd_buf to get the correct order
    for (j = 0; j < cmd_counter / 2; j++) {
        char temp = cmd_buf[j];
        cmd_buf[j] = cmd_buf[cmd_counter - j - 1];
        cmd_buf[cmd_counter - j - 1] = temp;
    }
    

    //read all directory
	uint32_t possible_file_num;
	file_name_t file_name_pool[MAX_DIR_ENTRIES];	
    file_name_t possible_file_name_pool[MAX_DIR_ENTRIES];
        // 
    // if (terminal[terminal_idx].buf_cnt ==0) return 0;
        // tab in the middle of the command, return 0 
    if (terminal[terminal_idx].buf_cnt != terminal[terminal_idx].curr_cur) return 0;

    possible_file_num = 0;


    for (i=0; i<dir_num; i++)
	{
		if(-1 == read_directory(file_name_pool[i].name, i))
			continue;
        // putc('\n');
		// puts(file_name_pool[i].name);
	}

    //search 
    for (i=0; i<dir_num; i++)
    {
        if (cmd_counter >0)
        {
            if (strncmp((int8_t*)cmd_buf, (int8_t*)file_name_pool[i].name, cmd_counter) == 0)
            {
                strncpy((int8_t*)possible_file_name_pool[possible_file_num].name, (int8_t*)file_name_pool[i].name, FILENAME_MAX_SIZE);
                possible_file_num++;
            }
        }
        if (cmd_counter == 0) 
        {
            strncpy((int8_t*)possible_file_name_pool[possible_file_num].name, (int8_t*)file_name_pool[i].name, FILENAME_MAX_SIZE);
            possible_file_num++;
        }
    }
    if ( possible_file_num == 0) return 0;
    if (flag == 0)
    {
        // only one file found
        // if (possible_file_num == 1)
        // {
            //calculate the length in the buffer
        search_list_index = 0;
        uint32_t total_file_length = strlen( (int8_t*)possible_file_name_pool[0].name);
        if (total_file_length>32) total_file_length =32;
        uint32_t legnth_to_fill = total_file_length - cmd_counter;

        last_modify_length = legnth_to_fill;

        for (i=0; i<legnth_to_fill; i++)
        {
            buf[terminal[terminal_idx].buf_cnt] = possible_file_name_pool[0].name[i+cmd_counter];
            terminal[terminal_idx].buf_cnt++;
            terminal[terminal_idx].curr_cur++;
            putc(possible_file_name_pool[0].name[i+cmd_counter]);
        }
        // }
        search_list_index = (search_list_index+1) % possible_file_num;
        return 0;
        
    }
    if (flag == 1)
    {
        uint32_t total_file_length = strlen( (int8_t*)possible_file_name_pool[search_list_index].name);
        if (total_file_length>32) total_file_length =32;
        uint32_t legnth_to_fill = total_file_length - cmd_counter;

        last_modify_length = legnth_to_fill;

        for (i=0; i<legnth_to_fill; i++)
        {
            buf[terminal[terminal_idx].buf_cnt] = possible_file_name_pool[search_list_index].name[i+cmd_counter];
            terminal[terminal_idx].buf_cnt++;
            terminal[terminal_idx].curr_cur++;
            putc(possible_file_name_pool[search_list_index].name[i+cmd_counter]);
        }
        // }
        search_list_index = (search_list_index+1) % possible_file_num;
        return 0;
    }
    return 0;
    
}
