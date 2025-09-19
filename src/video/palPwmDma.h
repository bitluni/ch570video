#pragma once
#include "CH57x_common.h"
#include "font.h"

const int xres = 300;
const int pixelsSync = 32;
const int pixelsShortSync = 32;
const int pixelsFront = 42;
const int pixelsBack = 26;
const int pixelsPerLine = 400;
const int pixelsReload = 19;
const int vBlank = 22;

const int rows = 33;
const int cols = 34;

const int levelSync = 0;
const int levelBlack = 4;
const int levelGrey = 10;
const int levelWhite = 16;

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
volatile uint8_t textBuffer[34][36];

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

    TMR_PWMCycleCfg(16);

    TMR_PWMInit(High_Level, 0);

    setLineLongLong(syncVram[0]);
    setLineShortShort(syncVram[1]);

    for(int r = 0; r < rows; r++)
        for(int c = 0; c < cols; c++)
            textBuffer[r][c] = startText[r & 15][c & 15] - 32;
            //textBuffer[r][c] = '0' + (c % 10) - 32;
    textBuffer[0][0] = ' ' - 32;
    for(int y = 0; y < 2; y++)
    {
        setLineSync(vram[y]);
    }

    int i = 0;
    for(; i < 304 - vBlank; i++)
        frameLines[i] = (uint32_t)vram[i & 1];
    frameLines[i++] = (uint32_t)syncVram[1];
    frameLines[i++] = (uint32_t)syncVram[1];
    frameLines[i++] = (uint32_t)syncVram[1];
    frameLines[i++] = (uint32_t)syncVram[0];
    frameLines[i++] = (uint32_t)syncVram[0];
    frameLines[i++] = (uint32_t)syncVram[0] + 400;    //long short
    frameLines[i++] = (uint32_t)syncVram[1];
    frameLines[i++] = (uint32_t)syncVram[1];
    for(int j = 0; j < vBlank; j++)
        frameLines[i++] = (uint32_t)vram[i & 1];

    SysTick->CTLR = 0;
    SysTick->CNT = 0;
    SysTick->CMP = 6400;
    SysTick->SR = 0;
    TMR_DMACfg(ENABLE, (uint32_t)frameLines[0], (uint32_t)frameLines[0] + 1600 - pixelsReload * 4, Mode_Single);
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
    R16_TMR_DMA_END = beg + 1600 - (pixelsReload << 2);
    TMR_PWMInit(High_Level, 0);
    TMR_PWMEnable();
    TMR_Enable();
    SysTick->SR = 0;

    if(TMR_GetITFlag(TMR_IT_CYC_END))
    {
        TMR_ClearITFlag(TMR_IT_CYC_END);
        TMR_ITCfg(DISABLE, TMR_IT_DMA_END);
    }

    currentLine++;
    if(currentLine == 312)
    {
        currentLine = 0;
        counter++;
    }
    if(currentLine < rows << 3)
    {
        int renderLine = currentLine;
        int r = renderLine >> 3;
        int y = renderLine & 7;
        uint32_t *p = &(vram[y & 1][pixelsSync + pixelsFront]);
        {
            for(int x = 0; x < 8 * cols; x++)
            {
                int ch = textBuffer[r][x >> 3];
                int bit = (x + 0) & 7;
                if((font8x8[ch][y] >> bit) & 1)
                    p[x] = levelWhite;
                else
                    p[x] = levelBlack;// + (x & 7);
            }
        }
    }
    if(currentLine == (rows << 3) || currentLine == (rows << 3) + 1)
    {
        uint32_t *p = &(vram[currentLine & 1][pixelsSync + pixelsFront]);
        for(int x = 0; x < 8 * cols; x++)
            p[x] = levelBlack;
    }
}
