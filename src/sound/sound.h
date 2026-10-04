#ifndef SOUND_H
#define SOUND_H

// CH1 (sweep square): jump, gate, hit, denied
// CH2 (square):       collect, pause
// CH4 (noise):        hit crunch

void sound_play_jump(void);
void sound_play_collect(void);
void sound_play_gate(void);
void sound_play_hit(void);
void sound_play_pause(void);
void sound_play_denied(void);

#endif // SOUND_H
