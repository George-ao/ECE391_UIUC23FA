yuyi's bug log
# pthread
  - semanttic error: when I push down bottom, the picture goes to bottom of the picture
  - How I solve: I find that I do not set the button_pressed to 0 after I deal with the tux inpujt. So, it will keep going down. It takes me 15 minutes to solve.
  - algorithmic error: When I push insert, home, pageup for a while, the picture will change room all the time, which is differnt from the demo.
  - How I solve: I use another variable last_cmd to store the last command. If the command is the same as the last command and current command is one of the three, I will directly set the button_pressed to 0 and go to next iteration. It takes me 30 minutes to solve.
  
        if(last_tux_cmd_ == tux_cmd_)
        {
          if(tux_cmd_ == CMD_ENTER || tux_cmd_ == CMD_MOVE_LEFT || tux_cmd_ == CMD_MOVE_RIGHT) 
          {
            button_pressed = 0;
            pthread_mutex_unlock(&tux_lock);
            continue;
          }
        }

# tuxctl
  - semantic error: button_state |= b & 0xF;	//CBAS forget to use bitmask
  - How I solve: I use bitmask to get the correct button state. It takes me 30 minutes to solve.
  
        button_state |= b & 0xF;	//CBAS forget to use bitmask 
   
  -  algorithmic error: I do not understand how button infomation is conveyed. wrong way to calculate the button state: it is actually active low.
  -  How I solve: I understand that the button state is active low.  It takes me 30 minutes to solve.
  
            case 0xEF:       return CMD_UP;		//1110 1111
            case 0xDF:   	 return CMD_DOWN;	//1101 1111
            case 0xBF:  	 return CMD_LEFT;	//1011 1111
            case 0x7F:  	 return CMD_RIGHT;	//0111 1111
            case 0xFD:       return CMD_MOVE_LEFT;	//1111 1101
            case 0xFB:        return CMD_ENTER;			//1111 1011
            case 0xF7:        return CMD_MOVE_RIGHT;	//1111 0111
            default:         return CMD_NONE;

  - semantic error: forget to restore previous led state in reset function
  - How I solve: I store the led state in the set_led function to a static variable. When I reset, I just call set_led again with the previous led state. It takes me an hour to solve.
  
        void reset_handler(struct tty_struct* tty)
        {
          tux_init(tty);				
          tux_set_led(tty, led_store);
          return;					
        }


# read_photo
  - semantic error: The screen shows nothing and program terminaets. 
  - How I solve: I use gdb to set breakpoint and find that there is a divide 0 error when I calculate average rgb value. I then use to if to let the 
     divide operation only happen when the count is not 0. It takes me 15 minutes to solve.

			    if(octree_fourth_level[i].count == 0) continue;	//skip the nodes that have no pixels

  - algorithmic error: the color of the picture is wrong. 
  - How I solve: I find out that I do not mofidy the index after sorting the array for the level four nodes are changed! I then use another array to map previous index of the nodes to it current index in the sorted level4 nodes. It takes me 30 minutes to solve.

          match_sort[octree_fourth_level[i].index] = i;	//match previous rgb_12 to current index


  - semantic error: I find the general color of the picture is right but they are tend to be "green". 
  - How I solve: I find that I do not understand the 16 bits well. Actually the most signnificant bit of rgb is of same priority. So I left shift r,b for one bit. It takes me one hour to solve.



# text to image
  - semantic error: status bar seems to print correct text but the image is not correct.
    - I can see the general shape of the status bar and it seems to be correct. However, I can not see the place to type in and it could not show the correct image. I think it has to do with my routine to change the text into image or the rountine in my adventure.c The scrolling seems to work so I think my draw vertical line is correct. 
  - How I solve: 
    - I used character index first. I discorver that I do not undetstand the font data. The first dimention is the charater. So, I should first acquire the index to find the correct pixel. It takes me half an hour to solve it.

                int ascii_ = (int) string[character_index];
                if(font_data[ascii_][column_index] && (0x80 >> row_index))
  - semantic error: but the image is still wrong. They looks like a huge block. I find the proble is that I do not use bit mask correctly! The latter oen is wrong as we only want to check if there is pixel to draw in that place. 
  - How I solve: I should use & instead of && to print the correct pixel. It takes me half an hour to solve the question.

                if(font_data[ascii_][column_index] & (0x80 >> row_index))
                if(font_data[ascii_][column_index] && (0x80 >> row_index))

  - semantic error: I see some weird character in the status bar. Then I find that when I loop through the character in columns, I use bar_height instead of font_height. 
  - How I solce: I change bar height to font_height and it works.

        for(column_index=0; column_index < BAR_HEIGHT;column_index++)
        for(column_index=0; column_index < FONT_HEIGHT;column_index++)

# game_loop
- algothmic error: msg in the middle of status bar and no type command text
- How I solve: I use a memset to initialize the string. Fill the string with 40 empty space, it then could properly perform like the demo. That's because our text in always in the middle like we intend to do in text_to_image function. Therfore, we initialize the string with 40 empty space. So, we could have the same effect as the demo. It takes me one hour so solve.
      
      memset(msg_show, ' ', 40);

# variables
  - algorithmic error: the first image could not move like what I do in demo. 
  - How I solve: I realize that I do not modify the scroll dimensio after I add the status bar. So, I change the scroll y dimention to (200-18=182) and it works. It takes me ten minutes to solve.


        #define SCROLL_Y_DIM    182                /* full image width      */


  - algotirhmic error: When I scroll the first image to the bottom, I find that the bottom line is not correct and it does not show corrcect pixel.
  - How I solve: I find that I do not set the line_compare register well. The value in that register shoul be 2*182-1 as the bottom line is line0. It takes me twenty minutes to solve.


          static unsigned short mode_X_CRTC[NUM_CRTC_REGS] = {
            0x5F00, 0x4F01, 0x5002, 0x8203, 0x5404, 0x8005, 0xBF06, 0x1F07,
            0x0008, 0x0109, 0x000A, 0x000B, 0x050C, 0xA00D, 0x000E, 0x000F,   //mp2.1: 0x4109 -> 0x0109   ;0x000C -> 0x050C; 0x000D->0xA00D
            0x9C10, 0x8E11, 0x8F12, 0x2813, 0x0014, 0x9615, 0xB916, 0xE317,
            0x6B18                                              // mp2.1: 0xFF18 -> 0x6B18
        };



