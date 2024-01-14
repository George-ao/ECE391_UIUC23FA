#include "terminal.h"


file_descriptor_t file_descriptor_table[MAX_FD_ENTRIES];



/* 
 * terminal_init: initialize every thing in terminal struct
 * Input: none
 * Output: none
 * Return value: none
 * Side effect: set the buffer to be NUL
*/
void terminal_init(){
    // set all characters in buffer to be NUL
    int i;
    for(i = 0; i < MAX_NUM; i++){
        memset(terminal[i].keyboard_buf, 0, BUF_SIZE);
        memset(terminal[i].history, 0, BUF_SIZE*MAX_HIS);
        // generally in 3.5, we modify terminal to be a three-member array for each terminal
        terminal[i].curr_cur = 0;
        terminal[i].buf_cnt = 0;
        terminal[i].read_flag = 0;
        terminal[i].oldest_idx = -1;
        terminal[i].newest_idx = 0;
        terminal[i].curr_idx = 0;
        terminal[i].hist_flag = 0;
        terminal[i].t_screen_x = 0;
        terminal[i].t_screen_y = 0;
        terminal[i].t_newline_flag = 0;
        terminal[i].t_prompt_color = 7;
        terminal[i].tab_indic = 0;
    }
}

/* 
 * terminal_open: set the read flag to be  0 and set the buffer to be NUL
 * Input: filename - not used
 * Output: none
 * Return value: always 0
 * Side effect: set the buffer to be NUL
*/
int32_t terminal_open(const uint8_t* filename){
    // set all characters in buffer to be NUL
    int i = get_curr_scheduler();
    memset(terminal[i].keyboard_buf, 0, BUF_SIZE);
     memset(terminal[i].history, 0, BUF_SIZE*MAX_HIS);
    // generally in 3.5, we modify terminal to be a three-member array for each terminal
    terminal[i].curr_cur = 0;
    terminal[i].buf_cnt = 0;
    terminal[i].read_flag = 0;
    terminal[i].oldest_idx = -1;
    terminal[i].newest_idx = 0;
    terminal[i].curr_idx = 0;
    terminal[i].hist_flag = 0;
    terminal[i].tab_indic = 0;
    return 0;
}

/* 
 * terminal_read: read data from the buffer, return after enter is pressed
 * Input: fd, buf, nbytes - not used, should be ignored
 * Output: none
 * Return value: always 0
 * Side effect: none
*/
int32_t terminal_read(int32_t fd, void* buf, int32_t nbytes){
    // check boundary conditions
    if(buf == NULL) return -1;
    char * buffer = (char *) buf;
    int32_t curr_terminal = get_curr_scheduler();
    int idx = terminal[curr_terminal].newest_idx;
    terminal[curr_terminal].buf_cnt = 0;
    if(nbytes < 0) return -1; // boundary tests
    if(nbytes == 0) return 0;
    if(nbytes > BUF_SIZE) nbytes = BUF_SIZE;
    // set read_flag
    terminal[curr_terminal].read_flag = 1;
    // wait for enter key pressed
    while(1){
        if(terminal[curr_terminal].buf_cnt < 0 || terminal[curr_terminal].buf_cnt > BUF_SIZE){
            memset(terminal[curr_terminal].keyboard_buf, 0, BUF_SIZE);
            terminal[curr_terminal].read_flag = 0;
            terminal[curr_terminal].buf_cnt = 0;
            terminal[curr_terminal].curr_cur = 0;
            return -1; // handle for boundary cases
        }
        if(terminal[curr_terminal].buf_cnt == 0) continue;
        if(terminal[curr_terminal].keyboard_buf[terminal[curr_terminal].buf_cnt - 1] == '\n') break;             // 1 stands for ENTER key
    }
    if(terminal[curr_terminal].buf_cnt == 1 && terminal[curr_terminal].keyboard_buf[0] == '\n'){
        memset(terminal[curr_terminal].keyboard_buf, 0, BUF_SIZE);
        terminal[curr_terminal].read_flag = 0;
        terminal[curr_terminal].buf_cnt = 0;
        terminal[curr_terminal].curr_cur = 0;
        terminal[curr_terminal].curr_idx = (idx - 1 + MAX_HIS) % MAX_HIS;
        terminal[curr_terminal].hist_flag = 0;
        return 0;
    }
    int i;
    
    for(i = 0; i < nbytes; i++){
        buffer[i] = terminal[curr_terminal].keyboard_buf[i];
        if(terminal[curr_terminal].keyboard_buf[i] == '\n') 
            terminal[curr_terminal].history[idx][i] = 0;
        else terminal[curr_terminal].history[idx][i] = terminal[curr_terminal].keyboard_buf[i];
    }
    
    memset(terminal[curr_terminal].keyboard_buf, 0, BUF_SIZE);
    // memset(terminal[curr_terminal].history[terminal[curr_terminal].newest_idx], 0, BUF_SIZE);

    terminal[curr_terminal].curr_idx = idx;
    if(terminal[curr_terminal].oldest_idx == -1 && idx == MAX_HIS - 1) terminal[curr_terminal].oldest_idx = 0;
    else if(terminal[curr_terminal].oldest_idx == idx) terminal[curr_terminal].oldest_idx = (terminal[curr_terminal].oldest_idx + 1) % MAX_HIS;
    terminal[curr_terminal].newest_idx = (idx + 1) % MAX_HIS;
    terminal[curr_terminal].hist_flag = 0;

    int tem = terminal[curr_terminal].buf_cnt - 1; // the number of character read
    terminal[curr_terminal].read_flag = 0;
    terminal[curr_terminal].buf_cnt = 0;
    terminal[curr_terminal].curr_cur = 0;
    terminal[curr_terminal].tab_indic = 0;
    return tem;
}

/* 
 * terminal_write: write the data read to the screen
 * Input: fd, buf, nbytes - not used, should be ignored
 * Output: none
 * Return value: return 0
 * Side effect: none
*/
int32_t terminal_write(int32_t fd, const void* buf, int32_t nbytes){
    // check boundary conditions
    if(buf == NULL) return -1;
    char * buffer = (char *) buf;
    if(nbytes < 0) return -1;
    if(nbytes == 0) return 0;
    // if(nbytes > BUF_SIZE) nbytes = BUF_SIZE;
    int i;
    int cnt = 0;
    for(i = 0; i < nbytes; i++){
        // skip NUL character
        if(buffer[i] == 0) continue;
        putc(buffer[i]);
        cnt++;
    }
    if(cnt == 0) return 0;
    return cnt - 1;
}

/* 
 * terminal_close: do nothing
 * Input: fd : not used, should be ignored
 * Output: none
 * Return value: always 0
 * Side effect: none
*/
int32_t terminal_close(int32_t fd){
    int i;
    for(i = 0; i < MAX_NUM; i++){
        memset(terminal[i].keyboard_buf, 0, BUF_SIZE);
        terminal[i].buf_cnt = 0;
        terminal[i].curr_cur = 0;
        terminal[i].read_flag = 0;
        terminal[i].tab_indic = 0;
    }
    return 0;
}
