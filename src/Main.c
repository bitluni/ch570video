#include "CH57x_common.h"
#include "palPwmDma.h"

int main()
{
	HSECFG_Capacitance(HSECap_18p);
    SetSysClock(CLK_SOURCE_HSE_PLL_100MHz);

	initVideo();
	while(1)
		updateVideo();
	return 0;
}