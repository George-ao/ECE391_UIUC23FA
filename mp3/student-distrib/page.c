#include "page.h"
#include "system_call.h"

/* 
 * page_init: initialize the page table
 * Input: none
 * Output: none
 * Return value: none
 * Side effect: set up the PD, PT
*/
void page_init() 
{
    //initialize all the entries in PD and PT to 0 ; blank page directory and page table
    int i;
    for(i=0; i < PD_ENTRY_NUM; i++)
    {
        page_directory[i].val = 0x2;
    }
    for(i=0; i < PT_ENTRY_NUM; i++)
    {
        page_table_0[i].val = 0;
    }
    // populate value to page table
    uint32_t video_memory_ = VIDEO_MEMORY;                      // video memory addr
    uint32_t page_table_addr = (uint32_t)page_table_0;            // pt addr
    uint32_t page_directory_addr = (uint32_t)page_directory;    // pd addr

    page_table_0[video_memory_  >> 12].val = (video_memory_ & 0xFFFFF000) | 0x3;    //pt entry
    SET_PTE(page_table_0, (uint32_t)VIDEO_BACKUP_0, (uint32_t)VIDEO_BACKUP_0, 0, 1);
    SET_PTE(page_table_0, (uint32_t)VIDEO_BACKUP_1, (uint32_t)VIDEO_BACKUP_1, 0, 1);
    SET_PTE(page_table_0, (uint32_t)VIDEO_BACKUP_2, (uint32_t)VIDEO_BACKUP_2, 0, 1);
    page_directory[0].val = (page_table_addr & 0xFFFFF000) | 0x3;                   //pd entry(pt)   RW = 1, present=1
    page_directory[1].val = (page_directory_addr & 0xFFFFF000) | 0x183;             //pd entry(kernel page)   phys=4MB, PS=1, prev=0, rw=1, present=1,G=1
    // cr3 : page directory addr
    // cr4 : allow 4 mb page (enable PSE)
    // cr0 : set paging (PG)
    asm volatile(
        "movl  %0, %%eax;           \
         movl  %%eax, %%cr3;        \
         movl  %%cr4, %%eax;        \
         orl   $0x00000010, %%eax;  \
         movl  %%eax, %%cr4;        \
         movl  %%cr0, %%eax;        \
         orl   $0x80000000, %%eax;  \
         movl  %%eax, %%cr0;"
        : /*no output*/
        : "r" (&page_directory)
        : "%eax"
    );
}

uint32_t cr3;

/* 
 * page_init_by_idx: initialize the page by index
 * Input: pid
 * Output: none
 * Return value: none
 * Side effect: change the cr3 
*/
void page_init_by_idx(uint32_t pid) {
    uint32_t* phys_addr = (uint32_t*)(pid * USER_MEM_SIZE + USER_PHYS_START);
    uint32_t* virt_addr = (uint32_t*) USER_VIRT;
    SET_PDE(page_directory, (uint32_t)phys_addr, (uint32_t)virt_addr);
    change_cr3();
}

/* 
 * page_video_map: set PTE, PDE of page_table_video
 * Input: start, pid
 * Output: none
 * Return value: none
 * Side effect: change the cr3 
*/
void page_video_map(uint8_t ** start, uint32_t pid) {
    *start = (uint8_t*)(USER_VIRT_VIDEO); 
    SET_PTE(user_page_table_video, VIDEO_MEMORY, (uint32_t)(*start), 1, 1);
    SET_PDE_PT(page_directory, (uint32_t)user_page_table_video, (uint32_t)(*start), 1, 1);
}

/* 
 * page_video_unmount: unmap the paging for the current running program
 * Input: pid
 * Output: none
 * Return value: none
 * Side effect: unmount the page video
*/
void page_video_unmount(uint32_t pid) {
    SET_PTE(user_page_table_video, VIDEO_MEMORY, (uint32_t)USER_VIRT_VIDEO, 1, 0);
    // SET_PDE_PT(page_directory, (uint32_t)user_page_table_video, (uint32_t)USER_VIRT_VIDEO, 1, 0);

}

