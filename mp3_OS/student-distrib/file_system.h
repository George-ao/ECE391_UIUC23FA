#ifndef _FILE_SYSTEM_H
#define _FILE_SYSTEM_H

#include "system_call.h"
#include "types.h"
#include "lib.h"

//write
# define DB_NUM 59
# define INODE_NUM 64

# define MAX_DIR_ENTRIES 63
# define DENTRY_OFFSET 64
# define DENTRY_LIST_SIZE 63
# define MAX_DATA_BLOCK_SIZE 1023
# define BLOCK_SIZE 4096
# define BOOT_BLOCK_SIZE 4096
# define INODE_SIZE 4096
# define FILENAME_MAX_SIZE 32
# define FILETYPE_SIZE 4
# define LENGTH_SIZE 4

// bitmap for file system

typedef struct dentry_t
{
    uint8_t  file_name[32];          // 32 B for file name
    uint32_t file_type;
    uint32_t inode_num;
    uint8_t  reserved[24];           // 24 bytes reserved
} dentry_t;

typedef struct boot_block_t
{
    uint32_t num_dir_entries;
    uint32_t num_inodes;
    uint32_t num_data_blocks;
    uint8_t  reserved[52];           // 52 bytes reserved
    dentry_t dir_entries[DENTRY_LIST_SIZE];
} boot_block_t;

typedef struct inode_t
{
    uint32_t length_in_B;
    uint32_t data_blocks[MAX_DATA_BLOCK_SIZE];
} inode_t;

typedef struct data_block_t
{
    uint8_t data[BLOCK_SIZE];
} data_block_t;

typedef struct file_name_t
{
    uint8_t name[32];
} file_name_t;


void    file_system_init (uint32_t start_addr);
int32_t read_dentry_by_name (const uint8_t* fname, dentry_t* dentry);
int32_t read_dentry_by_index (uint32_t index, dentry_t* dentry);
int32_t read_data (uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t length);
int32_t read_directory(uint8_t* buf, uint32_t index);
int32_t write_data (uint32_t inode, uint8_t* buf, uint32_t length);
// System call functions
int32_t file_open (const uint8_t* filename);
int32_t file_close (int32_t fd);
int32_t file_read (int32_t fd, void* buf, int32_t nbytes);
int32_t file_write (int32_t fd, const void* buf, int32_t nbytes);

int32_t dir_open (const uint8_t* filename);
int32_t dir_close (int32_t fd);
int32_t dir_read (int32_t fd, void* buf, int32_t nbytes);
int32_t dir_write (int32_t fd, const void* buf, int32_t nbytes);

int32_t tab_func(int32_t flag);

#endif /* FILE_SYSTEM_H */
