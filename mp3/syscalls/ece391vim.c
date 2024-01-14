#include <stdint.h>

#include "ece391support.h"
#include "ece391syscall.h"

#define MAX_HORIZON  160
#define MAX_VERTICAL 100

#define SCREEN_WIDTH  80
#define SCREEN_HEIGHT 25
#define BOTTOM_LINE   24

#define NORMAL_MODE  0
#define INSERT_MODE  1
#define COMMAND_MODE 2


int8_t text[MAX_VERTICAL][MAX_HORIZON];
uint8_t screen[SCREEN_HEIGHT*SCREEN_WIDTH];
uint8_t screen_backup[SCREEN_HEIGHT*SCREEN_WIDTH];
int8_t curr_key[1];
int32_t cursor_x, cursor_y;
int32_t start_x, start_y;
int32_t rows, cols;

int32_t mode;
int32_t saved;

int32_t fd;
int8_t filename[32];

int32_t word_count;



void vim_init()
{
    int32_t i, j;
    for (i = 0; i < MAX_VERTICAL; i++) {
        for (j = 0; j < MAX_HORIZON; j++) {
            text[i][j] = '\0';
        }
    }

    clear_screen();

    cursor_x = 0;
    cursor_y = 0;

    start_x = 0;
    start_y = 0;

    rows = 0;
    cols = 0;

    mode = NORMAL_MODE;
    saved = 0;

    fd = -1;
    for (i = 0; i < 32; i++) {
        filename[i] = '\0';
    }

    word_count = 0;

    ece391_ioctl(0, 0, 0);         // set terminal and keyboard to raw mode
}


int main ()
{
    int32_t dir_fd, cnt;
    int32_t i, j, sentinel, ret_val;
    uint8_t name[32 + 1];
    uint8_t buf[MAX_HORIZON*MAX_VERTICAL];

    if (0 != ece391_getargs (name, 32)) {
        ece391_fdputs (1, (uint8_t*)"could not read file name\n");
    return 3;
    }
    /* open file, if not exist, create one */
    if (-1 == (fd = ece391_open (buf))) {
        ece391_fdputs (1, (uint8_t*)"file not found, creating...\n");
        dir_fd = ece391_open ((uint8_t*)".");
        ece391_write (dir_fd, name, 32);        // 新建文件是啥
    }
    while (0 != (cnt = ece391_read (fd, buf, MAX_HORIZON*MAX_VERTICAL))) {
        if (-1 == cnt) {
            ece391_fdputs (1, (uint8_t*)"file read failed\n");
            return 3;
        }
    }

    vim_init();
    ece391_vidmap((uint8_t**)(&screen));
    word_count = cnt;

    ret_val = read_file(buf, cnt);
    if (ret_val == 0) {
        cursor_x = 0;
        cursor_y = 0;
    }
    else {
        ece391_fdputs (1, (uint8_t*)"file read failed\n");
    }

    show_content(0, 0);

    /* read from keyboard */
    while (1) {
        ece391_read (0, curr_key, 1);
        parse_key();
        update_cursor();
        show_content(start_x, start_y);
        show_bar();
    }
    return 0;
}





void vim_puts(int8_t* s, int32_t x, int32_t y)
{
    int32_t i;
    for (i = 0; s[i] != '\0'; i++) {
        vim_putc(s[i], x+i, y);
    }
}

void vim_putc(int8_t c, int32_t x, int32_t y)
{
    int8_t _c = c;
    if (c == '\n') {
        x = 0;
        y++;
        cursor_x = 0;
        cursor_y++;
    }
    else if (c == '\b') {
        if (x > 0)
            x--;
        _c = ' ';
        cursor_x--;
    }
    else if (c == '\t') {           // 2 spaces
        vim_putc(' ', x, y);
        vim_putc(' ', x+1, y);
        cursor_x += 2;
    }
    else if (c == '\r') {
        x = 0;
    }
    else if (c == '\f') {
        y = 0;
    }
    else if (c == '\v') {
        y++;
    }
    else if (c == '\0') {
        _c = ' ';
    }

    screen[y*SCREEN_WIDTH + x] = c;
}


int32_t read_file(uint8_t* buf, int32_t cnt) 
{
    int32_t i, j, sentinel;
    sentinel = 0;
    for (i = 0; i < MAX_VERTICAL; i++) {
        if (sentinel >= cnt)
            break;
        for (j = 0; j < MAX_HORIZON; j++) {
            if (buf[sentinel + j] == '\n') {
                sentinel = j+1;
                break;
            }
            sentinel = j+1;
            text[i][j] = buf[sentinel + j];
        }
    }

    if (sentinel >= cnt)
        return 0;
    else
        return sentinel;
}





