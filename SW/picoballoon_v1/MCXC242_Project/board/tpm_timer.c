/*
 * tpm_timer.c
 *
 *  Created on: 17. 4. 2025
 *      Author: vojtech
 */
#include "tpm_timer.h"

static volatile uint32_t g_milliseconds = 0;
static bool g_timer_running = false;

static tpm_config_t tpm2_config;
static volatile bool g_delay_complete = false;

void TPM0_IRQHandler(void)
{
	TPM_ClearStatusFlags(TIMER_BASE, kTPM_TimeOverflowFlag);
	if(g_timer_running)
	{
		g_milliseconds++;
	}
	__DSB();
}

void TPM2_IRQHandler(void)
{
	TPM_ClearStatusFlags(DELAY_TIMER_BASE, kTPM_TimeOverflowFlag);
	g_delay_complete = true;
	__DSB();
}

void tpm_timer_init(void)
{
	tpm_config_t tpm_config;
	TPM_GetDefaultConfig(&tpm_config);
	tpm_config.prescale = TIMER_PRESCALER;
	TPM_Init(TIMER_BASE, &tpm_config);
	TPM_SetTimerPeriod(TIMER_BASE, USEC_TO_COUNT(TIMER_PERIOD_US, TIMER_SOURCE_CLOCK / (1U << tpm_config.prescale)));

	TPM_EnableInterrupts(TIMER_BASE, kTPM_TimeOverflowInterruptEnable);
	EnableIRQ(TPM0_IRQn);

	TPM_StopTimer(TIMER_BASE);
	g_milliseconds = 0;
	g_timer_running = false;
}

void delay_us_init(void)
{
	TPM_GetDefaultConfig(&tpm2_config);
	tpm2_config.prescale = DELAY_TIMER_PRESCALER;
	TPM_Init(DELAY_TIMER_BASE, &tpm2_config);
	//TPM_SetTimerPeriod(TIMER_BASE, USEC_TO_COUNT(DELAY_TIMER_PERIOD_US, DELAY_TIMER_SOURCE_CLOCK / (1U << tpm2_config.prescale)));

	TPM_EnableInterrupts(DELAY_TIMER_BASE, kTPM_TimeOverflowInterruptEnable);
	EnableIRQ(TPM2_IRQn);

	TPM_StopTimer(DELAY_TIMER_BASE);
	g_delay_complete = false;
}

void delay_us(uint16_t us)
{
	if(us == 0)
		return;

	DELAY_TIMER_BASE->CNT = 0;
	TPM_SetTimerPeriod(DELAY_TIMER_BASE, USEC_TO_COUNT(us, DELAY_TIMER_SOURCE_CLOCK / (1U << tpm2_config.prescale)));

	g_delay_complete = false;
	TPM_StartTimer(DELAY_TIMER_BASE, kTPM_SystemClock);
	while(!g_delay_complete);
	TPM_StopTimer(DELAY_TIMER_BASE);
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

