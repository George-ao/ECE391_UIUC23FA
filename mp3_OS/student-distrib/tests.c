#include "tests.h"
#include "x86_desc.h"
#include "lib.h"

#include "debug.h"
#include "file_system.h"
#include "i8259.h"
#include "idt.h"
#include "idt_handler.h"
#include "idt_linkage.h"
#include "idt.h"
#include "keyboard.h"
#include "multiboot.h"
#include "page.h"
#include "rtc.h"
#include "terminal.h"

#define PASS 1
#define FAIL 0

#define VIDEO_START 0xB8000
#define VIDEO_SIZE 0x1000

#define KERNEL_START 0x400000
#define KERNEL_SIZE 0x400000
#define MAX_DIRECTORY_NUM 63

/* format these macros as you see fit */
#define TEST_HEADER 	\
	printf("[TEST %s] Running %s at %s:%d\n", __FUNCTION__, __FUNCTION__, __FILE__, __LINE__)
#define TEST_OUTPUT(name, result)	\
	printf("[TEST %s] Result = %s\n", name, (result) ? "PASS" : "FAIL");

static inline void assertion_failure(){
	/* Use exception #15 for assertions, otherwise
	   reserved by Intel */
	asm volatile("int $15");
}


/* @@ Checkpoint 1 tests */

/* IDT Test - Example
 * 
 * Asserts that first 10 IDT entries are not NULL
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage: Load IDT, IDT definition
 * Files: x86_desc.h/S
 */
int idt_test(){
	TEST_HEADER;

	int i;
	int result = PASS;
	for (i = 0; i < 10; ++i){
		if ((idt[i].offset_15_00 == NULL) && 
			(idt[i].offset_31_16 == NULL)){
			assertion_failure();
			result = FAIL;
		}
	}

	return result;
}

/* -------------------- TEST EXCEPTIONS -------------------- */

/* #0x0 Divide by 0 Test
 * 
 * If successful, should pop a divide by 0 exception
 * Inputs: None
 * Outputs: Exception or FAIL
 * Side Effects: None
 * Coverage: IDT, IDT handler
 * Files: idt_handler.c/h
 */
int divide_by_0_test() {
	TEST_HEADER;
	int a = 1;
	int b = 0;
	int c = a / b;
	c = c;		// to avoid unused variable warning
	return FAIL;
}


/* #0x4 Overflow Test
 * 
 * If successful, should pop an overflow exception
 * Inputs: None
 * Outputs: Exception or FAIL
 * Side Effects: None
 * Coverage: IDT, IDT handler
 * Files: idt_handler.c/h
 */
// int overflow_test() {
// 	TEST_HEADER;
// 	int a = 0x7FFFFFFF;
// 	int b = 0x7FFFFFFF;
// 	b += a;		// to avoid unused variable warning
// 	return FAIL;
// }

/* #0x5 Bound Range Exceeded Test
 * 
 * If successful, should pop a bound range exceeded exception
 * Inputs: None
 * Outputs: Exception or FAIL
 * Side Effects: None
 * Coverage: IDT, IDT handler
 * Files: idt_handler.c/h
 */
// int bound_range_exceeded_test() {
// 	TEST_HEADER;
// 	int a[2] = {0, 1};
// 	int b = a[2];
// 	b = b;		// to avoid unused variable warning
// 	return FAIL;
// }

/* #0x6 Invalid Opcode Test
 * 
 * If successful, should pop an invalid opcode exception
 * Inputs: None
 * Outputs: Exception or FAIL
 * Side Effects: None
 * Coverage: IDT, IDT handler
 * Files: idt_handler.c/h
 */
int invalid_opcode_test() {
	TEST_HEADER;
	asm volatile("ud2");
	return FAIL;
}

/* Test Any Exceptions -- use ASM
 * 
 * If successful, should pop an exception as indicated
 * Inputs: None
 * Outputs: Exception or FAIL
 * Side Effects: None
 * Coverage: IDT, IDT handler
 * Files: idt_handler.c/h
 */
int Any_Exception_Test() {
	TEST_HEADER;
	asm volatile("int $0x01");
	return FAIL;
}


/* Test System call -- use ASM
 * 
 * If successful, should pop a system call as indicated
 * Inputs: None
 * Outputs: system call or FAIL
 * Side Effects: None
 * Coverage: IDT, IDT handler
 * Files: idt_handler.c/h
 */
int System_Call_Test() {
	TEST_HEADER;
	asm volatile("int $0x80");
	return FAIL;
}


/* -------------------- TEST PAGING -------------------- */

