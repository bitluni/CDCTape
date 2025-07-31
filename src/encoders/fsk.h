#pragma once 
#include <ch32v20x.h>
#include <stdint.h>
#include "timer.h"
#include "dac.h"
#include "adc.h"
#include <cstddef>

int32_t dacCenter = 0x8000;
int32_t dacAmplitude = 0x1400;

inline void encodeByte(uint64_t &t0, uint64_t ticksPerWave, uint8_t b, bool startStopBits)
{
	int bitCount = 8;
	int framedByte = b;
	if(startStopBits)
	{
		framedByte = 1 | (int(b) << 1);
		bitCount = 10;
	}
	for(int i = 0; i < bitCount; i++)
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

void encodePacket(int freq, uint8_t *data, size_t size, bool startStopBits = false)
{
	const uint64_t ticksPerWave = (144000000LL / freq);
	uint64_t t0 = getTime();
	//sync
	dacWrite(dacCenter);
	while(getTime() - t0 < ticksPerWave * 20); //byte of slience

	t0 = getTime();
	encodeByte(t0, ticksPerWave, 0b01010011, startStopBits); // sync byte
	//encode the actual bytes
	for(size_t b = 0; b < size; b++)
		encodeByte(t0, ticksPerWave, data[b], false);
}


inline uint32_t nextSample(uint64_t &t)
{
	while (getTime() < t); // wait for next sample time
	return getLastAdc();
}

int decodeByte(uint16_t *samples, int pos, int64_t &pow)
{
	int mask = (32 * 8) - 1;
	uint8_t b = 0;
	pow = 0;
	for(int i = 0; i < 8; i++)
	{
		int p0 = pos + i * 32;
		int pow0 = samples[(p0 + 8) & mask] - samples[pos + 8];
		int pow1 = (samples[(p0 + 4) & mask] - samples[(p0 + 12) & mask] +
					samples[(p0 + 20) & mask] - samples[(p0 + 28) & mask]) >> 1;
		pow0 *= pow0;
		pow1 *= pow1;
		if(pow0 < pow1)
		{
			b |= 1 << i;
			pow += pow1;
		}
		else
			pow += pow0;
	}
	return b;
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

void decodePacket(int freq, uint8_t *buffer, size_t size)
{
	if(!size) return;
	const uint32_t ticksPerWave = 144000000 / freq;
	const uint32_t ticksPerSample = ticksPerWave >> 5;
	size_t bufferPos = 0;
	bool sync = false;
	int64_t syncPow = 0;
	
	//store history of samples
	const int samplesPerByte = 32 * 8;
	uint16_t samples[samplesPerByte];
	int samplePos = 0;
	int samplesCurrentByte = 0;
	for(int i = 0; i < samplesPerByte; i++)
		samples[i] = 0;

	uint64_t t = getTime();
	while(bufferPos < size)
	{
		t += ticksPerSample;
		while(getTime() < t);
		samples[samplePos++] = getLastAdc();
		samplePos = (samplePos + 1) & (samplesPerByte - 1);
		int64_t pow = 0;
		uint8_t decodedByte = decodeByte(samples, (samplePos - samplesPerByte) & (samplesPerByte - 1), pow);
		if(sync)
		{
			samplesCurrentByte++;
			if(samplesCurrentByte == samplesPerByte)
			{
				buffer[bufferPos++] = decodedByte;
				if(bufferPos == size) return;
				samplesCurrentByte = 0;
			}
		}
		else
			if(decodedByte == 0b01010011) // sync byte
			{
				if(pow > syncPow)
				{
					syncPow = pow;
				}
				else
				{
					sync = true;
					samplesCurrentByte = 1;
				}
			}
	}
	// amps = SUM(sample * sin(phase)) / N 
	// ampc = SUM(sample * cos(phase)) / N 
	// amp2 = amps * amps + ampc * ampc
}
