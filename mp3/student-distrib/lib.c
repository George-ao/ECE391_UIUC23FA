/* lib.c - Some basic library functions (printf, strlen, etc.)
 * vim:ts=4 noexpandtab */

#include "lib.h"
#include "page.h"
#include "system_call.h"

#define VIDEO       0xB8000
#define NUM_COLS    80
#define NUM_ROWS    25
#define ATTRIB      0x7
#define MEM_SIZE    4*1024

static uint8_t prompt_color = 0x7;
static int terminal_id = 0;
static int scheduler_id = -1;
static int screen_x;
static int screen_y;
static int keyboard_flag = 0;
static char* video_mem = (char *)VIDEO;
char* t_video_mem[MAX_NUM] = {(char *)0xBA000, (char *)0xBB000, (char *)0xBC000};
static int newline_flag = 0;

/* void clear(void);
 * Inputs: void
 * Return Value: none
 * Function: Clears video memory */
void clear(void) {
    int32_t i;
    for (i = 0; i < NUM_ROWS * NUM_COLS; i++) {
        *(uint8_t *)(video_mem + (i << 1)) = ' ';
        *(uint8_t *)(video_mem + (i << 1) + 1) = prompt_color;
    }
    screen_x = 0; // reset the cursor position
    screen_y = 0;
    update_cursor(screen_x, screen_y);
}

/* Standard printf().
 * Only supports the following format strings:
 * %%  - print a literal '%' character
 * %x  - print a number in hexadecimal
 * %u  - print a number as an unsigned integer
 * %d  - print a number as a signed integer
 * %c  - print a character
 * %s  - print a string
 * %#x - print a number in 32-bit aligned hexadecimal, i.e.
 *       print 8 hexadecimal digits, zero-padded on the left.
 *       For example, the hex number "E" would be printed as
 *       "0000000E".
 *       Note: This is slightly different than the libc specification
 *       for the "#" modifier (this implementation doesn't add a "0x" at
 *       the beginning), but I think it's more flexible this way.
 *       Also note: %x is the only conversion specifier that can use
 *       the "#" modifier to alter output. */
int32_t printf(int8_t *format, ...) {

    /* Pointer to the format string */
    int8_t* buf = format;

    /* Stack pointer for the other parameters */
    int32_t* esp = (void *)&format;
    esp++;

    while (*buf != '\0') {
        switch (*buf) {
            case '%':
                {
                    int32_t alternate = 0;
                    buf++;

format_char_switch:
                    /* Conversion specifiers */
                    switch (*buf) {
                        /* Print a literal '%' character */
                        case '%':
                            putc('%');
                            break;

                        /* Use alternate formatting */
                        case '#':
                            alternate = 1;
                            buf++;
                            /* Yes, I know gotos are bad.  This is the
                             * most elegant and general way to do this,
                             * IMHO. */
                            goto format_char_switch;

                        /* Print a number in hexadecimal form */
                        case 'x':
                            {
                                int8_t conv_buf[64];
                                if (alternate == 0) {
                                    itoa(*((uint32_t *)esp), conv_buf, 16);
                                    puts(conv_buf);
                                } else {
                                    int32_t starting_index;
                                    int32_t i;
                                    itoa(*((uint32_t *)esp), &conv_buf[8], 16);
                                    i = starting_index = strlen(&conv_buf[8]);
                                    while(i < 8) {
                                        conv_buf[i] = '0';
                                        i++;
                                    }
                                    puts(&conv_buf[starting_index]);
                                }
                                esp++;
                            }
                            break;

                        /* Print a number in unsigned int form */
                        case 'u':
                            {
                                int8_t conv_buf[36];
                                itoa(*((uint32_t *)esp), conv_buf, 10);
                                puts(conv_buf);
                                esp++;
                            }
                            break;

                        /* Print a number in signed int form */
                        case 'd':
                            {
                                int8_t conv_buf[36];
                                int32_t value = *((int32_t *)esp);
                                if(value < 0) {
                                    conv_buf[0] = '-';
                                    itoa(-value, &conv_buf[1], 10);
                                } else {
                                    itoa(value, conv_buf, 10);
                                }
                                puts(conv_buf);
                                esp++;
                            }
                            break;

                        /* Print a single character */
                        case 'c':
                            putc((uint8_t) *((int32_t *)esp));
                            esp++;
                            break;

                        /* Print a NULL-terminated string */
                        case 's':
                            puts(*((int8_t **)esp));
                            esp++;
                            break;

                        default:
                            break;
                    }

                }
                break;

            default:
                putc(*buf);
                break;
        }
        buf++;
    }
    return (buf - format);
}

/* int32_t puts(int8_t* s);
 *   Inputs: int_8* s = pointer to a string of characters
 *   Return Value: Number of bytes written
 *    Function: Output a string to the console */
