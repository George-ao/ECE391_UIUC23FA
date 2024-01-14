#ifndef SPEAKER_H
#define SPEAKER_H

#define C2 65
#define CS2 69
#define D2 73
#define DS2 78
#define E2 82
#define F2 87
#define FS2 92
#define G2 98
#define GS2 104
#define A2 110
#define AS2 117
#define B2 123

#define C3 131
#define CS3 139
#define D3 147
#define DS3 156
#define E3 165
#define F3 175
#define FS3 185
#define G3 196
#define GS3 208
#define A3 220
#define AS3 233
#define B3 247

#define C4 262
#define CS4 277
#define D4 294
#define DS4 311
#define E4 330
#define F4 349
#define FS4 370
#define G4 392
#define GS4 415
#define A4 440
#define AS4 466
#define B4 494

void make_some_noise(uint32_t f);
void shut_it_up();
extern void beep();
void play_note(uint32_t frequency);
extern void play_canon();
extern void play_rick_roll();

#endif
