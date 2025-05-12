/*
 * tpm_timer.c
 *
 *  Created on: 17. 4. 2025
 *      Author: vojtech
 */
#include "tpm_timer.h"

static volatile uint32_t g_milliseconds = 0;
static bool g_timer_running = false;

void TPM0_IRQHandler(void)
{
	TPM_ClearStatusFlags(TIMER_BASE, kTPM_TimeOverflowFlag);
	if(g_timer_running)
	{
		g_milliseconds++;
	}
	__DSB();
}

void tpm_timer_init(void)
{
	tpm_config_t tpmConfig;
	TPM_GetDefaultConfig(&tpmConfig);
	tpmConfig.prescale = TIMER_PRESCALER;
	TPM_Init(TIMER_BASE, &tpmConfig);
	TPM_SetTimerPeriod(TIMER_BASE, USEC_TO_COUNT(TIMER_PERIOD_US, TIMER_SOURCE_CLOCK / (1U << tpmConfig.prescale)));

	TPM_EnableInterrupts(TIMER_BASE, kTPM_TimeOverflowInterruptEnable);
	EnableIRQ(TPM0_IRQn);

	TPM_StopTimer(TIMER_BASE);
	g_milliseconds = 0;
	g_timer_running = false;
}

uint32_t get_time_ms(void)
{
	if(g_timer_running)
		return g_milliseconds;
	else
		return 0;
}

void tpm_timer_start(void)
{
	if(!g_timer_running)
	{
		g_milliseconds = 0;
		g_timer_running = true;
		TPM_StartTimer(TIMER_BASE, kTPM_SystemClock);
	}
}

void tpm_timer_stop(void)
{
	if(g_timer_running)
	{
		TPM_StopTimer(TIMER_BASE);
		g_timer_running = false;
	}
}

