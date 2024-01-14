#include "keyboard.h"
#include "lib.h"
#include "page.h"
#include "system_call.h"
#include "scheduler.h"
#include "speaker.h"

#define KEYBOARD_IRQ 1

// extern uint32_t last_modify_length = 0;

uint8_t kbshift=0, kbctrl=0, kbalt=0, kbcapslock=0, kbnumlock=0;
char scan_code_set_all_upper[SCANCODE_SET_SIZE]={
    0,0,'!','@','#','$','%','^','&','*','(',')','_','+','\b',
    '\t','Q','W','E','R','T','Y','U','I','O','P','{','}','\n',
    0,'A','S','D','F','G','H','J','K','L',':','\'','~',0,'|',
    'Z','X','C','V','B','N','M','<','>','?',0,'*',0,' ',0
}; // the partial scan code set 1 from official website, with only shift pressed

char scan_code_set[SCANCODE_SET_SIZE]={
    0,0,'1','2','3','4','5','6','7','8','9','0','-','=','\b',
    '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',
    0,'a','s','d','f','g','h','j','k','l',';','\'','`',0,'\\',
    'z','x','c','v','b','n','m',',','.','/',0,'*',0,' ',0
}; // the partial scan code set 1 from official website, with nothing else pressed

/* 
 * keyboard_init: initialize the keyboard  
 * Input: none
 * Output: none
 * Side effect: enable the interrupt from keyboard
*/
void keyboard_init(){
    enable_irq(KEYBOARD_IRQ); // keyboard is link to KEYBOARD_IRQ which is 1
}

