/*
 * sleep.c
 *
 *  Created on: 27. 4. 2025
 *      Author: vojtech
 */

#include "sleep.h"

volatile bool g_lptmr_expired = false;

void LPTMR0_IRQHandler(void)
{
	if(kLPTMR_TimerInterruptEnable & LPTMR_GetEnabledInterrupts(LPTMR_BASE))
	{
		LPTMR_DisableInterrupts(LPTMR_BASE, kLPTMR_TimerInterruptEnable);
		LPTMR_ClearStatusFlags(LPTMR_BASE, kLPTMR_TimerCompareFlag);
		LPTMR_StopTimer(LPTMR_BASE);
		g_lptmr_expired = true;
	}
}

void SLEEP_enter(uint32_t sec)
{
	g_lptmr_expired = false;

	//set lptmr
	LPTMR_SetTimerPeriod(LPTMR_BASE, (uint32_t)(LPO_CLK_FREQ * sec));
	LPTMR_StartTimer(LPTMR_BASE);
	LPTMR_EnableInterrupts(LPTMR_BASE, kLPTMR_TimerInterruptEnable);

	//set sleep mode
	SMC_PreEnterStopModes();
	SMC_SetPowerModeVlps(SMC);
	SMC_PostExitStopModes();
}
