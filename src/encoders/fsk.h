#pragma once 
#include <ch32v20x.h>
#include <stdint.h>
#include "timer.h"
#include "dac.h"
#include "adc.h"
#include <cstddef>

#ifndef max
#define max(a, b) ((a)>(b)?(a):(b))
#endif

#ifndef min
#define min(a, b) ((a)<(b)?(a):(b))
#endif

int32_t dacCenter = 0x8000;
int32_t dacAmplitude = 0x1400;

const uint64_t ticksPerSecond = 144000000LL;
const int syncByteCount = 4;
const uint8_t syncBytes[syncByteCount] = {'S', 'Y', 'N', 'C'};
constexpr int syncPowerTheshold = 100;
constexpr bool startStopBits = false;
constexpr int bitsPerWord = startStopBits ? 10 : 8;
constexpr int wavesPerBit = 1;
constexpr int samplesPerWave = 32;
constexpr int samplesPerBit = wavesPerBit * samplesPerWave;
constexpr int samplesPerWord = bitsPerWord * samplesPerBit;
constexpr int maxCorrectionSamples = 4;
constexpr int wave18 = samplesPerWave / 8;
constexpr int wave14 = samplesPerWave / 4;
constexpr int wave24 = wave14 * 2;
constexpr int wave34 = wave14 * 3;
constexpr int wave44 = wave14 * 4;

constexpr int runningSumSampleCount = samplesPerWord;
constexpr int runningSumSampleCountMask = runningSumSampleCount - 1;
typedef uint8_t adcSampleType;
constexpr adcSampleType centerLevel = 128;
constexpr int silenceLevel = 16;
adcSampleType runningSumSamples[runningSumSampleCount];
int runningSumSamplePos = 0;
int runningSums[bitsPerWord][2];
int runningSumAvgSum = 0;
int runningSumAvg = 0;


inline void encodeByte(uint64_t &t0, uint64_t ticksPerWave, uint8_t b)
{
	int framedByte = b;
	if(startStopBits)
		framedByte = 1 | (int(b) << 1);

	for(int i = 0; i < bitsPerWord; i++)
	{
		uint64_t t = 0;
		bool bit = (framedByte >> i) & 1;
		do
		{
			t = getTime() - t0;
			uint32_t p = (t * 256) / (ticksPerWave >> 1);
			if(bit) p >>= 1;
			dacWrite(dacCenter + ((dacAmplitude * sinTab[p & 0xff]) >> 7));

		}while(t < ticksPerWave * wavesPerBit);
		dacWrite(dacCenter);
		t0 += ticksPerWave * wavesPerBit;
	}
}

void encodePacket(int freq, uint8_t *data, size_t size)
{
	const uint64_t ticksPerWave = (ticksPerSecond / freq);
	uint64_t t0 = getTime();
	//sync
	dacWrite(dacCenter);
	while(getTime() - t0 < ticksPerWave * wavesPerBit * bitsPerWord * 2); //2 bytes of slience

	t0 = getTime();
	for(int i = 0; i < syncByteCount; i++)	
		encodeByte(t0, ticksPerWave, syncBytes[i]); // sync bytes
	//encode the actual bytes
	for(size_t b = 0; b < size; b++)
		encodeByte(t0, ticksPerWave, data[b]);
}

inline uint32_t nextSample(uint64_t &t)
{
	while (getTime() < t); // wait for next sample time
	return getLastAdc();
}

