/*
 * i2c.h
 *
 *  Created on: 1. 5. 2025
 *      Author: vojtech
 */

#ifndef I2C_CTRL_H_
#define I2C_CTRL_H_

#include "board.h"
#include "utils.h"
#include "fsl_i2c.h"

#define I2C_CLK 		CLOCK_GetFreq(I2C0_CLK_SRC)

#ifdef __cplusplus
extern "C"
{
#endif

void I2C_init(void);
bool I2C_read(uint8_t address, uint8_t reg, uint8_t reg_size, uint8_t *data, size_t data_size);
bool I2C_write(uint8_t address, uint8_t reg, uint8_t reg_size, uint8_t *data, size_t data_size);
bool I2C_write_read(uint8_t address, uint8_t cmd, uint8_t *data, size_t data_size);

#ifdef __cplusplus
}
#endif

#endif /* I2C_CTRL_H_ */
