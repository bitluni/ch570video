#pragma once
#include "CH57x_common.h"
#include "font.h"

const int rows = 34;
const int cols = 37;

const int xres = 300;
const int yres = rows * 8;
const int pixelsPerLine = 400;
const int pixelsPerLineHalf = pixelsPerLine >> 1;
const int pixelsSync = 32;
const int pixelsSyncShort = 16;
const int pixelsSyncLong = pixelsPerLineHalf - pixelsSyncShort;
const int pixelsFront = 50;
const int pixelsBack = pixelsPerLine - pixelsSync - pixelsFront - xres;
const int pixelsReload = 19;
const int linesTotal = 312;
const int linesBlankFront = 20;
const int linesSync = 8;
const int linesBlankBack = linesTotal - linesSync - linesBlankFront - yres;

const int levelSync = 0;
const int levelBlack = 4;
const int levelGrey = 10;
const int levelWhite = 16;

__attribute__((aligned(4))) uint32_t vram[2][400 - 0];

volatile int currentLine = 0;
volatile uint8_t textBuffer[34][37];

void initVideo()
{
    GPIOA_ModeCfg(GPIO_Pin_7, GPIO_ModeOut_PP_5mA);

    TMR_PWMCycleCfg(16);
    TMR_PWMInit(High_Level, 0);


    for(int r = 0; r < rows; r++)
        for(int c = 0; c < cols; c++)
            //textBuffer[r][c] = startText[r & 15][c & 15] - 32;
//            textBuffer[r][c] = '_'-32;//+ (r % 10) - 32;
            textBuffer[r][c] = '0' + (c % 10) - 32;

    TMR_DMACfg(ENABLE, (uint32_t)vram[0], (uint32_t)vram[1] + 1600, Mode_LOOP); //two lines
    TMR_PWMEnable();
    TMR_Enable();

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

//using pragmas to prevent GCC to replace loops by memcpy that is not in SRAM
#pragma GCC push_options
#pragma GCC optimize ("no-tree-loop-distribute-patterns")
__HIGH_CODE
void syncShortShort(uint32_t *line)
{
    uint32_t *l = line;
    for(int i = 0; i < pixelsSyncShort; i++)
    {
        l[i] = levelSync;
        l[i + pixelsPerLineHalf] = levelSync;
    }
    for(int i = pixelsSyncShort; i < pixelsPerLineHalf; i++)
    {
        l[i] = levelBlack;
        l[i + pixelsPerLineHalf] = levelBlack;
    }
}

__HIGH_CODE
void syncLongLong(uint32_t *line)
{
    uint32_t *l = line;
    for(int i = 0; i < pixelsSyncLong; i++)
    {
        l[i] = levelSync;
        l[i + pixelsPerLineHalf] = levelSync;
    }
    for(int i = pixelsSyncLong; i < pixelsPerLineHalf; i++)
    {
        l[i] = levelBlack;
        l[i + pixelsPerLineHalf] = levelBlack;
    }
}

__HIGH_CODE
void syncLongShort(uint32_t *line)
{
    uint32_t *l = line;
    for(int i = 0; i < pixelsSyncShort; i++)
    {
        l[i + pixelsPerLineHalf] = levelSync;
    }
    for(int i = pixelsSyncShort; i < pixelsPerLineHalf; i++)
    {
        l[i + pixelsPerLineHalf] = levelBlack;
    }
}

__HIGH_CODE
void firstBlank(uint32_t *line)
{
    uint32_t *l = line;
    for(int i = 0; i < pixelsSync; i++)
        l[i] = levelSync;
    for(int i = 0; i < pixelsSyncShort; i++)
        l[i + pixelsPerLineHalf] = levelBlack;
}

#pragma GCC pop_options

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
    TMR_ClearITFlag(TMR_IT_DMA_END);

    for(int b = 0; b < 2; b++)
    {
        currentLine++;
        if(currentLine == linesTotal)
        {
            currentLine = 0;
            counter++;
        }
        uint32_t *line = vram[b ^ 1];
        uint32_t *pixels = &(line[pixelsSync + pixelsFront]);

        if(b == 1)
            while(R16_TMR_DMA_NOW < ((uint32_t)(&vram[0][pixelsPerLine - pixelsBack]) & 0xffff));

        if(currentLine <= 2 || (currentLine == 6) || (currentLine == 7))
            syncShortShort(line);
        else
        if(currentLine <= 4)
            syncLongLong(line);
        else
        if(currentLine == 5)
            syncLongShort(line);
        else
        if(currentLine < 10)
            firstBlank(line);
        else
        if(currentLine >= linesBlankFront + linesSync) //on screen
        {
            int renderLine = currentLine - linesBlankFront - linesSync;
            if(renderLine < rows * 8)
            {
                int r = renderLine >> 3;
                int y = renderLine & 7;
                {
                    for(int x = 0; x < 8 * cols; x++)
                    {
                        int ch = textBuffer[r][x >> 3];
                        int bit = (x + 0) & 7;
                        if((font8x8[ch][y] >> bit) & 1)
                            pixels[x] = levelWhite;
                        else
                            pixels[x] = levelBlack;// + (x & 7);
                    }
                }
            }
            else
            if(renderLine < rows * 8 + 2)
                for(int x = 0; x < xres; x++)
                    pixels[x] = levelBlack;
        }
    }
}
