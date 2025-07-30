#pragma once
#include <ch32v20x.h>

void dacEnable(bool on)
{
	//dac
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	if(on)
	{
		GPIOB->CFGLR = 0x33333333;
		GPIOB->CFGHR = 0x33333333;
	}
	else
	{
		GPIOB->CFGLR = 0x44444444;
		GPIOB->CFGHR = 0x44444444;
	}
}

void dacWrite(uint32_t value)
{
	GPIOB->OUTDR = value; // write to lower byte
}