/* 
 * switch_video_map_paging: for SCHEUDLER
 * Input: pid
 * Output: none
 * Return value: none
 * Side effect: change the cr3 
*/
void switch_video_map_paging(int32_t pid){
    int32_t program_term, curr_term;
    process_control_block_t* pcb = get_pcb_by_pid(pid);
    int32_t user_video_indicator;
    curr_term = get_curr_terminal();

    if (pid == 0 || pid == 1 || pid == 2) {// if shell, map to video memory
        program_term = pid;
        user_video_indicator = 0;
    }
    else {                               // if user program, map to corresponding terminal (backup video memory
        program_term = pcb->terminal_num;
        user_video_indicator = pcb->user_video_indicator;
    }
    if(curr_term == program_term){  // if current terminal, map to video memory
        SET_PTE(page_table_0, (uint32_t)VIDEO_MEMORY, (uint32_t)VIDEO_MEMORY, 0, 1);
        SET_PTE(user_page_table_video, (uint32_t)VIDEO_MEMORY, USER_VIRT_VIDEO, 1, user_video_indicator);
        // page_table_0[VIDEO_MEMORY >> 12].val = (VIDEO_MEMORY & 0xFFFFF000) | 0x1;
    }
    else{
        switch (program_term)       // if not current terminal, map to backup video memory
        {
        case 0:
            SET_PTE(page_table_0, (uint32_t)VIDEO_BACKUP_0, (uint32_t)VIDEO_MEMORY, 0, 1);
            SET_PTE(user_page_table_video, (uint32_t)VIDEO_BACKUP_0, USER_VIRT_VIDEO, 1, user_video_indicator);
            break;
        case 1:
            SET_PTE(page_table_0, (uint32_t)VIDEO_BACKUP_1, (uint32_t)VIDEO_MEMORY, 0, 1);
            SET_PTE(user_page_table_video, (uint32_t)VIDEO_BACKUP_1, USER_VIRT_VIDEO, 1, user_video_indicator);
            break;
        case 2:
            SET_PTE(page_table_0, (uint32_t)VIDEO_BACKUP_2, (uint32_t)VIDEO_MEMORY, 0, 1);
            SET_PTE(user_page_table_video, (uint32_t)VIDEO_BACKUP_2, USER_VIRT_VIDEO, 1, user_video_indicator);
            break;
        default:
            break;
        }
    }
    change_cr3();       // flush TLB
}

/* 
 * update_video_mapping: for TERMINAL SWITCH
 * Input: terminal_id
 * Output: none
 * Return value: none
 * Side effect: DO NOT change the cr3 
 *              not changing program
*/
void update_video_mapping() {
    SET_PTE(page_table_0, (uint32_t)VIDEO_MEMORY, (uint32_t)VIDEO_MEMORY, 0, 1);
    change_cr3();
}

/* 
 * restore_video_mapping: for TERMINAL SWITCH
 * Input: pid
 * Output: none
 * Return value: none
 * Side effect: DO NOT change the cr3 
 *              not changing program
*/
void restore_video_mapping(int32_t pid) {
    switch_video_map_paging(pid);
}



/* 
 * change_cr3: change the cr3 (PDBR)
 * Input: none
 * Output: none
 * Return value: cr3 when saving cr3
 * Side effect: change the cr3 
*/
void change_cr3() {
    // save cr3
    asm volatile(
        "movl  %%cr3, %%eax;         \
         movl  %%eax, %0;"
        : "=r" (cr3)
        : /*no input*/
        : "%eax"
    );

    // change cr3
    asm volatile(
        "movl  %0, %%eax;           \
         movl  %%eax, %%cr3;"
        : /*no output*/
        : "r" (cr3)
        : "%eax"
    );
}
