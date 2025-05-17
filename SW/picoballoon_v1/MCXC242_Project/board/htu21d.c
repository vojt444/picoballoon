/*
 * htu21d.c
 *
 *  Created on: 26. 4. 2025
 *      Author: vojtech
 */
#include "htu21d.h"

static bool HTU21D_soft_reset(void);
static bool HTU21D_set_resolution(uint8_t resolution);
static uint8_t HTU21D_calc_crc8(uint8_t *data, uint8_t data_len);

bool HTU21D_init(void)
{
	if(!HTU21D_soft_reset())
		return false;

	return true;
}

static bool HTU21D_soft_reset(void)
{
	uint8_t cmd = HTU21D_CMD_SOFT_RESET;

	if(!I2C_write(HTU21D_I2C_ADDR, 0, 0, &cmd, 1))
		return false;

	delay_ms(20);

	return true;
}

static bool HTU21D_set_resolution(uint8_t resolution)
{
	if(resolution != HTU21D_RES_RH12_TEMP14 && resolution != HTU21D_RES_RH8_TEMP12
			&& resolution != HTU21D_RES_RH10_TEMP13 && resolution != HTU21D_RES_RH11_TEMP11)
		return false;

	uint8_t reg_value;
	if(!I2C_write_read(HTU21D_I2C_ADDR, HTU21D_CMD_READ_REG, &reg_value, 1))
		return false;

	reg_value &= ~HTU21D_USER_REG_RESOLUTION_MASK;
	reg_value |= (resolution & HTU21D_USER_REG_RESOLUTION_MASK);

	uint8_t buffer[2] = {HTU21D_CMD_WRITE_REG, reg_value};
	if(!I2C_write(HTU21D_I2C_ADDR, 0, 0, buffer, sizeof(buffer)))
		return false;

	return true;
}

static uint8_t HTU21D_calc_crc8(uint8_t *data, uint8_t data_len)
{
	uint8_t crc = 0;

	for(uint8_t i = 0; i < data_len; i++)
	{
		crc ^= data[i];
		for(uint8_t j = 0; j < 8; j++)
		{
			if(crc & 0x80)
				crc = (uint8_t)((crc << 1) ^ 0x31);
			else
				crc <<= 1;
		}
	}

	return crc;
}

bool HTU21D_get_humidity(float *humidity)
{
	if(humidity == NULL)
		return false;

	uint8_t data[3] = {0};
	if(!I2C_write_read(HTU21D_I2C_ADDR, HTU21D_CMD_HUMID_HOLD, data, 3))
		return false;

	uint8_t calc_crc = HTU21D_calc_crc8(data, 2);

	if(data[2] != calc_crc)
		return false;

	uint16_t raw_value = ((uint16_t)data[0] << 8) | (uint16_t)data[1];
	raw_value = raw_value & 0xFFFC;

	float rh = -6.0f + 125.0f * ((float)raw_value / 65536.0f);

	if(rh < 0.0f)
		rh = 0.0f;
	if(rh > 100.0f)
		rh = 100.0f;

	*humidity = rh;

	return true;
}
