#pragma once 
#include <ch32v20x.h>
#include <stdint.h>
#include "timer.h"
#include "dac.h"
#include "adc.h"
#include <cstddef>

int32_t dacCenter = 0x8000;
int32_t dacAmplitude = 0x1400;

inline void encodeByte(uint64_t &t0, uint64_t ticksPerWave, uint8_t b)
{
	int framedByte = 1 | (int(b) << 1);
	for(int i = 0; i < 10; i++)
	{
		uint64_t t = 0;
		bool bit = (framedByte >> i) & 1;
		do
		{
			t = getTime() - t0;
			/*
			uint32_t p = t / (ticksPerWave >> 2);
			if(bit) p >>= 1;
			dacWrite(dacCenter + (p & 1) * dacAmplitude);*/
			uint32_t p = (t * 256) / (ticksPerWave >> 1);
			if(bit) p >>= 1;
			dacWrite(dacCenter + ((dacAmplitude * sinTab[p & 0xff]) >> 7));

		}while(t < ticksPerWave * 2);
		dacWrite(dacCenter);
		t0 += ticksPerWave * 2;
	}
}

void encodePacket(int freq, uint8_t *data, size_t size)
{
	const uint64_t ticksPerWave = (144000000LL / freq);
	uint64_t t0 = getTime();
	//sync
	dacWrite(dacCenter);
	while(getTime() - t0 < ticksPerWave * 20); //byte of slience

	t0 = getTime();
	encodeByte(t0, ticksPerWave, 0b01010011); // sync byte
	//encode the actual bytes
	for(size_t b = 0; b < size; b++)
		encodeByte(t0, ticksPerWave, data[b]);
}


inline uint32_t nextSample(uint64_t &t)
{
	while (getTime() < t); // wait for next sample time
	return getLastAdc();
}

bool decodeByte(uint8_t& data) {
    uint64_t startOfStartBit;

    while (getEnergy(freq0) > getEnergy(freq1)) {
    }
    while (getEnergy(freq0) < getEnergy(freq1)) {
    }
    startOfStartBit = getTime();

    delayTicks(bitPeriod + (bitPeriod / 2));

    uint8_t byteInProgress = 0;
    for (int i = 0; i < 8; i++) {
        bool bit = getEnergy(freq0) < getEnergy(freq1);
        if (bit) {
            byteInProgress |= (1 << i);
        }
        delayTicks(bitPeriod);
    }

    data = byteInProgress;
    return true;
}

void recordSamples(int freq, uint8_t *buffer, size_t size)
{
	const uint32_t ticksPerWave = 144000000 / freq;
	const uint32_t ticksPerSample = ticksPerWave >> 5;
	size_t bufferPos = 0;
	
	//store history of samples
	uint64_t t = getTime();
	while(bufferPos < size)
	{
		while(getTime() - t < ticksPerSample);
		t += ticksPerSample;
		buffer[bufferPos++] = getLastAdc() >> 4;
	}
}

void decodePacket(uint8_t* buffer, int len) {
    uint8_t sync;

    while (true) {
        if (decodeByte(sync)) {
            if (sync == syncByte) {
                break;
            }
        }
        delayMs(1); 
    }

    for (int i = 0; i < len; i++) {
        if (!decodeByte(buffer[i])) {
            return;
        }
    }
}
