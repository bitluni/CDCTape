#include <ch32v20x.h>
#include <stdint.h>
#include "usb/usb_serial.h"
USBSerial Serial;

#include "timer.h"
#include "controls.h"
#include "dac.h"
#include "sine.h"
#include "adc.h"
#include "encoders/fsk.h"
#include "fs.h"


constexpr uint32_t sampleBufferSize = 51200;
uint8_t sampleBuffer[sampleBufferSize];
constexpr uint16_t maxChunkCount = 100;
uint8_t chunkStates[maxChunkCount];
uint8_t chunkRetries[maxChunkCount];
constexpr int maxRetries = 0;

int dacFreq = 0;
int ampLow = 0x2000;
int ampHigh = 0x3f00;
bool adcEnabled = false;
constexpr int encodeFreq = 1000;

int main(void)
{
	//SetSysClockTo144_HSIfix();
	//SystemCoreClockUpdate();
	initADC();
	initDelayTimer();
	Serial.begin(1000000);
	initSine();
	initControls();
	
	adcEnable(true);
    while(1)
    {
		if(dacFreq > 0)
			dacWrite(tone(dacFreq, ampLow, ampHigh));
		if(adcEnabled)
		{
			if(Serial.availableForWrite() > 0)
			{
				uint8_t s = getLastAdc() >> 4;
				Serial.write(s);
				Serial.flush();
				//Serial.write(i);
			}
		}
		if(Serial.available())
		{
			uint8_t cmd = Serial.read();
			switch(cmd)
			{
				case 1:
					stop();
					break;
				case 2:
					play();
					break;
				case 3:
					rew();
					break;
				case 4:
					ff();
					break;
				case 5:
					rec();
					break;
				case 6:
					{
						rec();
						delayMs(1000);
						dacEnable(true);
						delayMs(1000);
						for(int i = 0; i < 512; i++)
							sampleBuffer[i] = i & 255;
						for(int i = 0; i < 2; i++)
							encodePacket(encodeFreq, sampleBuffer, 512);

						dacEnable(false);
						stop();
						delayMs(1000);
						rew();
					}
					break;
				case 7:
					adcEnabled = !adcEnabled;
					break;
				case 8:
					dacFreq = Serial.read() + (((int)Serial.read()) << 8);
					if(dacFreq)
						dacEnable(true);
					else
						dacEnable(false);
					break;
				case 9:
					play();
					delayMs(800);
					delayMs(1500);
					recordSamples(encodeFreq, sampleBuffer, sampleBufferSize);
					Serial.write(sampleBuffer, sampleBufferSize);
					Serial.flush();
					stop();
					delayMs(500);
					rew();
					break;
				case 10:
				{
					play();
					delayMs(800);
					const int timeout = 10000; // 10 seconds
					int chunkCount = 0;
					int decodedDataSize = 0;
					/*bool packetFound = decodePacket(encodeFreq, sampleBuffer, sampleBufferSize, decodedDataSize, timeout);
					stop();
					delayMs(800);/**/
					for(int i = 0; i < maxChunkCount; i++)
					{
						chunkStates[i] = 0;
						chunkRetries[i] = 0;
					}
					while(1)
					{
						int chunkIndex = 0;
						int newChunkCount = chunkCount;
						int chunkSize = 0;
						int state = 0;
						uint8_t *chunk = loadChunk(encodeFreq, chunkIndex, newChunkCount, chunkSize, state, timeout);
						if(state == 0)
						{
							//no chunks found
							break;
						}
						if(state == 3)
						{
							//invalid chunk
						}
						else
						{
							//set new chunk count
							chunkCount = newChunkCount < maxChunkCount ? newChunkCount : maxChunkCount;
							if(chunkStates[chunkIndex] != 1)
							{
								//copy new chunk
								int chunkOffset = bytesPerChunk * chunkIndex;
								for(int i = 0; i < chunkSize; i++)
									sampleBuffer[chunkOffset + i] = chunk[i];
								if(decodedDataSize < chunkOffset + chunkSize)
									decodedDataSize = chunkOffset + chunkSize;
								chunkStates[chunkIndex] = state; //ok			
							}
							//update chunk states to host
							Serial.write((uint8_t)2);	//chunkStates
							Serial.write((uint8_t)chunkCount);
							for(int i = 0; i < chunkCount; i++)
								Serial.write(chunkStates[i]);/**/
							Serial.flush();	
						}
						int rewindChunks = 0;
						if(state == 2)
						{
							if(chunkRetries[chunkIndex] < maxRetries)
							{
								rewindChunks = 1;
								chunkRetries[chunkIndex]++;
							}
						}
						if(state == 1)
						{
							bool complete = chunkIndex == chunkCount - 1;
							//test if chunk missed
							for(int i = 0; i < chunkIndex; i++)
								if(chunkStates[i] != 1)
								{
									if(chunkRetries[i] < maxRetries)
									{
										chunkRetries[i]++;
										rewindChunks = 1 + chunkIndex - i;
									}
									complete = false;
									break;
								}
							if(complete) break;
						}
						if(rewindChunks)
						{
							stop();
							delayMs(1000);
							rew();
							delayMs(1000);
							delayMs(rewindChunks * rewindMillisPerChunk);
							stop();
							delayMs(1500);
							play();
							delayMs(1000);
						}
					}
					stop();
					if(chunkCount)
					{
						Serial.write((uint8_t)1);	//fileData
						int totalSize = decodedDataSize;// + 1;
						Serial.write((uint8_t*)&totalSize, 4);
						//Serial.write((uint8_t)(packetFound ? 1 : 0));
						Serial.write(sampleBuffer, decodedDataSize);
						Serial.flush();	
					}
					else
					{
						Serial.write((uint8_t)0);
						Serial.flush();
					}
					delayMs(800);
					rew();
					break;
				}
				case 11:
				{
					uint32_t fileLength = 0;
					for(int i = 0; i < 4; i++)
						fileLength |= Serial.read() << (i * 8);
					for(uint32_t i = 0; i < fileLength; i++)
						sampleBuffer[i] = Serial.read();
					rec();
					delayMs(1000);
					dacEnable(true);
					delayMs(1000);
					
					//encodePacket(encodeFreq, sampleBuffer, fileLength);
					storeData(encodeFreq, sampleBuffer, fileLength);

					dacEnable(false);
					stop();
					delayMs(1000);
					rew();
				}
				default:
					break;
			}
		}
    }
}
