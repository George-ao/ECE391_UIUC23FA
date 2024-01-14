
 #include "lib.h"
 #include "PIT.h"
 #include "speaker.h"
 #include "rtc.h"
 
/* make_some_noice: play the sound with given frequency
 *   INPUTS: f -- the given frequency
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: make the sound with given freq
 */
 // this code is inspired from https://wiki.osdev.org/PC_Speaker#Setting_It_Up
void make_some_noise(uint32_t f) {
 	uint32_t sound_Div;
 	uint32_t port_tmp;
 	sound_Div = 1193180 / f;     //Set the PIT to the desired frequency
 	outb(0xb6, 0x43);
 	outb((uint8_t) (sound_Div), 0x42);
 	outb( (uint8_t) (sound_Div >> 8), 0x42);
 	port_tmp = inb(0x61); // use the PC speaker here to play the sound
  	if (port_tmp != (port_tmp | 3)) {
 		outb( port_tmp | 3, 0x61);
 	}
 }
 
/* shut_it_up: stop the sound
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: stop the PC speaker
 */
void shut_it_up() {
 	uint8_t port_tmp = inb(0x61) & 0xFC;
 	outb(port_tmp, 0x61);
 }
 
/* beep: make a beep
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: let the PC speaker get a beep out
 */
 void beep() {
 	make_some_noise(1000);
 	//sleep(10);
 	shut_it_up();
    //set_PIT_2(old_frequency);
 }

/* play_note: play a certain note
 *   INPUTS: int frequency, int 0
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: let the PC speaker play a certain note
 */
void play_note(uint32_t frequency) {
    make_some_noise(frequency);
    rtc_read(0, 0, 0);
    //sleep(0 * 100);  // usleep takes time in microseconds
    shut_it_up();
    // make_some_noise(1193180);
    
}

/* play_canon: play canon in D - Pachelbel
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: play canon
 */
void play_canon() {
    // Canon in D - Pachelbel
    rtc_open(0);
    int32_t freq = 8;
    rtc_write(0, &freq, 1);
    play_note(G4);  // 5_u
    play_note(G4);  // 5_u
    play_note(E4);  // 3_u
    play_note(F4); // 4_u
    play_note(G4);  // 5_u
    play_note(G4);  // 5_u
    play_note(E4);  // 3_u
    play_note(F4); // 4_u
    play_note(G4);  // 5_u
    play_note(G3);  // 5
    play_note(A3);  // 6
    play_note(B3);  // 7
    play_note(C4);  // 1_u
    play_note(D4);  // 2_u
    play_note(E4);  // 3_u
    play_note(F4); // 4_u
    play_note(E4); // 3_u
    play_note(E4); // 3_u
    play_note(C4); // 1_u
    play_note(D4); // 2_u
    play_note(E4); // 3_u
    play_note(E4); // 3_u
    play_note(G3); // 3
    play_note(A3); // 4
    play_note(B3); // 5
    play_note(C4); // 6
    play_note(B3); // 5
    play_note(A3); // 4
    play_note(B3); // 5
    play_note(G3); // 3
    play_note(A3); // 4
    play_note(B3); // 5
    rtc_close(0);
}

/* play_rick_roll: play never gonna give you up
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: play never gonna give you up
 */
void play_rick_roll() {
    // Verse 1
    rtc_open(0);
    int32_t freq = 8;
    rtc_write(0, &freq, 1);
    play_note(G3);   // 5
    play_note(A3);   // 6
    play_note(C4);   // 1_u
    play_note(A3);   // 6
    play_note(E4);   // 3_u
    play_note(E4);   // 3_u
    play_note(E4);   // 3_u
    play_note(E4);   // 3_u
    play_note(E4);   // 3_u
    play_note(E4);   // 3_u
    play_note(D4);   // 2_u
    play_note(D4);   // 2_u
    rtc_read(0, 0, 0);
    rtc_read(0, 0, 0);
    play_note(G3);   // 5
    play_note(A3);   // 6
    play_note(C4);   // 1_u
    play_note(A3);   // 6
    play_note(D4);   // 2_u
    play_note(D4);   // 2_u
    play_note(D4);   // 2_u
    play_note(D4);   // 2_u
    play_note(D4);   // 2_u
    play_note(D4);   // 2_u
    play_note(C4);   // 1_u
    play_note(B3);   // 7
    play_note(A3);   // 6
    rtc_read(0, 0, 0);
    play_note(G3);   // 5
    play_note(A3);   // 6
    play_note(C4);   // 1_u
    play_note(A3);   // 6
    play_note(C4);   // 1_u
    play_note(C4);   // 1_u
    play_note(C4);   // 1_u
    play_note(C4);   // 1_u
    play_note(D4);   // 2_u
    play_note(D4);   // 2_u
    play_note(B3);   // 7
    play_note(B3);   // 7
    play_note(A3);   // 6
    play_note(A3);   // 6
    play_note(G3);   // 5
    play_note(G3);   // 5
    rtc_read(0, 0, 0);
    play_note(G3);   // 5
    play_note(G3);   // 5
    play_note(G3);   // 5
    play_note(G3);   // 5
    play_note(D4);   // 2_u
    play_note(D4);   // 2_u
    play_note(C4);   // 1_u
    play_note(C4);   // 1_u

    rtc_close(0);
}
