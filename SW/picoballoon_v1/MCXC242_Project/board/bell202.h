/*
 * bell202.h
 *
 *  Created on: 14. 5. 2025
 *      Author: vojtech
 */

#ifndef BELL202_H_
#define BELL202_H_

#include "clock_config.h"
#include "fsl_gpio.h"
#include "utils.h"
#include "tpm_timer.h"
#include "si4461.h"

//#define BELL202_GPIO_PIN			29U
//#define BELL202_GPIO_PORT			GPIOE

#define BELL202_GPIO_PIN			10U
#define BELL202_GPIO_PORT			GPIOC

#define BELL202_MARK_TIMER_PERIOD   417 // half period for 1200Hz tone [us] - log. 1
#define BELL202_SPACE_TIMER_PERIOD 	227  // half period for 2200Hz tone [us] - log. 0

#define BELL202_TIMER_BASE         	TPM1
#define BELL202_TIMER_SOURCE_CLOCK 	(CLOCK_GetFreq(kCLOCK_McgIrc48MClk))
#define BELL202_TIMER_PRESCALER    	TPM_CalculateCounterClkDiv(BELL202_TIMER_BASE, 1000000U / BELL202_TIMER_PERIOD_US, BELL202_TIMER_SOURCE_CLOCK);
#define BELL202_TIMER_PERIOD_US 	1U

#ifdef __cplusplus
extern "C"
{
#endif

void bell202_init_timer(void);
void bell202_start_tones(void);
void bell202_stop_tones(void);
void bell202_generate_tone(bool is_space, uint32_t duration_ms);
void bell202_change_tone(bool is_space);
void bell202_send_bit(bool bit, uint32_t duration_ms);
void bell202_send_byte(uint8_t byte, uint32_t duration_ms);

#ifdef __cplusplus
}
#endif

#endif /* BELL202_H_ */
