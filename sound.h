#ifndef SOUND_H
#define SOUND_H

#include <stdint.h>

void Sound_Init(void);

void Sound_On(void);
void Sound_Off(void);

void PlaySound(uint32_t frequency);
void StopSound(void);

void GetScoreSound(void);
void PlayGameOverSound(void);
#endif
