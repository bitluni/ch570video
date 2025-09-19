#pragma once
#include "CH57x_common.h"
#include "font.h"

const int xres = 150;
const int pixelsSync = 16;
const int pixelsShortSync = 8;
const int pixelsFront = 24;
const int pixelsBack = 10;
const int pixelsPerLine = 200;
const int pixelsReload = 0;

const int levelSync = 0;
const int levelBlack = 8;
const int levelGrey = 20;
const int levelWhite = 32;

__attribute__((aligned(4))) uint32_t vram[10][200 - 0];
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
    "03              ",
    "04              ",
    "05              ",
    "06              ",
    "07              ",
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

    setLineLongLong(vram[8]);
    setLineShortShort(vram[9]);
    //setLineLongShort(vram[10]);

    for(int r = 0; r < 16; r++)
        for(int c = 0; c < 16; c++)
            textBuffer[r][c] = startText[r][c] - 32;

    for(int y = 0; y < 8; y++)
    {
        setLineSync(vram[y]);
        for(int x = 0; x < 128; x++)
        {
            int ch = textBuffer[0][x >> 3];
            int bit = x & 7;
            vram[y][x + pixelsSync + pixelsFront] = levelBlack + (x & 7);
            if((font8x8[ch][y] >> bit) & 1)
                vram[y][x + pixelsSync + pixelsFront] = levelWhite;
        }
    }

    int i = 0;
    for(; i < 304; i++)
        frameLines[i + 8] = (uint32_t)vram[((i >> 1) & 7)];
    frameLines[i++] = (uint32_t)vram[9];
    frameLines[i++] = (uint32_t)vram[9];
    frameLines[i++] = (uint32_t)vram[9];
    frameLines[i++] = (uint32_t)vram[8];
    frameLines[i++] = (uint32_t)vram[8];
    frameLines[i++] = (uint32_t)vram[8] + 400;    //long short
    frameLines[i++] = (uint32_t)vram[9];
    frameLines[i++] = (uint32_t)vram[9];

    TMR_DMACfg(ENABLE, (uint32_t)frameLines[0], (uint32_t)frameLines[0] + 800 - pixelsReload * 4, Mode_Single);
    TMR_PWMEnable();
    TMR_Enable();

    TMR_ClearITFlag(TMR_IT_DMA_END);
    TMR_ITCfg(ENABLE, TMR_IT_DMA_END);
    PFIC_EnableIRQ(TMR_IRQn);
}

__HIGH_CODE
void updateVideo()
{
    return;
    /*int renderLine = 127;
    while(1)
    {
        while(renderLine == (((currentLine >> 1) + 8) & 127));
        int r = renderLine >> 3;
        int y = renderLine & 7;
        //for(int y = 0; y < 8; y++)
        {
            for(int x = 0; x < 8 * 16; x++)
            {
                int ch = textBuffer[r][x >> 3];
                int bit = (x + 0) & 7;
                if((font8x8[ch][y] >> bit) & 1)
                    vram[y][x + pixelsSync + pixelsFront] = levelGrey;
                else
                    vram[y][x + pixelsSync + pixelsFront] = levelBlack;// + (x & 7);
            }
        }
        renderLine = (renderLine + 1) & 127;
    }*/
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
    R16_TMR_DMA_BEG = beg;
    R16_TMR_DMA_END = beg + 800 - (pixelsReload << 2);

    TMR_PWMInit(High_Level, 0);

    TMR_PWMEnable();
    TMR_Enable();
    if(TMR_GetITFlag(TMR_IT_CYC_END))
    {
        TMR_ClearITFlag(TMR_IT_CYC_END);
        TMR_ITCfg(DISABLE, TMR_IT_DMA_END);
    }

    if(currentLine & 1)
    {
        if(currentLine < 256)
        {
            int renderLine = ((currentLine >> 1) + 7) & 127;
            int r = renderLine >> 3;
            int y = renderLine & 7;
            //for(int y = 0; y < 8; y++)
            {
                for(int x = 0; x < 8 * 16; x++)
                {
                    int ch = textBuffer[r][x >> 3];
                    int bit = (x + 0) & 7;
                    if((font8x8[ch][y] >> bit) & 1)
                        vram[y][x + pixelsSync + pixelsFront] = levelGrey;
                    else
                        vram[y][x + pixelsSync + pixelsFront] = levelBlack;// + (x & 7);
                }
            }
        }
    }

    currentLine++;
    if(currentLine == 312) currentLine = 0;
}