/* Page Fault Test Series
 * 
 * If successful, should pop a page fault exception
 * Inputs: None
 * Outputs: Exception or FAIL
 * Side Effects: None
 * Coverage: IDT, IDT handler
 * Files: idt_handler.c/h, page.c/h
 */
int page_fault_test_1() {
	TEST_HEADER;
	int *a = (int *)0x100000;
	int b = *a;
	b = b;
	return FAIL;
}
// lower boundary condition of kernel memory
int page_fault_kernel_boundary_1() {
	TEST_HEADER;
	int *a = (int *)0x3FFFFF;
	int b = *a;
	b = b;		// to avoid unused variable warning
	return FAIL;
}
// upper boundary condition of kernel memory
int page_fault_kernel_boundary_2() {
	TEST_HEADER;
	int *a = (int *)0x800000;
	int b = *a;
	b = b;		// to avoid unused variable warning
	return FAIL;
}
// lower boundary condition of video memory
int page_fault_video_boundary_1() {
	TEST_HEADER;
	int *a = (int *)0xB7FFF;
	int b = *a;
	b = b;		// to avoid unused variable warning
	return FAIL;
}
// upper boundary condition of video memory
int page_fault_video_boundary_2() {
	TEST_HEADER;
	int *a = (int *)0xB9000;
	int b = *a;
	b = b;		// to avoid unused variable warning
	return FAIL;
}

/* Page Dereference Null Test
 * 
 * If successful, should pop a page fault exception
 * Inputs: None
 * Outputs: Exception or FAIL
 * Side Effects: None
 * Coverage: IDT, IDT handler
 * Files: idt_handler.c/h, page.c/h
 */
int page_deref_null_test() {
	TEST_HEADER;

	int* a = NULL;
	int b = *a;			// should pop a page fault exception
	b = b;				// to avoid unused variable warning
	return FAIL;
}

/* Page Success Test
 * 
 * If successful, 
 * Inputs: None
 * Outputs: PASS
 * Side Effects: None
 * Coverage: IDT, IDT handler
 * Files: idt_handler.c/h, page.c/h
 */
int page_success_test() {
	TEST_HEADER;

	int* a = (int *) VIDEO_START;			// point to video memory
	int* b = (int *) KERNEL_START;			// point to kernel memory
	int temp;

	for (; a < (int *) (VIDEO_START + VIDEO_SIZE); a++) {
		temp = *a;							// try copy video memory out
	}
	for (; b < (int *) (KERNEL_START + KERNEL_SIZE); b++) {
		temp = *b;							// try copy kernel memory out
	}
	return PASS;							// if no exception, PASS
}




/* @@ Checkpoint 2 tests */
/* -------------------- TEST TERMINAL -------------------- */

char terminal_buf[128] = {0};
/* terminal_tests
 * 
 * Write and read from terminal
 * Inputs: None
 * Outputs: PASS
 * Side Effects: None
 * Coverage: Terminal
 * Files: terminal.c/h
 */
int terminal_tests(){
	terminal_open(0);
	while(1){
		// printf("Input: ");
		printf("Input Count: %d\n", terminal_read(0,terminal_buf,128));
		// printf("Output: ");
		printf("Output Count: %d\n", terminal_write(0,terminal_buf,128));
	}
	return FAIL;
}

/* -------------------- TEST RTC -------------------- */
/* rtc_tests
 * Write and read from rtc, Set different frequency
 * Inputs: None
 * Outputs: PASS
 * Side Effects: None
 * Coverage: RTC
 * Files: rtc.c/h
 */
int rtc_tests(){
	int freq = 1024;						// default frequency
	int time_cnt = 0;
	rtc_open(0);
	clear();
	while(1){
		rtc_read(0,0,0);
		printf("1");
		rtc_write(0,&freq,0);
		time_cnt++;
		if(time_cnt%freq == 0){
			clear();
			if(freq >= 4) freq /= 2;		// change frequency
			else freq = 1024;				// reset frequency
			time_cnt = 0;
		}
	}
	return FAIL;
}



/* -------------------- TEST FILE -------------------- */
// uint8_t* file_name = (uint8_t*)"dddd";
uint8_t* file_name = (uint8_t*)"hello";
uint8_t* dir_name = (uint8_t*) ".";
/* file_open_test
 * 
 * Give a file name, print the file name, type, and inode number
 * Inputs: None
 * Outputs: PASS
 * Side Effects: None
 * Coverage: File System
 * Files: file_system.c/h
 */
