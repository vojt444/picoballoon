/*
 * htu21d.h
 *
 *  Created on: 26. 4. 2025
 *      Author: vojtech
 */

#ifndef HTU21D_H_
#define HTU21D_H_

#include "clock_config.h"
#include "fsl_gpio.h"
#include "utils.h"
#include "i2c_ctrl.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define HTU21D_I2C_ADDR				0x40

#define HTU21D_USER_REG_RESOLUTION_MASK 0x81

#define HTU21D_CMD_TEMP_HOLD      0xE3
#define HTU21D_CMD_HUMID_HOLD     0xE5
#define HTU21D_CMD_TEMP_NOHOLD    0xF3
#define HTU21D_CMD_HUMID_NOHOLD   0xF5
#define HTU21D_CMD_WRITE_REG      0xE6
#define HTU21D_CMD_READ_REG       0xE7
#define HTU21D_CMD_SOFT_RESET     0xFE

#define HTU21D_RES_RH12_TEMP14    0x00
#define HTU21D_RES_RH8_TEMP12     0x01
#define HTU21D_RES_RH10_TEMP13    0x80
#define HTU21D_RES_RH11_TEMP11    0x81

#define HTU21D_MEAS_TIME_RH12     29
#define HTU21D_MEAS_TIME_RH11     15
#define HTU21D_MEAS_TIME_RH10     9
#define HTU21D_MEAS_TIME_RH8      4
#define HTU21D_MEAS_TIME_TEMP14   50
#define HTU21D_MEAS_TIME_TEMP13   25
#define HTU21D_MEAS_TIME_TEMP12   13
#define HTU21D_MEAS_TIME_TEMP11   7

bool HTU21D_init(void);
bool HTU21D_get_humidity(float *humidity);

#ifdef __cplusplus
}
#endif

#endif /* HTU21D_H_ */
