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
volatile uint32_t frameLines[312];

/*********************************************************************
 * @fn      DebugInit
 *
 * @brief   ���Գ�ʼ��
 *
 * @return  none
 */
void DebugInit(void)
{
    GPIOA_SetBits(bTXD_0);
    GPIOA_ModeCfg(bRXD_0, GPIO_ModeIN_PU);      // RXD-������������
    GPIOA_ModeCfg(bTXD_0, GPIO_ModeOut_PP_20mA); // TXD-�������������ע������IO������ߵ�ƽ
    UART_DefInit();
}

void setLineSync(uint32_t *line)
{
    int i = 0;
    for(int j = 0; j < pixelsSync; j++)
        line[i++] = levelSync;
    for(int j = 0; j < pixelsFront; j++)
        line[i++] = levelBlack;
    i+= xres;
    for(int j = 0; j < pixelsBack - pixelsReload; j++)
        line[i++] = levelBlack;
}

void setLineShortShort(uint32_t *line)
{
/*    int i = 0;
    for(int j = 0; j < pixelsSync; j++)
        line[i++] = levelSync;
    for(int j = 0; j < pixelsPerLine - pixelsSync; j++)
        line[i++] = levelWhite;/**/

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

void setLineLongShort(uint32_t *line)
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

void setLineLongLong(uint32_t *line)
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

volatile int lineShown = -1;
volatile int lineDrawn = 0;

void drawNextLine()
{
    int vi = lineDrawn & 7;
    while(lineShown == vi);
    switch(lineDrawn)
    {
    case 0:
    case 1:
    case 2:
    case 6:
    case 7:
        setLineShortShort(vram[vi]);
        break;
    case 3:
    case 4:
        setLineLongLong(vram[vi]);
        break;
    case 5:
        setLineLongShort(vram[vi]);
        break;
    default:
        {
            //setLineSync(vram[vi]);
            for(int x = 0; x < pixelsPerLine; x++)
                vram[vi][x] = levelWhite;
//            for(int x = 0; x < xres; x++)
//                vram[vi][x + pixelsSync + pixelsFront] = levelBlack;
        }
        break;
    }
    lineDrawn++;
    if(lineDrawn == 312)
        lineDrawn = 0;
}

volatile int currentLine = 0;

void initVideo()
{
    GPIOA_ModeCfg(GPIO_Pin_7, GPIO_ModeOut_PP_5mA);

    TMR_PWMCycleCfg(32);

    TMR_PWMInit(High_Level, 0);
//    TMR_DMACfg(ENABLE, (uint32_t)&videoLine[0], (uint32_t)&videoLine[pixelsPerLine], Mode_LOOP);

//    for(int i = 0; i < 8; i++)
//        drawNextLine();
    setLineShortShort(vram[0]);
    setLineLongLong(vram[1]);
    setLineLongShort(vram[2]);

    const char *text = "bitluni was here!  ";
    for(int y = 0; y < 8; y++)
    {
        setLineSync(vram[3 + y]);
        for(int x = 0; x < xres; x++)
        {
            int ch = text[(x >> 3)];
            int bit = x & 7;
            vram[3 + y][x + pixelsSync + pixelsFront] = levelBlack + (x & 7);
            if((font8x8[ch][y] >> bit) & 1)
                vram[3 + y][x + pixelsSync + pixelsFront] = levelWhite;
        }
    }

    frameLines[0] = (uint32_t)vram[0] & 0xffff;
    frameLines[1] = (uint32_t)vram[0] & 0xffff;
    frameLines[2] = (uint32_t)vram[0] & 0xffff;
    frameLines[3] = (uint32_t)vram[1] & 0xffff;
    frameLines[4] = (uint32_t)vram[1] & 0xffff;
    frameLines[5] = (uint32_t)vram[2] & 0xffff;
    frameLines[6] = (uint32_t)vram[0] & 0xffff;
    frameLines[7] = (uint32_t)vram[0] & 0xffff;
    for(int i = 8; i < 312; i++)
        frameLines[i] = (uint32_t)vram[3 + ((i >> 1) & 7)] & 0xffff;
    lineShown = 0;
    TMR_DMACfg(ENABLE, (uint32_t)frameLines[0], (uint32_t)frameLines[0] + 800 - pixelsReload * 4, Mode_Single);
//    TMR_DMACfg(ENABLE, (uint32_t)vram[lineShown], (uint32_t)vram[lineShown] + 800 - pixelsReload * 4, Mode_Single);
//    TMR_DMACfg(ENABLE, (uint32_t)vram[0], (uint32_t)vram[0] + 1600, Mode_LOOP);
    TMR_PWMEnable();
    TMR_Enable();

    TMR_ClearITFlag(TMR_IT_DMA_END);
    TMR_ITCfg(ENABLE, TMR_IT_DMA_END);
    PFIC_EnableIRQ(TMR_IRQn);
}

void updateVideo()
{
	/*static int s = 0;
	for(int y = 0; y < 8; y++)
	{
		while(frameLines[currentLine] == vram[3 + y]);
		for(int x = 0; x < xres; x++)
		{
			int ch = text[((x + s) >> 3) & 15 ];
			int bit = (x + s) & 7;
			vram[3 + y][x + pixelsSync + pixelsFront] = levelBlack + (x & 7);
			if((font8x8[ch][y] >> bit) & 1)
				vram[3 + y][x + pixelsSync + pixelsFront] = levelWhite;
		}
	}
	s++;
*/
//  drawNextLine();
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
//    lineShown = (lineShown + 1) & 7;
//    uint32_t beg = (uint32_t)vram[lineShown] & 0xffff;
//    R16_TMR_DMA_BEG = beg;
//    R16_TMR_DMA_END = beg + 800 - (pixelsReload << 2);
    currentLine++;
    if(currentLine == 312) currentLine = 0;
    uint32_t beg = (uint32_t)frameLines[currentLine];
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