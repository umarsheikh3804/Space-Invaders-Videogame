#ifndef __Sounds_h
#define __Sounds_h
#include <stdint.h>
#include "DAC.h"
typedef struct {
	const unsigned char *wave; 
	uint32_t length; 
} sound_t; 
//typedef enum {Shoot, InvaderKilled, Explosion, FastInvader1, FastInvader2, FastInvader3, HighPitch} soundEffect; 
void playSound(soundEffect sound); 
#endif