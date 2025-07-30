#pragma once
#include <ch32v20x.h>
#include <stdint.h>
#include "timer.h"
#include "sine.h"

const int carrierFreq[] = {
	523, 587, 659, 740, 831, 932, 1109, 1245, 1397, 1568, 1760, 1976
};

uint8_t encodeByte(uint16_t value)
{
	uint64_t t = getTime();
	//uint64_t phase0 = (t / 14400000) & 1;
	int32_t s = 0;
	for(int i = 0; i < 2; i++)
		if(value & (1 << i))
		{
			uint64_t phase = (t * 256 * carrierFreq[i * 4]) / 144000000LL;
			return (sinTab[phase & 0xff] + 128) >> 3;
		}
	return (s >> 1) + 32;
}

/*
uint8_t encodeByte(uint16_t value)
{
	uint64_t t = getTime();
	//uint64_t phase0 = (t / 14400000) & 1;
	uint8_t s = 0;
	for(int i = 0; i < 9; i++)
	{
		uint64_t phase = (((t * 7635LL * carrierFreq[8 - i]) >> 32) & 0xff) >> 7;
		s += 8 * (phase & ((value >> i) & 1));
	}
	return s;
}*/

uint8_t encodeBit(int freq, uint16_t value)
{
	if(value & 1)
	{
		uint64_t t = getTime();
		uint64_t phase = (t * 256 * freq) / 144000000LL;
		return (sinTab[phase & 0xff] + 128) >> 3;
	}
	else
	{
		return 32;
	}
}