/* 
 * keyboard_handler: the handler for keyboard  
 * Input: none
 * Output: none
 * Side effect: print the partial scancodes to the screen (and sending EOI afterwards)
*/
void keyboard_handler(){
    cli();
    enable_keyboard();
    int32_t terminal_idx = get_curr_terminal();
    int32_t pid_of_term;
    uint8_t scancode=inb(KEYBOARD_DATA);
    uint8_t asccode;
    int32_t pid;
    int tab_count=4; // counter for the spaces for tab in buffer
    update_video_mapping();
    switch (scancode)
    {
        /* special cases*/
        case PRESSED_CAPSLOCK: kbcapslock=1-kbcapslock; break;
        case PRESSED_NUMLOCK: kbnumlock=1-kbnumlock; break;
        case PRESSED_LSHIFT: kbshift=1; break;
        case RELEASE_LSHIFT: kbshift=0; break;
        case PRESSED_RSHIFT: kbshift=1; break;
        case RELEASE_RSHIFT: kbshift=0; break;
        case PRESSED_ALT: kbalt=1; break;
        case RELEASE_ALT: kbalt=0; break;
        case PRESSED_CTRL: kbctrl=1; break;
        case RELEASE_CTRL: kbctrl=0; break;
        case PRESSED_F1: 
            if(kbalt) terminal_idx=0; 
            change_curr_terminal(terminal_idx);
            break;
        case PRESSED_F2: 
            if(kbalt) terminal_idx=1; 
            change_curr_terminal(terminal_idx);
            break;
        case PRESSED_F3: 
            if(kbalt) terminal_idx=2; 
            change_curr_terminal(terminal_idx);
            break;
        case PRESSED_F4: 
            if(!terminal[terminal_idx].read_flag)
                    printf("    "); // print 4 spaces directly if terminal is not reading
                else{
                    if(terminal[terminal_idx].buf_cnt>=124) break; // if terminal[terminal_idx].buf_cnt+4 exceed the threshold of the buffer, we reject the tab
                    for(; tab_count>0; tab_count--){
                        if(terminal[terminal_idx].curr_cur == terminal[terminal_idx].buf_cnt){
                            terminal[terminal_idx].keyboard_buf[terminal[terminal_idx].curr_cur++] = ' ';
                            terminal[terminal_idx].buf_cnt++;
                        }
                        else{
                            line_shift(1);
                            terminal[terminal_idx].keyboard_buf[terminal[terminal_idx].curr_cur - 1] = ' ';
                        }
                        putc(' ');
                    }
                }
            break;

        case PRESSED_U: 
            up_history();
            break;
        case PRESSED_D: 
            down_history();
            break;
        case PRESSED_L: 
            buffer_shift(-1);
            break;
        case PRESSED_R: 
            buffer_shift(1);
            break;
        case 0xC7: 
            pid = get_pid();
            restore_video_mapping(pid);
            send_eoi(KEYBOARD_IRQ);
            disable_keyboard();
            sti();
            play_canon();
            return;
        case 0xC9: 
            pid = get_pid();
            restore_video_mapping(pid);
            send_eoi(KEYBOARD_IRQ);
            disable_keyboard();
            sti();
            play_rick_roll();
            return;    
        
        default: 
            if(scancode>SCANCODE_SET_SIZE){
                if(kbnumlock) shut_it_up();
                break;
            } // invalid scancode

            if(kbshift&&kbcapslock) // cases for both shift and capslock are pressed
            {   
                if(alphabet_check(scancode)) // if it's an alphabet, print the lower case
                    {
                        asccode=scan_code_set[scancode];
                    }
                    else // if it's not an alphabet, print the upper case
                    {
                        asccode=scan_code_set_all_upper[scancode];
                    }
            }
            else if(kbshift) // cases for only shift is pressed
                {
                    asccode=scan_code_set_all_upper[scancode];
                }
            else if(kbcapslock) // cases for only capslock is pressed
                {
                if(alphabet_check(scancode)) // if it's an alphabet, print the upper case
                    {
                        asccode=scan_code_set_all_upper[scancode];
                    }
                    else // if it's not an alphabet, print the lower case
                    {
                        asccode=scan_code_set[scancode];
                    }
                }
            else 
            {
                asccode=scan_code_set[scancode];
            }
            if(kbnumlock){
                switch (asccode)
                {
                	case 'z':	case 'Z':	make_some_noise(C3); 	break;
					case 's':	case 'S':	make_some_noise(CS3);	break;
					case 'x':	case 'X':	make_some_noise(D3);	break;
					case 'd':	case 'D':	make_some_noise(DS3);	break;
					case 'c':	case 'C':	make_some_noise(E3);	break;
					case 'v':	case 'V':	make_some_noise(F3);	break;
					case 'g':	case 'G':	make_some_noise(FS3);	break;
					case 'b':	case 'B':	make_some_noise(G3);	break;
					case 'h':	case 'H':	make_some_noise(GS3);	break;
					case 'n':	case 'N':	make_some_noise(A3);	break;
					case 'j':	case 'J':	make_some_noise(AS3);	break;
					case 'm':	case 'M':	make_some_noise(B3);	break;

					case 'q':	case 'Q':	make_some_noise(C4);	break;
					case '2':				make_some_noise(CS4);	break;
					case 'w':	case 'W':	make_some_noise(D4);	break;
					case '3':				make_some_noise(DS4);	break;
					case 'e':	case 'E':	make_some_noise(E4);	break;
					case 'r':	case 'R':	make_some_noise(F4);	break;
					case '5':				make_some_noise(FS4);	break;
					case 't':	case 'T':	make_some_noise(G4);	break;
					case '6':				make_some_noise(GS4);	break;
					case 'y':	case 'Y':	make_some_noise(A4);	break;
					case '7':				make_some_noise(AS4);	break;
					case 'u':	case 'U':	make_some_noise(B4);	break;
                    default:                shut_it_up();   break;
                }
                break;
            }
            if(asccode=='\t'){ // handle tab (4 spaces) and for buffer
                tab_func(terminal[terminal_idx].tab_indic);
                terminal[terminal_idx].tab_indic =1 ;
                break;
            }
            else
                terminal[terminal_idx].tab_indic=0; 
                // last_modify_length = 0;
            if(((asccode=='c')||(asccode=='C')) && kbctrl){ //clean the screen when pressing ctrl+C
                pid_of_term = scheduler_queue[terminal_idx];
                get_pcb_by_pid(pid_of_term)->signals[SIG_INTERRUPT] = 1;
                break;    
            }
            if(((asccode=='l')||(asccode=='L')) && kbctrl){ //clean the screen when pressing ctrl+L
                clear();
                break;    
            }
            else if(asccode=='\b'){ // use the helper function for backspace and handle for buffer
                if(terminal[terminal_idx].buf_cnt==0) break;
                // no condition for no read flag
                if(terminal[terminal_idx].buf_cnt != terminal[terminal_idx].curr_cur && terminal[terminal_idx].read_flag){
                    line_shift(-1);
                    break;
                }
                screen_backspace();
                if(terminal[terminal_idx].buf_cnt>0 && terminal[terminal_idx].read_flag){
                    terminal[terminal_idx].curr_cur--;
                    terminal[terminal_idx].keyboard_buf[--terminal[terminal_idx].buf_cnt]=0;
                }
                break;
            }
            // not modify yet
            // else if(asccode=='\t'){ // handle tab (4 spaces) and for buffer
            //     tab_func();
            //     break;
            // }
            if((terminal[terminal_idx].buf_cnt>=BUF_SIZE-1)&&(asccode!='\n'))
                break; // if the buffer count exceeds 127

            if(terminal[terminal_idx].read_flag){ //if the terminal is reading, get the things into the buffer
                if(terminal[terminal_idx].curr_cur != terminal[terminal_idx].buf_cnt){
                    if(asccode == '\n'){
                        recover_shift();
                        terminal[terminal_idx].keyboard_buf[terminal[terminal_idx].buf_cnt++]=asccode;
                        putc(asccode);
                        break;
                    }
                    line_shift(1);
                    terminal[terminal_idx].keyboard_buf[terminal[terminal_idx].curr_cur - 1] = asccode;
                }
                else{
                    terminal[terminal_idx].keyboard_buf[terminal[terminal_idx].curr_cur++]=asccode;
                    terminal[terminal_idx].buf_cnt++;
                }
            }
            putc(asccode);
    }
    pid = get_pid();
    restore_video_mapping(pid);
    send_eoi(KEYBOARD_IRQ);
    disable_keyboard();
    sti();
}

/* 
 * alphabet_: helper for checking whether we're printing a alphabet or not (use for shift/capslock cases)  
 * Input: scan code
 * Output: 1 if it's a alphabet, 0 otherwise
 * Side effect: tell whether we're printing a alphabet or not
*/
int alphabet_check(uint8_t scancode){
    int retval=0;
    if((scancode>=0x10)&&(scancode<=0x19)) retval=1; // check whether the character is an alphabet, 0x10-0x19: 'q','w','e','r','t','y','u','i','o','p'
    if((scancode>=0x1E)&&(scancode<=0x26)) retval=1; // in order to get the correct behaviour of, 0x1E-0x26: 'a','s','d','f','g','h','j','k','l'
    if((scancode>=0x2C)&&(scancode<=0x32)) retval=1; // pressing shift/ capslock/ shift and capslock: 'z','x','c','v','b','n','m'
    return retval;
}











