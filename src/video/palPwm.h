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
    R16_PWM_CLOCK_DIV = 2;
    R8_PWM_POLAR = 0;
    R8_PWM_CONFIG = 0b110;
    R16_PWM_CYC_VALUE = 64;
    R32_PWM1_3_DATA = 32;
    R8_PWM_OUT_EN = 1;
//     PWMX_16bit_CycleCfg(CH_PWM1, 64);
//    PWMX_16bit_ACTOUT(32);
//    PWMX_AlterOutCfg(CH_PWM1);

}

__HIGH_CODE
void updateVideo()
{
}