int32_t puts(int8_t* s) {
    register int32_t index = 0;
    while (s[index] != '\0') {
        putc(s[index]);
        index++;
    }
    return index;
}

/* void putc(uint8_t c);
 * Inputs: uint_8* c = character to print
 * Return Value: void
 *  Function: Output a character to the console */
void putc(uint8_t c) {
    int32_t curr_scheduler = scheduler_id;
    if(curr_scheduler == -1) curr_scheduler = 0;
    if(terminal_id == curr_scheduler || keyboard_flag){ // doint a if here to make sure we can write to the screen correctly
        if(c == '\n' || c == '\r') {
            if(newline_flag){
                newline_flag = 0;
                return;
            }
            if(screen_y == NUM_ROWS - 1){ // condition for scrolling up
                scroll_up();
                return;
            }
            // if(screen_x == 0 && screen_y != 0) return;
            screen_y++;
            screen_x = 0;
        } else {
            if(newline_flag) newline_flag = 0;
            // if(scroll_flag) scroll_up();
            // t_video_mem[scheduler_id]
            *(uint8_t *)(video_mem + ((NUM_COLS * screen_y + screen_x) << 1)) = c;
            *(uint8_t *)(video_mem + ((NUM_COLS * screen_y + screen_x) << 1) + 1) = prompt_color;
            screen_x++;
            // checkpoint 2 change the order of the statements
            if(screen_x == NUM_COLS) newline_flag = 1;
            if(screen_x == NUM_COLS && screen_y == NUM_ROWS - 1) scroll_up();
            screen_y = (screen_y + (screen_x / NUM_COLS)) % NUM_ROWS;
            screen_x %= NUM_COLS;
        }
        update_cursor(screen_x, screen_y);
    }
    else{ // while other scheduler is occupying the time slice
        if(c == '\n' || c == '\r') {
            if(terminal[curr_scheduler].t_newline_flag){
                terminal[curr_scheduler].t_newline_flag = 0;
                return;
            }
            if(terminal[curr_scheduler].t_screen_y == NUM_ROWS - 1){ // condition for scrolling up
                scroll_up();
                return;
            }
            // if(screen_x == 0 && screen_y != 0) return;
            terminal[curr_scheduler].t_screen_y++;
            terminal[curr_scheduler].t_screen_x = 0;
        } else {
            if(terminal[curr_scheduler].t_newline_flag) terminal[curr_scheduler].t_newline_flag = 0;
            // if(scroll_flag) scroll_up();
            *(uint8_t *)(video_mem + ((NUM_COLS * terminal[curr_scheduler].t_screen_y + terminal[curr_scheduler].t_screen_x) << 1)) = c;
            *(uint8_t *)(video_mem + ((NUM_COLS * terminal[curr_scheduler].t_screen_y + terminal[curr_scheduler].t_screen_x) << 1) + 1) = terminal[curr_scheduler].t_prompt_color;
            terminal[curr_scheduler].t_screen_x++;
            // checkpoint 2 change the order of the statements
            if(terminal[curr_scheduler].t_screen_x == NUM_COLS) terminal[curr_scheduler].t_newline_flag = 1;
            if(terminal[curr_scheduler].t_screen_x == NUM_COLS && terminal[curr_scheduler].t_screen_y == NUM_ROWS - 1) scroll_up();
            terminal[curr_scheduler].t_screen_y = (terminal[curr_scheduler].t_screen_y + (terminal[curr_scheduler].t_screen_x / NUM_COLS)) % NUM_ROWS;
            terminal[curr_scheduler].t_screen_x %= NUM_COLS;
        }
    }
}

/* void scroll_up(void);
 * Inputs: void
 * Return Value: void
 *  Function: helper function to scroll down the screen */
void scroll_up(void){
    int32_t i;
    for (i = 0; i < (NUM_ROWS - 1) * NUM_COLS; i++) 
    {
        video_mem[i << 1] = video_mem[(i + NUM_COLS) << 1];   // scroll each position
        video_mem[(i << 1) + 1] = video_mem[((i + NUM_COLS) << 1) + 1];   // scroll each position
    }
    for (; i < NUM_ROWS * NUM_COLS; i++){
        video_mem[i << 1] = ' ';  // set the last line to spaces
        if(terminal_id == scheduler_id || keyboard_flag)
            video_mem[(i << 1) + 1] = prompt_color;
        else
            video_mem[(i << 1) + 1] = terminal[scheduler_id].t_prompt_color;
    }
    int32_t curr_scheduler = scheduler_id;
    if(terminal_id == curr_scheduler || keyboard_flag){
        screen_x = 0;
        screen_y = NUM_ROWS - 1;
        update_cursor(screen_x, screen_y);
        return;
    }
    terminal[curr_scheduler].t_screen_x = 0;
    terminal[curr_scheduler].t_screen_y = NUM_ROWS - 1;
    // scroll_flag = 0;
}

