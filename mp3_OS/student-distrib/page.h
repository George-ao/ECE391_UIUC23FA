/* page.h - Defines for various x86 descriptors, descriptor tables,
 * and selectors
 * vim:ts=4 noexpandtab
 */

#ifndef _PAGE_H
#define _PAGE_H

#include "types.h"

// #ifndef ASM


#define PD_ENTRY_NUM 1024               //page directory entry
#define PT_ENTRY_NUM 1024               //page table entry             
#define BYTES_TO_ALIGN_TO_PD (4*1024)   // 4kb
#define BYTES_TO_ALIGN_TO_PT (4*1024)   // 4kb
#define BYTES_TO_ALIGN_TO_VM (4*1024)   // 4kb

#define VIDEO_MEMORY    0x000B8000      // start addr of each video memory 
#define VIDEO_BACKUP_0  0x000BA000      // both physical and virtual
#define VIDEO_BACKUP_1  0x000BB000
#define VIDEO_BACKUP_2  0x000BC000

#define KERNEL_MEMORY   0x00400000      // 4MB

#define USER_PHYS_START 0x00800000
#define USER_VIRT       0x08000000
#define USER_VIRT_VIDEO 0x08400000
#define USER_MEM_SIZE   0x00400000


/* This is a page director entry. */
typedef union page_directory_entry_t {
        uint32_t val;
        struct 
        {
            uint32_t present       : 1;     // present bit
            uint32_t read_write    : 1;     // read/write 
            uint32_t user_super    : 1; 
            uint32_t PWT           : 1;
            uint32_t PCD           : 1;
            uint32_t A             : 1;     // accessed
            uint32_t D             : 1;     // dirty
            uint32_t PS            : 1;
            uint32_t global        : 1;     // global
            uint32_t AVL           : 3;
            uint32_t pt_addr       :20;     // page table addr
        } __attribute__ ((packed));
} page_directory_entry_t;

/* This is a page table entry. */
typedef union page_table_entry_t {
        uint32_t val;
        struct {
            uint32_t present       : 1;  // present bit
            uint32_t read_write    : 1;  // read/write
            uint32_t user_super    : 1;  // user supervisor
            uint32_t PWT           : 1;  // write through 
            uint32_t PCD           : 1;
            uint32_t A             : 1;  // accessed
            uint32_t D             : 1;  // dirty
            uint32_t PAT           : 1;  // page attribute table
            uint32_t G             : 1;  // global
            uint32_t AVL           : 3;  // available
            uint32_t page_addr     : 20; // page table addr
        } __attribute__ ((packed));
} page_table_entry_t;



//create a blank page directory and page table
page_directory_entry_t page_directory[PD_ENTRY_NUM] __attribute__((aligned (BYTES_TO_ALIGN_TO_PD)));    // align them to 4kb
page_table_entry_t page_table_0[PT_ENTRY_NUM]         __attribute__((aligned (BYTES_TO_ALIGN_TO_PT)));    // align them to 4kb
page_table_entry_t user_page_table_video[PT_ENTRY_NUM] __attribute__((aligned (BYTES_TO_ALIGN_TO_PT)));
page_table_entry_t page_table_video_2[PT_ENTRY_NUM] __attribute__((aligned (BYTES_TO_ALIGN_TO_PT)));
page_table_entry_t page_table_video_3[PT_ENTRY_NUM] __attribute__((aligned (BYTES_TO_ALIGN_TO_PT)));



//page init function
void page_init();
void page_init_by_idx(uint32_t pid);
// void other_page_init(uint32_t* addr, uint32_t* virt_addr);
// void kernel_page_init(uint32_t* addr, uint32_t* virt_addr);
void page_video_map(uint8_t ** start, uint32_t pid);
void page_video_unmount(uint32_t pid);
void switch_video_map_paging(int32_t pid);
void update_video_mapping();
void restore_video_mapping(int32_t pid);
void change_cr3();



// 0xFFC00000: bit 31-22; 0x87 : 1000 0111 (ps, user, r/w, present)
#define SET_PDE(pd, phys_addr, vir_addr) do { \
    (pd)[(vir_addr) >> 22].val = (((phys_addr) & 0xFFC00000) | 0x87);  \
} while(0)

// 0xFFFFF000: bit 31-12; 0x02 : 0000 0010 (r/w)
#define SET_PDE_PT(pd, phys_addr, vir_addr, priv, present) do { \
    (pd)[(vir_addr) >> 22].val = (((phys_addr) & 0xFFFFF000) | 0x02 | (priv)<<2 | (present));  \
} while(0)

// 0xFFFFF000: bit 31-12; 0x02 : 0000 0010 (r/w)
#define SET_PTE(pt, phys_addr, vir_addr, priv, present) do { \
    (pt)[((vir_addr)&0x003ff000) >> 12].val = (((phys_addr) & 0xFFFFF000) | 0x02 | (priv)<<2 | (present)); \
} while(0)

// #endif /* ASM */
#endif /* _PAGE_H */
