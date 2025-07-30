#pragma once
#include <ch32v20x.h>
#include "timer.h"

constexpr int buttonRewPin = 6; 
constexpr int buttonPlayPin = 5;
constexpr int buttonStopPin = 4; 
constexpr int buttonRecPin = 3;
constexpr int interfaceDelayMs = 500;

void play()
{
	GPIOA->BSHR = 1 << buttonPlayPin; // set pin
	delayMs(interfaceDelayMs);
	GPIOA->BCR = 1 << buttonPlayPin; // reset pin
}

void stop()
{
	GPIOA->BSHR = 1 << buttonStopPin; // set pin
	delayMs(interfaceDelayMs);
	GPIOA->BCR = 1 << buttonStopPin; // reset pin
}

void rew()
{
	GPIOA->BSHR = 1 << buttonRewPin; // set pin
	delayMs(interfaceDelayMs);
	GPIOA->BCR = 1 << buttonRewPin; // reset pin
}

void ff()
{
	GPIOA->BSHR = 1 << buttonRewPin; // set pin
	delayMs(interfaceDelayMs);
	GPIOA->BCR = 1 << buttonRewPin; // reset pin
	delayMs(interfaceDelayMs);
	GPIOA->BSHR = 1 << buttonRewPin; // set pin
	delayMs(interfaceDelayMs);
	GPIOA->BCR = 1 << buttonRewPin; // reset pin
}

void rec()
{
	GPIOA->BSHR = 1 << buttonRecPin; // set pin
	delayMs(interfaceDelayMs * 2);
	GPIOA->BCR = 1 << buttonRecPin; // reset pin
}

void initControls()
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	GPIO_InitTypeDef  GPIO_InitStructure = {0};
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Pin = 1 << buttonStopPin;
	GPIOA->BCR = 1 << buttonStopPin; 
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin = 1 << buttonPlayPin;
	GPIOA->BCR = 1 << buttonPlayPin; 
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin = 1 << buttonRewPin;
	GPIOA->BCR = 1 << buttonRewPin; 
    GPIO_Init(GPIOA, &GPIO_InitStructure);
	GPIO_InitStructure.GPIO_Pin = 1 << buttonRecPin;
	GPIOA->BCR = 1 << buttonRecPin; 
    GPIO_Init(GPIOA, &GPIO_InitStructure);
}