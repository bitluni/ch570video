#pragma once
#include "CH57x_common.h"
#include "font.h"

const int xres = 150;
const int pixelsSync = 16;
const int pixelsFront = 24;
const int pixelsBack = 10;
const int pixelsPerLine = 200;
const int pixelsReload = 0;

const int levelSync = 0;
const int levelBlack = 8;
const int levelGrey = 20;
const int levelWhite = 32;

__attribute__((aligned(4))) uint32_t vram[11][200 - 0];
volatile uint16_t frameLines[312];

volatile int lineShown = -1;
volatile int lineDrawn = 0;
volatile int currentLine = 0;

void initVideo()
{
    GPIOA_ModeCfg(GPIO_Pin_7, GPIO_ModeOut_PP_20mA);
    TMR_PWMInit(High_Level, PWM_Times_1);
    TMR_PWMCycleCfg(16);
    TMR_PWMActDataWidth(1);
    TMR_PWMEnable();
    TMR_Enable();
}

__HIGH_CODE
void updateVideo()
{
    static int i = 0;
    while(1)
    {
        R32_TMR_FIFO = i;
        i++;
        i&=15;
    }
}
