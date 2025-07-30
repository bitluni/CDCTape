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

uint8_t sampleBuffer[32000];

int dacFreq = 0;
int ampLow = 0x2000;
int ampHigh = 0x3f00;
bool adcEnabled = false;

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
						for(int i = 0; i < 256; i++)
							sampleBuffer[i] = i;
						for(int i = 0; i < 32; i++)
							encodePacket(1000, sampleBuffer, 32);

						/*uint64_t t0 = getTime();
						while (getTime() - t0 < ms2ticks(5000))
						{
							//uint64_t t = getTime(); 
							dacWrite(tone(1200, 0x8000 - 0x1400, 0x8000 + 0x1400));
						}*/

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
					//decodePacket(500, sampleBuffer, 256);
					//Serial.write(sampleBuffer, 1600);
					recordSamples(1000, sampleBuffer, 32000);
					Serial.write(sampleBuffer, 32000);
					Serial.flush();
					stop();
					delayMs(500);
					rew();
					break;
				default:
					break;
			}
		}
    }
}
