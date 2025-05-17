/*
 * tpm_timer.h
 *
 *  Created on: 17. 4. 2025
 *      Author: vojtech
 */
#include "fsl_clock.h"
#include "fsl_tpm.h"

#ifndef TPM_TIMER_H_
#define TPM_TIMER_H_

#define TIMER_BASE 					TPM0
#define TIMER_SOURCE_CLOCK 			(CLOCK_GetFreq(kCLOCK_McgIrc48MClk) / 4)
#define TIMER_PRESCALER 			TPM_CalculateCounterClkDiv(TIMER_BASE, 1000000U / TIMER_PERIOD_US, TIMER_SOURCE_CLOCK);
#define TIMER_PERIOD_US 			1000U

#define DELAY_TIMER_BASE         	TPM2
#define DELAY_TIMER_SOURCE_CLOCK 	(CLOCK_GetFreq(kCLOCK_McgIrc48MClk))
#define DELAY_TIMER_PRESCALER    	TPM_CalculateCounterClkDiv(DELAY_TIMER_BASE, 1000000U / DELAY_TIMER_PERIOD_US, DELAY_TIMER_SOURCE_CLOCK);
#define DELAY_TIMER_PERIOD_US 		1U


#ifdef __cplusplus
extern "C"
{
#endif

void tpm_timer_init(void);
uint32_t get_time_ms(void);
void tpm_timer_start(void);
void tpm_timer_stop(void);

void delay_us_init(void);
void delay_us(uint16_t us);

#ifdef __cplusplus
}
#endif

#endif /* TPM_TIMER_H_ */
