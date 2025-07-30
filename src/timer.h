#pragma once
#include <stdint.h>
#include <core_riscv.h>

/*
void __attribute__((interrupt("WCH-Interrupt-fast"))) SysTick_Handler(void)
{
    time++;
    SysTick->SR=0;
    return;
}*/

void initDelayTimer()
{
	SysTick->SR=0;
	SysTick->CNT=0;
	SysTick->CMP=0x8000000000000000LL;//0x100;
	SysTick->CTLR= 
		1 | // enable timer
		0/*2*/ | // enable interrupt
		4 | // select clock source (HCLK) 0 = AHB/8, 1 = AHB
		0/*8*/; // free running mode
}

inline uint64_t getTime()
{
	return SysTick->CNT;
}

inline uint64_t ms2ticks(uint64_t ms)
{
	return ms * 144000;
}

inline uint64_t us2ticks(uint64_t us)
{
	return us * 144;
}

inline void delayTicks(uint64_t ticks)
{
	const uint64_t start = getTime();
	while (getTime() - start < ticks) 
	{
	};
}

inline void delayUs(uint64_t us)
{
	delayTicks(us2ticks(us));
}

inline void delayMs(uint64_t ms)
{
	delayTicks(ms2ticks(ms));
}