/* void enable_cursor(uint8_t start, uint8_t end);
 * Inputs: start, end
 * Return Value: none
 *  Function: function to set the cursor */
void enable_cursor(uint8_t start, uint8_t end){
    outb(0x0A, CURSOR_PORT);    // select cursor start register in port
    outb((inb(CURSOR_DATA) & 0xC0) | start, CURSOR_DATA);  // start - start cursor shape
    outb(0x0B, CURSOR_PORT);    // select cursor end register in port
    outb((inb(CURSOR_DATA) & 0xE0) | end, CURSOR_DATA);   // end - end cursor shape
}

/* void disable_cursor();
 * Inputs:  none
 * Return Value: none
 *  Function: function to disable the cursor */
void disable_cursor(uint8_t start, uint8_t end){
    outb(0x0A, CURSOR_PORT);  // low cursor shape register
    outb(0x20, CURSOR_DATA);  // bit 5 disables the cursor
}

/* void update_cursor(int x, int y);
 * Inputs: x, y position of the cursor
 * Return Value: none
 *  Function: function to update the cursor */
void update_cursor(int x, int y){
    uint16_t position = y * NUM_COLS + x;  // calculate the position
    outb(0x0F, CURSOR_PORT);  // lower byte
    outb((uint8_t) (position & 0xFF), CURSOR_DATA);
    outb(0x0E, CURSOR_PORT);  // higher byte
    outb((uint8_t) ((position>>8) & 0xFF), CURSOR_DATA);
}


/* int8_t* itoa(uint32_t value, int8_t* buf, int32_t radix);
 * Inputs: uint32_t value = number to convert
 *            int8_t* buf = allocated buffer to place string in
 *          int32_t radix = base system. hex, oct, dec, etc.
 * Return Value: number of bytes written
 * Function: Convert a number to its ASCII representation, with base "radix" */
