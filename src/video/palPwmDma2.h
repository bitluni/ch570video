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


volatile int lineShown = -1;
volatile int lineDrawn = 0;

__HIGH_CODE
void drawNextLine(int loop)
{
    while(loop)
    {
        int vi = lineDrawn & 7;
        while(lineShown == vi);
        uint32_t * line = vram[vi];
        switch(lineDrawn)
        {
        case 0:
        case 1:
        case 2:
        case 6:
        case 7: //short short
            {

                int i = 0;
                for(int j = 0; j < pixelsSync; j++)
                    line[i++] = levelSync;
                for(int j = 0; j < (pixelsPerLine >> 1) - pixelsSync; j++)
                    line[i++] = levelBlack;
                for(int j = 0; j < pixelsSync; j++)
                    line[i++] = levelSync;
                for(int j = 0; j < (pixelsPerLine >> 1) - pixelsSync - pixelsReload; j++)
                    line[i++] = levelBlack;/**/
            }
            break;
        case 3:
        case 4: //long long
            {
                int i = 0;
                for(int j = 0; j < (pixelsPerLine >> 1) - pixelsSync; j++)
                    line[i++] = levelSync;
                for(int j = 0; j < pixelsSync; j++)
                    line[i++] = levelBlack;
                for(int j = 0; j < (pixelsPerLine >> 1) - pixelsSync; j++)
                    line[i++] = levelSync;
                for(int j = 0; j < pixelsSync - pixelsReload; j++)
                    line[i++] = levelBlack;
            }
            break;
        case 5: //long short
            {
                int i = 0;
                for(int j = 0; j < (pixelsPerLine >> 1) - pixelsSync; j++)
                    line[i++] = levelSync;
                for(int j = 0; j < pixelsSync; j++)
                    line[i++] = levelBlack;
                for(int j = 0; j < pixelsSync; j++)
                    line[i++] = levelSync;
                for(int j = 0; j < (pixelsPerLine >> 1) - pixelsSync - pixelsReload; j++)
                    line[i++] = levelBlack;
            }
            break;
        case 8:
            {   //first line sync
                int i = 0;
                for(int j = 0; j < pixelsSync; j++)
                    line[i++] = levelSync;
                for(int j = 0; j < pixelsFront; j++)
                    line[i++] = levelBlack;
                i+= xres;
                for(int j = 0; j < pixelsBack - pixelsReload; j++)
                    line[i++] = levelBlack;
                for(int x = 0; x < xres; x++)
                    vram[vi][x + pixelsSync + pixelsFront] = levelBlack;
            }
        default:
            {
            }
            break;
        }
        lineDrawn++;
        if(lineDrawn == 312)
            lineDrawn = 0;
    }
}

const char *text = "bitluni was here!  ";

void initVideo()
{
    GPIOA_ModeCfg(GPIO_Pin_7, GPIO_ModeOut_PP_5mA);

    TMR_PWMCycleCfg(32);

    TMR_PWMInit(High_Level, 0);
    for(int i = 0; i < 8; i++)
        drawNextLine(0);
    lineShown = 0;
    TMR_DMACfg(ENABLE, (uint32_t)vram[lineShown], (uint32_t)vram[lineShown] + 800 - pixelsReload * 4, Mode_Single);
    TMR_PWMEnable();
    TMR_Enable();

    TMR_ClearITFlag(TMR_IT_DMA_END);
    TMR_ITCfg(ENABLE, TMR_IT_DMA_END);
    PFIC_EnableIRQ(TMR_IRQn);
    drawNextLine(1);
}

void updateVideo()
{
    drawNextLine(0);
}

/*********************************************************************
 * @fn      TMR_IRQHandler
 *
 * @brief   TMR�жϺ���
 *
 * @return  none
 */
__INTERRUPT
__HIGH_CODE
void TMR_IRQHandler(void) // TMR0
{
    lineShown = (lineShown + 1) & 7;
    uint32_t beg = (uint32_t)vram[lineShown] & 0xffff;
    R16_TMR_DMA_BEG = beg;
    R16_TMR_DMA_END = beg + 800 - (pixelsReload << 2);

    TMR_PWMInit(High_Level, 0);
    //TMR_DMACfg(ENABLE, (uint32_t)&videoLine[0], (uint32_t)&videoLine[pixelsPerLine], Mode_Single);
    TMR_PWMEnable();
    TMR_Enable();
    if(TMR_GetITFlag(TMR_IT_CYC_END))
    {
        TMR_ClearITFlag(TMR_IT_CYC_END);
        TMR_ITCfg(DISABLE, TMR_IT_DMA_END);
    }
}