/* save file */
int32_t save_file()
{
    int32_t x, y, sentinel;
    uint8_t buf[MAX_HORIZON*MAX_VERTICAL];
    sentinel = 0;
    for (y = 0; y < MAX_VERTICAL; y++) {
        if (sentinel >= word_count)
            break;
        for (x = 0; x < MAX_HORIZON; x++) {
            buf[sentinel] = text[y][x];
            sentinel++;
            if (text[y][x] == '\n')
                break;
        }
    }
    ece391_write(fd, buf, word_count);
}

void quit()
{
    if (saved == 0) {
        vim_puts ((uint8_t*)"Save file before quit? (y/n): ", 0, BOTTOM_LINE);
        ece391_read (0, curr_key, 1);
        if (curr_key[0] == 'y') {
            save_file();
        }
    }
    ece391_close(fd);
    ece391_halt(0);
}

void clear_screen()
{
    int32_t i;
    for (i = 0; i < SCREEN_HEIGHT*SCREEN_WIDTH; i++) {
        screen_backup[i] = screen[i];
        screen[i] = ' ';
    }
}

void restore_screen()
{
    int32_t i;
    for (i = 0; i < SCREEN_HEIGHT*SCREEN_WIDTH; i++) {
        screen[i] = screen_backup[i];
    }
}





void show_content(int32_t start_x, int32_t start_y)
{
    int32_t x, y;
    for (y = 0; y < SCREEN_HEIGHT; y++) {
        ece391_fdputs (1, text[y]);
    }
}

void show_bar()
{
    switch(mode) {
        case NORMAL_MODE:
            vim_puts((int8_t*)"-NORMAL MODE-", 0, BOTTOM_LINE);
            break;
        case INSERT_MODE:
            vim_puts((int8_t*)"-INSERT MODE-", 0, BOTTOM_LINE);
            break;
        case COMMAND_MODE:
            vim_puts((int8_t*)"-COMMAND MODE-", 0, BOTTOM_LINE);
            break;
        default:
            break;
    }
}





void parse_key()
{
    switch (mode) {
        case NORMAL_MODE:
            parse_normal_key();
            break;
        case INSERT_MODE:
            parse_insert_key();
            break;
        case COMMAND_MODE:
            parse_command_key();
            break;
        default:
            break;
    }
}

void parse_normal_key()
{
    switch (curr_key[0]) {
        case 'i':
            mode = INSERT_MODE;
            break;
        case ':':
            mode = COMMAND_MODE;
            break;
        case 'h':
            if (cursor_x > 0)
                cursor_x--;
            break;
        case 'j':
            if (cursor_y < SCREEN_HEIGHT)
                cursor_y++;
            break;
        case 'k':
            if (cursor_y > 0)
                cursor_y--;
            break;
        case 'l':
            if (cursor_x < SCREEN_WIDTH)
                cursor_x++;
            break;
        default:
            break;
    }
}

void parse_insert_key()
{
    switch (curr_key[0]) {
        case 27:                // ESC
            mode = NORMAL_MODE;
            break;
        case 127:               // Backspace
            if (cursor_x > 0)
                cursor_x--;
            text[cursor_y][cursor_x] = ' ';
            break;
        case '\n':
            cursor_x = 0;
            cursor_y++;
            break;
        default:
            text[cursor_y][cursor_x] = curr_key[0];
            cursor_x++;
            break;
    }
}

void parse_command_key()
{
    switch (curr_key[0]) {
        case 27:                // ESC
            mode = NORMAL_MODE;
            break;
        case 'w':
            save_file();
            break;
        case 'q':
            quit();
            break;
        default:
            break;
    }
}




void update_cursor()
{
    int32_t i, j;
    for (i = 0; i < SCREEN_HEIGHT; i++) {
        for (j = 0; j < SCREEN_WIDTH; j++) {
            if (i == cursor_y && j == cursor_x) {
                text[i][j] = 'X';
            }
            else {
                text[i][j] = ' ';
            }
        }
    }
}


void scroll_up()
{
    if (start_y > 0)
        start_y--;
}

void scroll_down()
{
    if (start_y < MAX_VERTICAL - SCREEN_HEIGHT)
        start_y++;
}

void scroll_left()
{
    if (start_x > 0)
        start_x--;
}

void scroll_right()
{
    if (start_x < MAX_HORIZON - SCREEN_WIDTH)
        start_x++;
}




