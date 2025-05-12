/*
 * sleep.h
 *
 *  Created on: 27. 4. 2025
 *      Author: vojtech
 */
#include "fsl_smc.h"
#include "fsl_lptmr.h"

#ifndef SLEEP_H_
#define SLEEP_H_

#define LPTMR_BASE         	LPTMR0
#define LPTMR_IRQn         	LPTMR0_IRQn

#ifdef __cplusplus
extern "C"
{
#endif

void SLEEP_enter(uint32_t sec);

#ifdef __cplusplus
}
#endif

#endif /* SLEEP_H_ */
