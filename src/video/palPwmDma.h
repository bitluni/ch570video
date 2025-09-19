#pragma once
#include "CH57x_common.h"
#include "font.h"

const int xres = 150;
const int pixelsSync = 16;
const int pixelsShortSync = 8;
const int pixelsFront = 24;
const int pixelsBack = 10;
const int pixelsPerLine = 200;
const int pixelsReload = 9;

const int levelSync = 0;
const int levelBlack = 8;
const int levelGrey = 20;
const int levelWhite = 32;

__attribute__((aligned(4))) uint32_t vram[2][400 - 0];
__attribute__((aligned(4))) uint32_t syncVram[2][400 - 0];
volatile uint16_t frameLines[312];

void setLineSync(uint32_t *line)
{
    int i = 0;
    for(int j = 0; j < pixelsSync; j++)
        line[i++] = levelSync;
    for(int j = 0; j < pixelsFront; j++)
        line[i++] = levelBlack;
    for(int j = 0; j < xres; j++)
        line[i++] = levelBlack;
    for(int j = 0; j < pixelsBack - pixelsReload; j++)
        line[i++] = levelBlack;
}

void setLineShortShort(uint32_t *line)
{
    int i = 0;
    for(int j = 0; j < pixelsShortSync; j++)
        line[i++] = levelSync;
    for(int j = 0; j < (pixelsPerLine >> 1) - pixelsShortSync; j++)
        line[i++] = levelBlack;
    for(int j = 0; j < pixelsShortSync; j++)
        line[i++] = levelSync;
    for(int j = 0; j < (pixelsPerLine >> 1) - pixelsShortSync - pixelsReload; j++)
        line[i++] = levelBlack;/**/
}

void setLineLongShort(uint32_t *line)
{
    int i = 0;
    for(int j = 0; j < (pixelsPerLine >> 1) - pixelsShortSync; j++)
        line[i++] = levelSync;
    for(int j = 0; j < pixelsShortSync; j++)
        line[i++] = levelBlack;
    for(int j = 0; j < pixelsShortSync; j++)
        line[i++] = levelSync;
    for(int j = 0; j < (pixelsPerLine >> 1) - pixelsShortSync - pixelsReload; j++)
        line[i++] = levelBlack;
}

void setLineLongLong(uint32_t *line)
{
    int i = 0;
    for(int j = 0; j < (pixelsPerLine >> 1) - pixelsShortSync; j++)
        line[i++] = levelSync;
    for(int j = 0; j < pixelsShortSync; j++)
        line[i++] = levelBlack;
    for(int j = 0; j < (pixelsPerLine >> 1) - pixelsShortSync; j++)
        line[i++] = levelSync;
    for(int j = 0; j < pixelsShortSync - pixelsReload; j++)
        line[i++] = levelBlack;
}

volatile int currentLine = 0;
volatile uint8_t textBuffer[16][16];

const char startText[16][16] = {
    "00              ",
    "01              ",
    "02              ",
    "03   bitluni    ",
    "04              ",
    "05     was      ",
    "06              ",
    "07    here!     ",
    "08              ",
    "09              ",
    "10              ",
    "11              ",
    "12              ",
    "13              ",
    "14              ",
    "15              ",
    };

void initVideo()
{
    GPIOA_ModeCfg(GPIO_Pin_7, GPIO_ModeOut_PP_5mA);

    TMR_PWMCycleCfg(32);

    TMR_PWMInit(High_Level, 0);

    setLineLongLong(syncVram[0]);
    setLineShortShort(syncVram[1]);

    for(int r = 0; r < 16; r++)
        for(int c = 0; c < 16; c++)
            textBuffer[r][c] = startText[r][c] - 32;

    for(int y = 0; y < 2; y++)
    {
        setLineSync(vram[y]);
    }

    int i = 0;
    for(; i < 304; i++)
        frameLines[i + 8] = (uint32_t)vram[((i >> 1) & 1)];
    frameLines[i++] = (uint32_t)syncVram[1];
    frameLines[i++] = (uint32_t)syncVram[1];
    frameLines[i++] = (uint32_t)syncVram[1];
    frameLines[i++] = (uint32_t)syncVram[0];
    frameLines[i++] = (uint32_t)syncVram[0];
    frameLines[i++] = (uint32_t)syncVram[0] + 400;    //long short
    frameLines[i++] = (uint32_t)syncVram[1];
    frameLines[i++] = (uint32_t)syncVram[1];

    SysTick->CTLR = 0;
    SysTick->CNT = 0;
    SysTick->CMP = 6400;
    SysTick->SR = 0;
    TMR_DMACfg(ENABLE, (uint32_t)frameLines[0], (uint32_t)frameLines[0] + 800 - pixelsReload * 4, Mode_Single);
    TMR_PWMEnable();
    TMR_Enable();
    //               enable     no int     clk/1      reload
    SysTick->CTLR = (1 << 0) | (0 << 1) | (1 << 2) | (1 << 3);

    TMR_ClearITFlag(TMR_IT_DMA_END);
    TMR_ITCfg(ENABLE, TMR_IT_DMA_END);
    PFIC_EnableIRQ(TMR_IRQn);
}


volatile uint32_t counter = 0;
char *hex = "0123456789ABCDEF";
__HIGH_CODE
void updateVideo()
{
    for(int i = 0; i < 8; i++)
        textBuffer[10][12 - i] = hex[((counter >> (i * 4)) & 15)] - 32;
    return;
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
    uint32_t beg = frameLines[currentLine];
    //fix sync
    while(!SysTick->SR);
    R16_TMR_DMA_BEG = beg;
    R16_TMR_DMA_END = beg + 800 - (pixelsReload << 2);
    TMR_PWMInit(High_Level, 0);
    TMR_PWMEnable();
    TMR_Enable();
    SysTick->SR = 0;

    if(TMR_GetITFlag(TMR_IT_CYC_END))
    {
        TMR_ClearITFlag(TMR_IT_CYC_END);
        TMR_ITCfg(DISABLE, TMR_IT_DMA_END);
    }

    if(currentLine & 1)
    {
        if(currentLine < 256)
        {
            int renderLine = ((currentLine >> 1) + 3) & 127;
            int r = renderLine >> 3;
            int y = renderLine & 7;
            uint32_t *p = &(vram[y & 1][pixelsSync + pixelsFront]);
            {
                for(int x = 0; x < 8 * 16; x++)
                {
                    int ch = textBuffer[r][x >> 3];
                    int bit = (x + 0) & 7;
                    if((font8x8[ch][y] >> bit) & 1)
                        p[x] = levelGrey;
                    else
                        p[x] = levelBlack;// + (x & 7);
                }
            }
        }
    }

    currentLine++;
    if(currentLine == 312)
    {
        currentLine = 0;
        counter++;
    }
}
