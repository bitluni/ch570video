#include "CH57x_common.h"
//#include "video/palPwm.h"
#include "video/palPwmDma.h"
//#include "video/palPwmDma2.h"

int main()
{
	HSECFG_Capacitance(HSECap_18p);
    SetSysClock(CLK_SOURCE_HSE_PLL_100MHz);

	initVideo();
	while(1)
		updateVideo();
	return 0;
}
