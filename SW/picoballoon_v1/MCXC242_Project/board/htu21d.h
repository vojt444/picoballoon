/*
 * htu21d.h
 *
 *  Created on: 26. 4. 2025
 *      Author: vojtech
 */

#ifndef HTU21D_H_
#define HTU21D_H_

#include "board.h"
#include "utils.h"
#include "i2c_ctrl.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define HTU21D_I2C_ADDR				0x40

#define HTU21D_USER_REG_RESOLUTION_MASK 0x81

#define HTU21D_CMD_TEMP_HOLD      0xE3 /* Trigger temperature measurement with hold master mode */
#define HTU21D_CMD_HUMID_HOLD     0xE5 /* Trigger humidity measurement with hold master mode */
#define HTU21D_CMD_TEMP_NOHOLD    0xF3 /* Trigger temperature measurement with no hold master mode */
#define HTU21D_CMD_HUMID_NOHOLD   0xF5 /* Trigger humidity measurement with no hold master mode */
#define HTU21D_CMD_WRITE_REG      0xE6 /* Write user register */
#define HTU21D_CMD_READ_REG       0xE7 /* Read user register */
#define HTU21D_CMD_SOFT_RESET     0xFE /* Soft reset */

/* Resolution configurations */
#define HTU21D_RES_RH12_TEMP14    0x00 /* RH: 12bit, Temp: 14bit (default) */
#define HTU21D_RES_RH8_TEMP12     0x01 /* RH: 8bit, Temp: 12bit */
#define HTU21D_RES_RH10_TEMP13    0x80 /* RH: 10bit, Temp: 13bit */
#define HTU21D_RES_RH11_TEMP11    0x81 /* RH: 11bit, Temp: 11bit */

/* Measurement times in milliseconds for different resolutions */
#define HTU21D_MEAS_TIME_RH12     29   /* 12bit RH measurement time in ms */
#define HTU21D_MEAS_TIME_RH11     15   /* 11bit RH measurement time in ms */
#define HTU21D_MEAS_TIME_RH10     9    /* 10bit RH measurement time in ms */
#define HTU21D_MEAS_TIME_RH8      4    /* 8bit RH measurement time in ms */
#define HTU21D_MEAS_TIME_TEMP14   50   /* 14bit temperature measurement time in ms */
#define HTU21D_MEAS_TIME_TEMP13   25   /* 13bit temperature measurement time in ms */
#define HTU21D_MEAS_TIME_TEMP12   13   /* 12bit temperature measurement time in ms */
#define HTU21D_MEAS_TIME_TEMP11   7    /* 11bit temperature measurement time in ms */

bool HTU21D_init(void);
bool HTU21D_get_humidity(float *humidity);

#ifdef __cplusplus
}
#endif

#endif /* HTU21D_H_ */
