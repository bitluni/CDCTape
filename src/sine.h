#pragma once
#include <math.h>
#include <stdint.h>
#include "timer.h"

int8_t sinTab[256];

void initSine()
{
	for(int i = 0; i < 256; i++)
	{
		sinTab[i] = (int8_t)(sin(i * M_PI / 128) * 127);
	}
}

inline int32_t sin(uint32_t freq, uint64_t t)
{
	//uint32_t phase = (t * 256 * freq) / 144000000LL;
	uint64_t phase = (t * 7635LL * freq) >> 32;
	return sinTab[phase & 0xff];
}

inline int32_t cos(uint32_t freq, uint64_t t)
{
	//uint32_t phase = (t * 256 * freq) / 144000000LL;
	uint64_t phase = ((t * 7635LL * freq) >> 32) + 64;
	return sinTab[(phase + 64) & 0xff];
}

inline uint32_t tone(int freq, int min = 0x0, int max = 0xffff)
{
	return min + (((max - min) * (sin(freq, getTime()) + 127)) >> 8);
}
