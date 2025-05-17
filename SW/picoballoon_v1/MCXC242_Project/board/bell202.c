/*
 * bell202.c
 *
 *  Created on: 14. 5. 2025
 *      Author: vojtech
 */

#include "bell202.h"

static tpm_config_t tpm1_config;
static volatile bool bell202_tone_active = false;

static volatile bool current_tone_is_space = false;
static volatile bool current_pin_state = false;
static volatile uint32_t phase_accumulator = 0;

void TPM1_IRQHandler(void)
{
	TPM_ClearStatusFlags(BELL202_TIMER_BASE, kTPM_TimeOverflowFlag);
	if(bell202_tone_active)
	{
		current_pin_state = !current_pin_state;
		GPIO_PinWrite(BELL202_GPIO_PORT, BELL202_GPIO_PIN, current_pin_state ? 1 : 0);
		phase_accumulator++;
	}
}

void bell202_init_timer()
{
	TPM_GetDefaultConfig(&tpm1_config);
	tpm1_config.prescale = BELL202_TIMER_PRESCALER;
	TPM_Init(BELL202_TIMER_BASE, &tpm1_config);
	TPM_SetTimerPeriod(BELL202_TIMER_BASE, USEC_TO_COUNT(BELL202_MARK_TIMER_PERIOD-1, BELL202_TIMER_SOURCE_CLOCK / (1U << tpm1_config.prescale)));

	TPM_EnableInterrupts(BELL202_TIMER_BASE, kTPM_TimeOverflowInterruptEnable);
	EnableIRQ(TPM1_IRQn);

	TPM_StopTimer(BELL202_TIMER_BASE);
	bell202_tone_active = false;
}

void bell202_start_tones(void)
{
    current_tone_is_space = false;
    current_pin_state = false;
    phase_accumulator = 0;

    GPIO_PinWrite(BELL202_GPIO_PORT, BELL202_GPIO_PIN, 0);

    TPM_SetTimerPeriod(BELL202_TIMER_BASE, USEC_TO_COUNT(BELL202_MARK_TIMER_PERIOD-1, BELL202_TIMER_SOURCE_CLOCK / (1U << tpm1_config.prescale)));
    BELL202_TIMER_BASE->CNT = 0;

    bell202_tone_active = true;
    BELL202_TIMER_BASE->SC |= TPM_SC_CMOD(1);
}

void bell202_stop_tones(void)
{
	bell202_tone_active = false;
	GPIO_PinWrite(GPIOB, BELL202_GPIO_PIN, 0);
	current_pin_state = false;
}

void bell202_generate_tone(bool is_space, uint32_t duration_us)
{
	BELL202_TIMER_BASE->CNT = 0;
	if(is_space)
		TPM_SetTimerPeriod(BELL202_TIMER_BASE, USEC_TO_COUNT(BELL202_MARK_TIMER_PERIOD-1, BELL202_TIMER_SOURCE_CLOCK / (1U << tpm1_config.prescale)));
	else
		TPM_SetTimerPeriod(BELL202_TIMER_BASE, USEC_TO_COUNT(BELL202_SPACE_TIMER_PERIOD-1, BELL202_TIMER_SOURCE_CLOCK / (1U << tpm1_config.prescale)));

	GPIO_PortClear(BELL202_GPIO_PORT, 1 << BELL202_GPIO_PIN);
	TPM_StartTimer(BELL202_TIMER_BASE, kTPM_SystemClock);
	bell202_tone_active = true;
	delay_us(duration_us);
	bell202_tone_active = false;
	TPM_StopTimer(BELL202_TIMER_BASE);
	GPIO_PortClear(BELL202_GPIO_PORT, 1 << BELL202_GPIO_PIN);
}

void bell202_change_tone(bool is_space)
{
	if(current_tone_is_space != is_space)
	{
		uint32_t current_cmod = BELL202_TIMER_BASE->SC & TPM_SC_CMOD_MASK;
		BELL202_TIMER_BASE->SC &= ~TPM_SC_CMOD_MASK;

		float current_phase_position;
		if(current_tone_is_space)
			current_phase_position = (float)BELL202_TIMER_BASE->CNT / BELL202_SPACE_TIMER_PERIOD;
		else
			current_phase_position = (float)BELL202_TIMER_BASE->CNT / BELL202_MARK_TIMER_PERIOD;

		uint32_t new_counter;
		if(is_space)
		{
			new_counter = (uint32_t)(current_phase_position * BELL202_SPACE_TIMER_PERIOD);
			BELL202_TIMER_BASE->MOD = BELL202_SPACE_TIMER_PERIOD - 1;
			TPM_SetTimerPeriod(BELL202_TIMER_BASE, USEC_TO_COUNT(BELL202_SPACE_TIMER_PERIOD - 1, BELL202_TIMER_SOURCE_CLOCK / (1U << tpm1_config.prescale)));
		}
		else
		{
			new_counter = (uint32_t)(current_phase_position * BELL202_MARK_TIMER_PERIOD);
			TPM_SetTimerPeriod(BELL202_TIMER_BASE, USEC_TO_COUNT(BELL202_MARK_TIMER_PERIOD - 1, BELL202_TIMER_SOURCE_CLOCK / (1U << tpm1_config.prescale)));
		}

		BELL202_TIMER_BASE->CNT = new_counter;
		current_tone_is_space = is_space;
		BELL202_TIMER_BASE->SC |= current_cmod;
	}
}

void bell202_send_bit(bool bit, uint32_t duration_us)
{
	if(bit == 0)
		bell202_change_tone(!current_tone_is_space);

	delay_us(duration_us);
}

void bell202_send_byte(uint8_t byte, uint32_t duration_us)
{
	for(int i = 0; i < 8; i++)
	{
		bool bit = (byte >> i) & 0x01;
		bell202_send_bit(bit, duration_us);
	}
}
