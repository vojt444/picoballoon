/*
 * MPL3115A2.h
 *
 *  Created on: 10. 4. 2025
 *      Author: vojtech
 */

#ifndef MPL3115A2_H_
#define MPL3115A2_H_

#include "board.h"
#include "fsl_i2c.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define MPL_ADDRESS					0xC0U
#define MPL_I2C_BASE				I2C0
#define MPL_I2C_MASTER_CLK_FREQ 	CLOCK_GetFreq(I2C0_CLK_SRC)

void MPL_init(void);

#ifdef __cplusplus
}
#endif

#endif /* MPL3115A2_H_ */