int dentry_name_test(){

	TEST_HEADER;
	clear();
	int j;
	dentry_t dentry;
	int32_t result = read_dentry_by_name(file_name, &dentry);
	if(result == -1){				// file not found
		return FAIL;		
	}
	printf("file_name: ");
	for(j=0; j<32; j++)						// file name, with paddings
	{
		printf("%c",dentry.file_name[j]);
	}
	printf("\n");
	printf("file type: %d\n", dentry.file_type);
	printf("inode num: %d\n", dentry.inode_num);
	return PASS;
}

/* dentry_index_test
 * 
 * Give an index, print the file name, type, and inode number
 * Inputs: None
 * Outputs: PASS
 * Side Effects: None
 * Coverage: File System
 * Files: file_system.c/h
 */
int dentry_index_test(){
	TEST_HEADER;
	dentry_t dentry;
	int j;
	int32_t result = read_dentry_by_index(1, &dentry);
	if(result == -1){				// file not found
		return FAIL;
	}
	printf("file_name: ");
	for(j=0; j<32; j++)						// file name, with paddings
	{
		printf("%c",dentry.file_name[j]);
	}
	printf("\n");
	printf("file type: %d\n", dentry.file_type);
	printf("inode num: %d\n", dentry.inode_num);
	return PASS;
}

/*read_data_test
 * 
 * If successful, 
 * Inputs: None
 * Outputs: PASS
 * Side Effects: None
 * Coverage: File System
 * Files: file_system.c/h
*/
int read_data_test(){
	dentry_t dentry;
	uint32_t inode;
	int32_t  i, j;
	int32_t  result;
	uint8_t  buf[1000000];							// buffer big enough
	TEST_HEADER;
	// clear();
	if(-1 == read_dentry_by_name(file_name, &dentry))
		return FAIL;
	// printf("file name: %s\n", dentry.file_name);
	printf("file_name: ");
	for(j=0; j<32; j++)						// file name, with paddings
	{
		printf("%c",dentry.file_name[j]);
	}
	printf("\n");
	inode = dentry.inode_num;
	result = read_data(inode, 0, buf, 1000000);		// read data
	if(result == -1){
		return FAIL;
	}
	// printf("buf: %s\n", buf);
	for (i = 0; i < result; ++i)
	{
		if (buf[i] == 0)							// ignore NULLs
			continue;
		putc(buf[i]);
	}
	printf("\n");
	return PASS;
}

/*read_directory_test
 * 
 * Print all the files: name & type & size in the directory, 
 * Inputs: None
 * Outputs: PASS
 * Side Effects: None
 * Coverage: File System
 * Files: file_system.c/h
*/
int read_directory_test()
{
	// dentry_t dentry;
	// uint32_t inode, length;
	uint32_t  i, j, total;
	uint8_t  buf[42];							// size = 32 + 4 + 4 + 2paddings
	TEST_HEADER;
	clear();
	for(i=0; i< MAX_DIRECTORY_NUM; i++)
	{
		// printf("file_name: ",buf[j]);
		if(-1 == read_directory(buf, i))
			continue;
		
		printf("file_name: ");
		for(j=0; j<32; j++)						// file name, with paddings
		{
			printf("%c",buf[j]);
		}

		printf("  file_type: ");				// file type, with format
		for(j=35; j<36; j++)
		{
			printf("%d",buf[j]);
		}

		printf("  file_size: ");				// file size, with format
		total=0;
		for(j=36; j<40; j++)
		{
			// printf("%d",buf[j]);
			total += buf[j] << (24-8*(j-36));	// convert to decimal
		}
		printf("%d", total);
		printf("\n");
	}
	return PASS;
}

/*file_open_test
 * 
 * If open successfully, return 0
 * Inputs: None
 * Outputs: PASS
 * Side Effects: None
 * Coverage: File System
 * Files: file_system.c/h
*/
int file_open_test(){
	TEST_HEADER;
	int result = file_open(file_name);
	if (result == -1)						// file not found
	{
		return FAIL;
	}
	return PASS;
}

/*file_read_test
 * 
 * If read successfully, return 0
 * Inputs: None
 * Outputs: PASS
 * Side Effects: None
 * Coverage: File System
 * Files: file_system.c/h
*/
int file_read_test(){
	TEST_HEADER;
	uint8_t  buf[10];	//arbitrary number for buf size
	int result = file_read(0,buf,0);
	if (result == 0)						// file not found
	{
		return PASS;
	}
	return FAIL;
}
/*dir_read_test
 * 
 * If read successfully, return 0
 * Inputs: None
 * Outputs: PASS
 * Side Effects: None
 * Coverage: File System
 * Files: file_system.c/h
*/
int dir_read_test(){
	TEST_HEADER;
	uint8_t  buf[10];	//arbitrary number for buf size
	int result = file_read(0,buf,0);
	if (result == 0)						// file not found
	{
		return PASS;
	}
	return FAIL;
}
/*file_close_test
 * 
 * If close successfully, return 0
 * Inputs: None
 * Outputs: PASS
 * Side Effects: None
 * Coverage: File System
 * Files: file_system.c/h
*/
int file_close_test(){
	TEST_HEADER;
	int result = file_close(0);
	if (result == 0)						// close successfully
	{
		return PASS;
	}
	return FAIL;
}

