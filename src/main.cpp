#include <ch32v20x.h>
#include <stdint.h>
#include "timer.h"
#include "controls.h"
#include "dac.h"
#include "sine.h"
#include "adc.h"
#include "encoders/fsk.h"

#include "usb/usb_serial.h"
USBSerial Serial;

constexpr uint32_t sampleBufferSize = 50000;
uint8_t sampleBuffer[sampleBufferSize];

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
					delayMs(1500);
					const int timeout = 10000; // 10 seconds
					int decodedDataSize = 0;
					bool packetFound = decodePacket(encodeFreq, sampleBuffer, sampleBufferSize, decodedDataSize, timeout);
					stop();
					delayMs(800);
					decodedDataSize++; //packetFound indicator
					Serial.write((uint8_t*)&decodedDataSize, 4);
					Serial.write((uint8_t)(packetFound ? 1 : 0));
					Serial.write(sampleBuffer, decodedDataSize);
					Serial.flush();
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
					encodePacket(encodeFreq, sampleBuffer, fileLength);
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