int8_t* itoa(uint32_t value, int8_t* buf, int32_t radix) {
    static int8_t lookup[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    int8_t *newbuf = buf;
    int32_t i;
    uint32_t newval = value;

    /* Special case for zero */
    if (value == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        return buf;
    }

    /* Go through the number one place value at a time, and add the
     * correct digit to "newbuf".  We actually add characters to the
     * ASCII string from lowest place value to highest, which is the
     * opposite of how the number should be printed.  We'll reverse the
     * characters later. */
    while (newval > 0) {
        i = newval % radix;
        *newbuf = lookup[i];
        newbuf++;
        newval /= radix;
    }

    /* Add a terminating NULL */
    *newbuf = '\0';

    /* Reverse the string and return */
    return strrev(buf);
}

/* int8_t* strrev(int8_t* s);
 * Inputs: int8_t* s = string to reverse
 * Return Value: reversed string
 * Function: reverses a string s */
int8_t* strrev(int8_t* s) {
    register int8_t tmp;
    register int32_t beg = 0;
    register int32_t end = strlen(s) - 1;

    while (beg < end) {
        tmp = s[end];
        s[end] = s[beg];
        s[beg] = tmp;
        beg++;
        end--;
    }
    return s;
}

/* uint32_t strlen(const int8_t* s);
 * Inputs: const int8_t* s = string to take length of
 * Return Value: length of string s
 * Function: return length of string s */
uint32_t strlen(const int8_t* s) {
    register uint32_t len = 0;
    while (s[len] != '\0')
        len++;
    return len;
}

/* void* memset(void* s, int32_t c, uint32_t n);
 * Inputs:    void* s = pointer to memory
 *          int32_t c = value to set memory to
 *         uint32_t n = number of bytes to set
 * Return Value: new string
 * Function: set n consecutive bytes of pointer s to value c */
void* memset(void* s, int32_t c, uint32_t n) {
    c &= 0xFF;
    asm volatile ("                 \n\
            .memset_top:            \n\
            testl   %%ecx, %%ecx    \n\
            jz      .memset_done    \n\
            testl   $0x3, %%edi     \n\
            jz      .memset_aligned \n\
            movb    %%al, (%%edi)   \n\
            addl    $1, %%edi       \n\
            subl    $1, %%ecx       \n\
            jmp     .memset_top     \n\
            .memset_aligned:        \n\
            movw    %%ds, %%dx      \n\
            movw    %%dx, %%es      \n\
            movl    %%ecx, %%edx    \n\
            shrl    $2, %%ecx       \n\
            andl    $0x3, %%edx     \n\
            cld                     \n\
            rep     stosl           \n\
            .memset_bottom:         \n\
            testl   %%edx, %%edx    \n\
            jz      .memset_done    \n\
            movb    %%al, (%%edi)   \n\
            addl    $1, %%edi       \n\
            subl    $1, %%edx       \n\
            jmp     .memset_bottom  \n\
            .memset_done:           \n\
            "
            :
            : "a"(c << 24 | c << 16 | c << 8 | c), "D"(s), "c"(n)
            : "edx", "memory", "cc"
    );
    return s;
}

/* void* memset_word(void* s, int32_t c, uint32_t n);
 * Description: Optimized memset_word
 * Inputs:    void* s = pointer to memory
 *          int32_t c = value to set memory to
 *         uint32_t n = number of bytes to set
 * Return Value: new string
 * Function: set lower 16 bits of n consecutive memory locations of pointer s to value c */
void* memset_word(void* s, int32_t c, uint32_t n) {
    asm volatile ("                 \n\
            movw    %%ds, %%dx      \n\
            movw    %%dx, %%es      \n\
            cld                     \n\
            rep     stosw           \n\
            "
            :
            : "a"(c), "D"(s), "c"(n)
            : "edx", "memory", "cc"
    );
    return s;
}

/* void* memset_dword(void* s, int32_t c, uint32_t n);
 * Inputs:    void* s = pointer to memory
 *          int32_t c = value to set memory to
 *         uint32_t n = number of bytes to set
 * Return Value: new string
 * Function: set n consecutive memory locations of pointer s to value c */
void* memset_dword(void* s, int32_t c, uint32_t n) {
    asm volatile ("                 \n\
            movw    %%ds, %%dx      \n\
            movw    %%dx, %%es      \n\
            cld                     \n\
            rep     stosl           \n\
            "
            :
            : "a"(c), "D"(s), "c"(n)
            : "edx", "memory", "cc"
    );
    return s;
}

/* void* memcpy(void* dest, const void* src, uint32_t n);
 * Inputs:      void* dest = destination of copy
 *         const void* src = source of copy
 *              uint32_t n = number of byets to copy
 * Return Value: pointer to dest
 * Function: copy n bytes of src to dest */
void* memcpy(void* dest, const void* src, uint32_t n) {
    asm volatile ("                 \n\
            .memcpy_top:            \n\
            testl   %%ecx, %%ecx    \n\
            jz      .memcpy_done    \n\
            testl   $0x3, %%edi     \n\
            jz      .memcpy_aligned \n\
            movb    (%%esi), %%al   \n\
            movb    %%al, (%%edi)   \n\
            addl    $1, %%edi       \n\
            addl    $1, %%esi       \n\
            subl    $1, %%ecx       \n\
            jmp     .memcpy_top     \n\
            .memcpy_aligned:        \n\
            movw    %%ds, %%dx      \n\
            movw    %%dx, %%es      \n\
            movl    %%ecx, %%edx    \n\
            shrl    $2, %%ecx       \n\
            andl    $0x3, %%edx     \n\
            cld                     \n\
            rep     movsl           \n\
            .memcpy_bottom:         \n\
            testl   %%edx, %%edx    \n\
            jz      .memcpy_done    \n\
            movb    (%%esi), %%al   \n\
            movb    %%al, (%%edi)   \n\
            addl    $1, %%edi       \n\
            addl    $1, %%esi       \n\
            subl    $1, %%edx       \n\
            jmp     .memcpy_bottom  \n\
            .memcpy_done:           \n\
            "
            :
            : "S"(src), "D"(dest), "c"(n)
            : "eax", "edx", "memory", "cc"
    );
    return dest;
}

/* void* memmove(void* dest, const void* src, uint32_t n);
 * Description: Optimized memmove (used for overlapping memory areas)
 * Inputs:      void* dest = destination of move
 *         const void* src = source of move
 *              uint32_t n = number of byets to move
 * Return Value: pointer to dest
 * Function: move n bytes of src to dest */
void* memmove(void* dest, const void* src, uint32_t n) {
    asm volatile ("                             \n\
            movw    %%ds, %%dx                  \n\
            movw    %%dx, %%es                  \n\
            cld                                 \n\
            cmp     %%edi, %%esi                \n\
            jae     .memmove_go                 \n\
            leal    -1(%%esi, %%ecx), %%esi     \n\
            leal    -1(%%edi, %%ecx), %%edi     \n\
            std                                 \n\
            .memmove_go:                        \n\
            rep     movsb                       \n\
            "
            :
            : "D"(dest), "S"(src), "c"(n)
            : "edx", "memory", "cc"
    );
    return dest;
}

/* int32_t strncmp(const int8_t* s1, const int8_t* s2, uint32_t n)
 * Inputs: const int8_t* s1 = first string to compare
 *         const int8_t* s2 = second string to compare
 *               uint32_t n = number of bytes to compare
 * Return Value: A zero value indicates that the characters compared
 *               in both strings form the same string.
 *               A value greater than zero indicates that the first
 *               character that does not match has a greater value
 *               in str1 than in str2; And a value less than zero
 *               indicates the opposite.
 * Function: compares string 1 and string 2 for equality */
int32_t strncmp(const int8_t* s1, const int8_t* s2, uint32_t n) {
    int32_t i;
    for (i = 0; i < n; i++) {
        if ((s1[i] != s2[i]) || (s1[i] == '\0') /* || s2[i] == '\0' */) {

            /* The s2[i] == '\0' is unnecessary because of the short-circuit
             * semantics of 'if' expressions in C.  If the first expression
             * (s1[i] != s2[i]) evaluates to false, that is, if s1[i] ==
             * s2[i], then we only need to test either s1[i] or s2[i] for
             * '\0', since we know they are equal. */
            return s1[i] - s2[i];
        }
    }
    return 0;
}

/* int8_t* strcpy(int8_t* dest, const int8_t* src)
 * Inputs:      int8_t* dest = destination string of copy
 *         const int8_t* src = source string of copy
 * Return Value: pointer to dest
 * Function: copy the source string into the destination string */
int8_t* strcpy(int8_t* dest, const int8_t* src) {
    int32_t i = 0;
    while (src[i] != '\0') {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
    return dest;
}

/* int8_t* strcpy(int8_t* dest, const int8_t* src, uint32_t n)
 * Inputs:      int8_t* dest = destination string of copy
 *         const int8_t* src = source string of copy
 *                uint32_t n = number of bytes to copy
 * Return Value: pointer to dest
 * Function: copy n bytes of the source string into the destination string */
int8_t* strncpy(int8_t* dest, const int8_t* src, uint32_t n) {
    int32_t i = 0;
    while (src[i] != '\0' && i < n) {
        dest[i] = src[i];
        i++;
    }
    while (i < n) {
        dest[i] = '\0';
        i++;
    }
    return dest;
}

/* void test_interrupts(void)
 * Inputs: void
 * Return Value: void
 * Function: increments video memory. To be used to test rtc */
void test_interrupts(void) {
    int32_t i;
    for (i = 0; i < NUM_ROWS * NUM_COLS; i++) {
        video_mem[i << 1]++;
    }
}

/* void screen_backspace(void)
 * Inputs: void
 * Return Value: void
 * Function: clear the last memory address if possible (including the consideration of line changing)*/
void screen_backspace(void){
    if(screen_x == 0){
        // cursor is at the start of the screen, no need to backspace
        if(screen_y == 0) return;
        screen_y--;
        screen_x = NUM_COLS - 1;
    }
    else
        screen_x--;
    // set the deleted to space
    *(uint8_t *)(video_mem + ((NUM_COLS * screen_y + screen_x) << 1)) = ' ';
    *(uint8_t *)(video_mem + ((NUM_COLS * screen_y + screen_x) << 1) + 1) = prompt_color;
    update_cursor(screen_x, screen_y);
}

/* 
 * get_curr_terminal: get the current terminal_id
 * Input: none
 * Output: none
 * Return value: the current terminal_id
 * Side effect: none
*/
int32_t get_curr_terminal(){
    return terminal_id;
}

/* 
 * change_curr_terminal: switch to a terminal (via alt + F1/F2/F3)
 * Input: idx
 * Output: none
 * Return value: none
 * Side effect: change the terminal to the given index
*/
void change_curr_terminal(int32_t idx){
    // check validity
    cli();
    if(idx < 0 || idx > 2 || idx == terminal_id) return;
    // save current screen context to the corresponding struct
    terminal[terminal_id].t_screen_x = screen_x;
    terminal[terminal_id].t_screen_y = screen_y;
    terminal[terminal_id].t_newline_flag = newline_flag;
    terminal[terminal_id].t_prompt_color = prompt_color;
    update_video_mapping(terminal_id);
    memcpy((uint8_t *)t_video_mem[terminal_id], (uint8_t *)video_mem, MEM_SIZE);

    // update to new screen context from the correct struct
    terminal_id = idx;
    screen_x = terminal[terminal_id].t_screen_x;
    screen_y = terminal[terminal_id].t_screen_y;
    newline_flag = terminal[terminal_id].t_newline_flag;
    prompt_color = terminal[terminal_id].t_prompt_color;
    memcpy((uint8_t *)video_mem, (uint8_t *)t_video_mem[terminal_id], MEM_SIZE);
    int32_t pid_now = get_pid();
    restore_video_mapping(pid_now); // update the mapping and cursor
    update_cursor(screen_x, screen_y);
    sti();
}

/* 
 * get_curr_scheduler: get the current scheduler_id
 * Input: none
 * Output: none
 * Return value: the current scheduler_id
 * Side effect: none
*/
int32_t get_curr_scheduler(){
    return scheduler_id;
}

/* 
 * change_curr_scheduler: switch to a scheduler)
 * Input: idx
 * Output: none
 * Return value: none
 * Side effect: change the scheduler to the given index
*/
void change_curr_scheduler(int32_t idx){
    scheduler_id = idx;
}

/* 
 * enable_keyboard: set the keyboard_flag to 1 (so that we can putc to the current terminal)
 * Input: none
 * Output: none
 * Return value: none
 * Side effect: enable the keyboard in putc
*/
void enable_keyboard(){
    keyboard_flag = 1;
}

/* 
 * disable_keyboard: set the keyboard_flag to 0 (so that we can use putc correctly)
 * Input: none
 * Output: none
 * Return value: none
 * Side effect: disable the keyboard in putc
*/
void disable_keyboard(){
    keyboard_flag = 0;
}

/* 
 * up_history: browse back the history of buffer
 * Input: none
 * Output: none
 * Return value: none
 * Side effect: go back once in the history of buffer
*/
void up_history(){
    // update screen_x and screen_y
    screen_x += terminal[terminal_id].buf_cnt - terminal[terminal_id].curr_cur;
    screen_y = (screen_y + (screen_x / NUM_COLS)) % NUM_ROWS;
    screen_x %= NUM_COLS;
    // already the oldest command in history
    if(terminal[terminal_id].hist_flag == 2) return;
    if(terminal[terminal_id].oldest_idx == -1 && terminal[terminal_id].curr_idx == MAX_HIS -1) return;
    // clear keyboard_buffer and the command line in screen
    int i, bufc;
    bufc = terminal[terminal_id].buf_cnt;
    for(i = 0; i < bufc; i++) screen_backspace();
    memset(terminal[terminal_id].keyboard_buf, 0, BUF_SIZE); // clear the buffer
    terminal[terminal_id].buf_cnt = 0;
    // print the command and store it in keyboard buffer
    for(i = 0; i < BUF_SIZE; i++){
        int idx = terminal[terminal_id].curr_idx;
        char c = terminal[terminal_id].history[idx][i];
        if(c!=0){
            putc(c);
            terminal[terminal_id].keyboard_buf[terminal[terminal_id].buf_cnt] = c;
            terminal[terminal_id].buf_cnt++;
        }
    }
    terminal[terminal_id].curr_cur = terminal[terminal_id].buf_cnt;
    terminal[terminal_id].curr_idx = (terminal[terminal_id].curr_idx - 1 + MAX_HIS) % MAX_HIS;
    // set a flag of the position if it leaves the dual position
    if(terminal[terminal_id].hist_flag == 0) terminal[terminal_id].hist_flag = 1;
    // set a flag of the position if it is in the right(older) of the dual position
    if(((terminal[terminal_id].curr_idx + 1) % MAX_HIS) == terminal[terminal_id].oldest_idx) terminal[terminal_id].hist_flag = 2;
}

/* 
 * up_history: browse foward the history of buffer
 * Input: none
 * Output: none
 * Return value: none
 * Side effect: go foward once in the history of buffer
*/
void down_history(){
    // update screen_x and screen_y
    screen_x += terminal[terminal_id].buf_cnt - terminal[terminal_id].curr_cur;
    screen_y = (screen_y + (screen_x / NUM_COLS)) % NUM_ROWS;
    screen_x %= NUM_COLS;
    // the current is not in history, nothing to do
    if(terminal[terminal_id].hist_flag == 0) return;
    // clear keyboard_buffer and the command line in screen
    int i, bufc;
    bufc = terminal[terminal_id].buf_cnt;
    for(i = 0; i < bufc; i++) screen_backspace();
    memset(terminal[terminal_id].keyboard_buf, 0, BUF_SIZE); // clear the buffer
    terminal[terminal_id].buf_cnt = 0;
    // already the newest command in history, leave it blank
    if(((terminal[terminal_id].curr_idx + 2) % MAX_HIS) == terminal[terminal_id].newest_idx){
        terminal[terminal_id].curr_idx = (terminal[terminal_id].curr_idx + 1) % MAX_HIS;
        // set a flag of the position if it is in the left(newer) of the dual position
        terminal[terminal_id].hist_flag = 0;
        terminal[terminal_id].curr_cur = terminal[terminal_id].buf_cnt;
        return;
    }
    // print the command and store it in keyboard buffer
    for(i = 0; i < BUF_SIZE; i++){
        char c = terminal[terminal_id].history[(terminal[terminal_id].curr_idx + 2) % MAX_HIS][i];
        if(c!=0){
            putc(c);
            terminal[terminal_id].keyboard_buf[terminal[terminal_id].buf_cnt] = c;
            terminal[terminal_id].buf_cnt++;
        }
    }
    terminal[terminal_id].curr_cur = terminal[terminal_id].buf_cnt;
    terminal[terminal_id].curr_idx = (terminal[terminal_id].curr_idx + 1) % MAX_HIS;
    // set a flag of the position if it leaves the dual position
    if(terminal[terminal_id].hist_flag == 2) terminal[terminal_id].hist_flag = 1;
}

/* 
 * buffer_shift: shift the buffer left / right
 * Input: none
 * Output: none
 * Return value: none
 * Side effect: go left / right in the buffer
*/
void buffer_shift(int step){
    int bufc;
    bufc=terminal[terminal_id].buf_cnt;
    if(bufc==0) return;
    if((step==-1) && (terminal[terminal_id].curr_cur>0)){
        terminal[terminal_id].curr_cur--;
        if(screen_x == 0){
            // cursor is at the start of the screen, no need to backspace
            if(screen_y == 0) return;
            screen_y--;
            screen_x = NUM_COLS - 1;
        }
        else
            screen_x--;
        update_cursor(screen_x, screen_y);
    }
    else if((step==1) && (terminal[terminal_id].curr_cur<terminal[terminal_id].buf_cnt)) {
        terminal[terminal_id].curr_cur++;
        if(screen_x == NUM_COLS-1){
            // cursor is at the start of the screen, no need to backspace
            screen_y++;
            screen_x = 0;
        }
        else
            screen_x++;
        update_cursor(screen_x, screen_y);
    }

}


void line_shift(int step){
    int bufc, i, j;
    bufc=terminal[terminal_id].buf_cnt;
    if(bufc==0) return;
    // delete
    if((step==-1) && (terminal[terminal_id].curr_cur>0)){
        screen_backspace();
        // keyboard buffer and screen
        j = 0;
        for(i = terminal[terminal_id].curr_cur; i < bufc; i++){
            char c = terminal[terminal_id].keyboard_buf[i];
            terminal[terminal_id].keyboard_buf[i - 1] = c;
            *(uint8_t *)(video_mem + ((NUM_COLS * screen_y + screen_x + j) << 1)) = c;
            *(uint8_t *)(video_mem + ((NUM_COLS * screen_y + screen_x + j) << 1) + 1) = prompt_color;
            j++;
        }
        *(uint8_t *)(video_mem + ((NUM_COLS * screen_y + screen_x + j) << 1)) = ' ';
        *(uint8_t *)(video_mem + ((NUM_COLS * screen_y + screen_x + j) << 1) + 1) = prompt_color;
        terminal[terminal_id].keyboard_buf[bufc - 1] = 0;
        terminal[terminal_id].curr_cur--;
        terminal[terminal_id].buf_cnt--;
    }
    // add
    else if((step==1) && (terminal[terminal_id].curr_cur<bufc)) {
        j = bufc - terminal[terminal_id].curr_cur;
        for(i = bufc-1; i > terminal[terminal_id].curr_cur-1; i--){
            char c = terminal[terminal_id].keyboard_buf[i];
            terminal[terminal_id].keyboard_buf[i + 1] = c;
            *(uint8_t *)(video_mem + ((NUM_COLS * screen_y + screen_x + j) << 1)) = c;
            *(uint8_t *)(video_mem + ((NUM_COLS * screen_y + screen_x + j) << 1) + 1) = prompt_color;
            j--;
        }
        terminal[terminal_id].curr_cur++;
        terminal[terminal_id].buf_cnt++;
    }

}

void recover_shift(){
        screen_x += terminal[terminal_id].buf_cnt - terminal[terminal_id].curr_cur;
        if(screen_x > NUM_COLS-1){
            // cursor is at the start of the screen, no need to backspace
            screen_y++;
            screen_x%=NUM_COLS;
        }
}

void print_color(){
    char* prompt = "Welcome to use the color editor!\nType: color                no arguments to print this prompt\nType: color x x            1st arg - 0 for background color, 1 for font color\n                           2nd arg - 0-F for colors as below\nThe color same as the background color can not be seen!!!\n";
    puts(prompt);
    uint8_t restore = prompt_color;
    uint8_t restore2;
    char* str00 = "0 - ";
    puts((int8_t *)str00);
    prompt_color = prompt_color & 0xF0 ;
    char* str0 = "black\n";
    puts((int8_t *)str0);
    prompt_color++;
    restore2 = prompt_color;
    prompt_color = restore;
    char* str11 = "1 - ";
    puts(str11);
    prompt_color = restore2;
    char* str1 = "blue\n";
    puts((int8_t *)str1);
    prompt_color++;
    restore2 = prompt_color;
    prompt_color = restore;
    char* str22 = "2 - ";
    puts((int8_t *)str22);
    prompt_color = restore2;
    char* str2 = "green\n";
    puts((int8_t *)str2);
    prompt_color++;
    restore2 = prompt_color;
    prompt_color = restore;
    char* str33 = "3 - ";
    puts((int8_t *)str33);
    prompt_color = restore2;
    char* str3 = "sapphire blue\n";
    puts((int8_t *)str3);
    prompt_color++;
    restore2 = prompt_color;
    prompt_color = restore;
    char* str44 = "4 - ";
    puts((int8_t *)str44);
    prompt_color = restore2;
    char* str4 = "red\n";
    puts((int8_t *)str4);
    prompt_color++;
    restore2 = prompt_color;
    prompt_color = restore;
    char* str55 = "5 - ";
    puts((int8_t *)str55);
    prompt_color = restore2;
    char* str5 = "purple\n";
    puts((int8_t *)str5);
    prompt_color++;
    restore2 = prompt_color;
    prompt_color = restore;
    char* str66 = "6 - ";
    puts((int8_t *)str66);
    prompt_color = restore2;
    char* str6 = "orange\n";
    puts((int8_t *)str6);
    prompt_color++;
    restore2 = prompt_color;
    prompt_color = restore;
    char* str77 = "7 - ";
    puts((int8_t *)str77);
    prompt_color = restore2;
    char* str7 = "light grey\n";
    puts((int8_t *)str7);
    prompt_color++;
    restore2 = prompt_color;
    prompt_color = restore;
    char* str88 = "8 - ";
    puts((int8_t *)str88);
    prompt_color = restore2;
    char* str8 = "dark grey\n";
    puts((int8_t *)str8);
    prompt_color++;
    restore2 = prompt_color;
    prompt_color = restore;
    char* str99 = "9 - ";
    puts((int8_t *)str99);
    prompt_color = restore2;
    char* str9 = "berry blue\n";
    puts((int8_t *)str9);
    prompt_color++;
    restore2 = prompt_color;
    prompt_color = restore;
    char* strAA = "A - ";
    puts((int8_t *)strAA);
    prompt_color = restore2;
    char* strA = "mint green\n";
    puts((int8_t *)strA);
    prompt_color++;
    restore2 = prompt_color;
    prompt_color = restore;
    char* strBB = "B - ";
    puts((int8_t *)strBB);
    prompt_color = restore2;
    char* strB = "sky blue\n";
    puts((int8_t *)strB);
    prompt_color++;
    restore2 = prompt_color;
    prompt_color = restore;
    char* strCC = "C - ";
    puts((int8_t *)strCC);
    prompt_color = restore2;
    char* strC = "orange red\n";
    puts((int8_t *)strC);
    prompt_color++;
    restore2 = prompt_color;
    prompt_color = restore;
    char* strDD = "D - ";
    puts((int8_t *)strDD);
    prompt_color = restore2;
    char* strD = "pink\n";
    puts((int8_t *)strD);
    prompt_color++;
    restore2 = prompt_color;
    prompt_color = restore;
    char* strEE = "E - ";
    puts((int8_t *)strEE);
    prompt_color = restore2;
    char* strE = "yellow\n";
    puts((int8_t *)strE);
    prompt_color++;
    restore2 = prompt_color;
    prompt_color = restore;
    char* strFF = "F - ";
    puts((int8_t *)strFF);
    prompt_color = restore2;
    char* strF = "white\n";
    puts((int8_t *)strF);
    prompt_color = restore;
}

int32_t setcolor(uint8_t* arg){
    if(arg == 0) return -1;
    if(arg[0] == '\0' || arg[1] == '\0') return -1;
    if(arg[0] < 48 || arg[0] > 49 || arg[1] < 48 || arg[1] > 70) return -1;
    if(arg[1] < 65 && arg[1] > 57) return -1;
    uint8_t char1 = (uint8_t)arg[0] - 48;
    uint8_t char2;
    if(arg[1] <= 57) char2 = (uint8_t)arg[1] - 48;
    else char2 = (uint8_t)arg[1] - 55;
    if(char1 == 0){
        // background
        prompt_color = (char2 << 4)|(prompt_color & 0xF);
        set_fullscreen_color(0);
    }
    else if(char1 == 1){
        // text
        prompt_color = (char2 & 0xF)|(prompt_color & 0xF0);
        set_fullscreen_color(1);
    }
    else return -1;
    return 0;
}


void set_fullscreen_color(int32_t indicator){
    if((indicator!=0) && (indicator!=1)) return;
    int32_t i;
    for (i = 0; i < NUM_ROWS * NUM_COLS; i++) {
        if(indicator == 0){
            uint8_t old_color = *(uint8_t *)(video_mem + (i << 1) + 1);
            *(uint8_t *)(video_mem + (i << 1) + 1) = (prompt_color & 0xF0)|(old_color & 0x0F);
        }
        else *(uint8_t *)(video_mem + (i << 1) + 1) = prompt_color;
    }
}