/*file_write_test
 * 
 * write should return -1
 * Inputs: None
 * Outputs: PASS
 * Side Effects: None
 * Coverage: File System
 * Files: file_system.c/h
*/
int file_write_test(){
	TEST_HEADER;
	int result = file_write(0,0,0);
	if (result == -1)						// read only
	{
		return PASS;
	}
	printf("Read Only System!!!\n");
	return FAIL;
}


/*dir_open_test
 * 
 * If open successfully, return 0
 * Inputs: None
 * Outputs: PASS
 * Side Effects: None
 * Coverage: File System
 * Files: file_system.c/h
*/
int dir_open_test(){
	TEST_HEADER;
	int result = dir_open(dir_name);
	if (result == -1)						// file not found
	{
		return FAIL;
	}
	return PASS;
}

/*dir_close_test
 * 
 * If close successfully, return 0
 * Inputs: None
 * Outputs: PASS
 * Side Effects: None
 * Coverage: File System
 * Files: file_system.c/h
*/
int dir_close_test(){
	TEST_HEADER;
	int result = dir_close(0);
	if (result == 0)						// close successfully
	{
		return PASS;
	}
	return FAIL;
}

/*dir_write_test
 * 
 * write should return -1
 * Inputs: None
 * Outputs: PASS
 * Side Effects: None
 * Coverage: File System
 * Files: file_system.c/h
*/
int dir_write_test(){
	TEST_HEADER;
	int result = dir_write(0,0,0);
	if (result == -1)						// read only
	{
		return PASS;
	}
	printf("Read Only System!!!\n");
	return FAIL;
}

/* @@ Checkpoint 3 tests */
/* @@ Checkpoint 4 tests */
/* @@ Checkpoint 5 tests */


/* Test suite entry point */
void launch_tests(){
	// TEST_OUTPUT("Test System Call", System_Call_Test());


	/* -------- @@ Checkpoint 1 Tests -------- */
	// TEST_OUTPUT("idt_test", idt_test());
	/* ---------- Exception Tests ---------- */
	// TEST_OUTPUT("Test Any Exceptions", Any_Exception_Test());
	// TEST_OUTPUT("divide_by_0_test", divide_by_0_test());
	// TEST_OUTPUT("invalid_opcode_test", invalid_opcode_test());

	/* ---------- Page Fault Test ---------- */
	// TEST_OUTPUT("page_fault_test_1", page_fault_test_1());
	// TEST_OUTPUT("page_fault_kernel_lower_boundary", page_fault_kernel_boundary_1());
	// TEST_OUTPUT("page_fault_kernel_upper_boundary", page_fault_kernel_boundary_2());
	// TEST_OUTPUT("page_fault_video_lower_boundary", page_fault_video_boundary_1());
	// TEST_OUTPUT("page_fault_video_upper_boundary", page_fault_video_boundary_2());
	// TEST_OUTPUT("page_deref_null_test", page_deref_null_test());
	// TEST_OUTPUT("page_SUCCESS_test", page_success_test());


	/* -------- @@ Checkpoint 2 Tests -------- */
	// TEST_OUTPUT("terminal_tests", terminal_tests());
	// TEST_OUTPUT("rtc_tests", rtc_tests());

	/* ---------- File System Test ---------- */
	// TEST_OUTPUT("dentry_name_test", dentry_name_test());
	// TEST_OUTPUT("dentry_index_test", dentry_index_test());
	// TEST_OUTPUT("read_data_test", read_data_test());
	// TEST_OUTPUT("read_directory_test",read_directory_test());
	// TEST_OUTPUT("file_open_test", file_open_test());
	// TEST_OUTPUT("file_close_test", file_close_test());
	// TEST_OUTPUT("file_write_test", file_write_test());
	// TEST_OUTPUT("dir_open_test", dir_open_test());
	// TEST_OUTPUT("dir_close_test", dir_close_test());
	// TEST_OUTPUT("dir_write_test", dir_write_test());
	// TEST_OUTPUT("dir_read_test", 	dir_read_test());
	// TEST_OUTPUT("file_read_test", 	file_read_test());
	// launch your tests here
}