//sampling rate is freq * 32
void recordSamples(int freq, uint8_t *buffer, size_t size)
{
	const uint32_t ticksPerWave = ticksPerSecond / freq;
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

void runningSumInit()
{
	for(int i = 0; i < 8; i++)
	{
		runningSums[i][0] = 0;
		runningSums[i][1] = 0;
	}
	for(int i = 0; i < runningSumSampleCount; i++)
		runningSumSamples[i] = centerLevel;
	runningSumAvgSum = runningSumSampleCount * centerLevel;
	runningSumAvg = centerLevel;
}

inline adcSampleType runningSumSampleAt(int bit, int pos)
{
	return runningSumSamples[(runningSumSamplePos + (bit * 32 + pos)) & runningSumSampleCountMask];
}

inline void runningSumPushSample(adcSampleType sample)
{
	runningSumSamples[runningSumSamplePos] = sample;
	runningSumSamplePos = (runningSumSamplePos + 1) & runningSumSampleCountMask;
}

void maintainRunningSums(adcSampleType newSample)
{
	//maintain avg sum
	runningSumAvgSum -= runningSumSampleAt(0, 0);

	//subtract old samples from the sum
	for(int i = 0; i < bitsPerWord; i++)
	{
		int s0 = 0;
		for(int j = 0; j < wavesPerBit; j++)
		{
			//2 x carrier freq for 0
			runningSums[i][0] += 
				- runningSumSampleAt(i, s0) 			//upper portion of sine
				- runningSumSampleAt(i, s0 + wave24) 	//upper portion of sine 2nd wave
				+ runningSumSampleAt(i, s0 + wave14)	//lower portion of sine
				+ runningSumSampleAt(i, s0 + wave34);	//lower portion of sine 2nd wave
			//1 x carrier freq for 1
			runningSums[i][1] += 
				- runningSumSampleAt(i, s0) 			//upper portion of sine
				+ runningSumSampleAt(i, s0 + wave24);	//lower portion of sine
			s0 += samplesPerWave;
		}
	}
	//push in the new sample
	runningSumPushSample(newSample);

	//maintain avg sum
	runningSumAvgSum += runningSumSampleAt(7, 31);
	runningSumAvg = runningSumAvgSum / samplesPerWord;

	//add new samples to the sum
	for(int i = 0; i < bitsPerWord; i++)
	{
		int s0 = 0;
		for(int j = 0; j < wavesPerBit; j++)
		{
			//2 x carrier freq for 0
			runningSums[i][0] += 
				+ runningSumSampleAt(i, s0 + wave14 - 1)	//upper portion of sine
				+ runningSumSampleAt(i, s0 + wave34 - 1) 	//upper portion of sine 2nd wave
				- runningSumSampleAt(i, s0 + wave24 - 1)	//lower portion of sine
				- runningSumSampleAt(i, s0 + wave44 - 1);	//lower portion of sine 2nd wave
			//1 x carrier freq for 1
			runningSums[i][1] += 
				+ runningSumSampleAt(i, s0 + wave24 - 1) 	//upper portion of sine
				- runningSumSampleAt(i, s0 + wave44 - 1);	//lower portion of sine
			s0 += samplesPerWave;
		}
	}	
}

void runningSumDecode(uint16_t &data, int &minPower)
{
	//calculating data and minimal power	
	minPower = 0x7fffffff;
	data = 0;
	for(int i = 0; i < bitsPerWord; i++)
	{
		if(runningSums[i][1] > runningSums[i][0])
		{
			data |= 1 << i;
			minPower = minPower < runningSums[i][1] ? minPower : runningSums[i][1];
		}
		else
			minPower = minPower < runningSums[i][0] ? minPower : runningSums[i][0];
	}
}

int runningSumTotalLevel()
{
	int power = 0;
	for(int i = 0; i < 8; i++)
	{
		power = max(power, max(
					abs(runningSumSampleAt(i, 0) - runningSumAvg), 
					abs(runningSumSampleAt(i, 4) - runningSumAvg)));
	}
	//for(int i = 0; i < bitsPerWord; i++)
	//	power += runningSums[i][0] + runningSums[i][1];
	return power;
}

bool decodePacket(int freq, uint8_t *decodedBuffer, int decodedBufferSize, int &decodedBytes, int timeout = 10000)
{
	decodedBytes = 0;
	if(!decodedBufferSize) 
		return false;

	const uint32_t ticksPerWave = 144000000 / freq;
	const uint32_t ticksPerSample = ticksPerWave / samplesPerWave;

	constexpr int historySize = maxCorrectionSamples * 2 + 1;
	uint64_t timeoutTicks = ms2ticks(timeout);
	int 	historyPower[historySize]; 
	uint16_t historyData [historySize];
	int syncBytesFound = 0;
	runningSumInit();
	int samplesSinceLastByte = 0;
	__disable_irq();
	bool ret = false;
	int samplesSinceSilence = 0;
	int minSilence = 255;
	uint64_t t0 = getTime();
	uint64_t t = t0;
	while(t - t0 < timeoutTicks || syncBytesFound > 0)
	{
		while(getTime() - t < ticksPerSample);
		t += ticksPerSample;
		adcSampleType adcValue = getLastAdc() >> 4;
	
		maintainRunningSums(adcValue);
		int powerLevel = runningSumTotalLevel();
		if(powerLevel < silenceLevel)
			samplesSinceSilence = 0;
		else
			samplesSinceSilence++;
		uint16_t decodedByte = 0;
		int power = 0;
		runningSumDecode(decodedByte, power);

		//maintain history of power levels to adjust sync if needed
		int maxPower = -100000;
		int maxPowerOffset = 0;
		for(int i = 0; i < historySize; i++)
		{
			//shift history
			if(i < historySize - 1)
			{
				historyPower[i] = historyPower[i + 1];
				historyData [i] = historyData [i + 1];
			}
			else
			{
				historyPower[i] = power;
				historyData [i] = decodedByte;
			}
			//find max power level
			if(maxPower < historyPower[i])
			{
				maxPower = historyPower[i];
				maxPowerOffset = i;
			}
		}

		//find the sync byte first
		if(syncBytesFound == 0)
		{
			//check for sync power first sync byte and predecessing silence
			if(power >= syncPowerTheshold
				&& decodedByte == syncBytes[0] 
				&& samplesSinceSilence < samplesPerWord + samplesPerBit
			)
			{
				//minSilence = 255;
				samplesSinceLastByte = 0;
				syncBytesFound = 1;
				decodedBuffer[decodedBytes++] = decodedByte;
			}
		}
		else
		{
			//read all samples for a byte + correction
			if(samplesSinceLastByte == samplesPerWord + maxCorrectionSamples)
			{
				//byte out of sync
				if(maxPower <= 0) //sync lost?
				{
					//uint16_t dataByte = historyData[maxPowerOffset];
					//decodedBuffer[decodedBytes++] = startStopBits ? ((dataByte >> 1) & 255) : (dataByte & 255);
					//decodedBuffer[decodedBytes++] = minSilence;
					ret = true;
					break;
				}
				//write back best byte
				uint16_t dataByte = historyData[maxPowerOffset];
				decodedBuffer[decodedBytes++] = startStopBits ? ((dataByte >> 1) & 255) : (dataByte & 255);
				//minSilence = min(powerLevel, minSilence);	//testing silence level
				if(syncBytesFound < syncByteCount)
				{
					//check further sync bytes
					if(dataByte != syncBytes[syncBytesFound])
					{
						//snyc bytes do not match
						//skip to next packet
						
						syncBytesFound = 0;
						decodedBytes -= syncBytesFound;
						continue;
					}
					syncBytesFound++;
				}
				samplesSinceLastByte = historySize - 1 - maxPowerOffset;
				if(decodedBytes == decodedBufferSize)
				{
					ret = true;
					break;
				}
			}
			else
				samplesSinceLastByte++;
		}
	}
	__enable_irq();
	//timeout
	return ret;
}