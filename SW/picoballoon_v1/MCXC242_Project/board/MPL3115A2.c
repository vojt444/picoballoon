/*
 * MPL3115A2.c
 *
 *  Created on: 10. 4. 2025
 *      Author: vojtech
 */

#include "MPL3115A2.h"

static bool MPL3115A2_modify_reg(uint8_t reg, uint8_t mask, uint8_t value);
static uint8_t MPL3115A2_read_ID(void);
static bool MPL3115A2_soft_reset(void);
static bool MPL3115A2_set_oversample_ratio(mpl3115a2_oversample_t oversample);

bool MPL_init(void)
{
	uint8_t id = 0;
	id = MPL3115A2_read_ID();

	if(id != MPL3115A2_WHO_AM_I_VALUE)
		return false;

	if(!MPL3115A2_soft_reset())
		return false;

	if(!MPL3115A2_set_oversample_ratio(kMPL3115A2_OS_128))
		return false;

	return true;
}

bool MPL3115A2_set_standby_mode(void)
{
	return MPL3115A2_modify_reg(MPL3115A2_CTRL_REG1, MPL3115A2_CTRL_REG1_SBYB, 0);
}

bool MPL3115A2_set_active_mode(void)
{
	return MPL3115A2_modify_reg(MPL3115A2_CTRL_REG1, MPL3115A2_CTRL_REG1_SBYB, 1);
}

bool MPL_one_shot_measurement(mpl3115a2_data_t *data, mpl3115a2_mode_t mode)
{
	uint8_t ctrl_reg = 0;
	uint32_t timeout = 1000;

	if(data == NULL)
		return false;

	//make sure device is in standby mode
	if(!I2C_read(MPL_ADDRESS, MPL3115A2_CTRL_REG1, 1, &ctrl_reg, 1))
		return false;

	//clear SBYB to enter standby mode if not already in standby
	if((ctrl_reg & MPL3115A2_CTRL_REG1_SBYB) != 0)
	{
		ctrl_reg &= ~MPL3115A2_CTRL_REG1_SBYB;
		if(!I2C_write(MPL_ADDRESS, MPL3115A2_CTRL_REG1, 1, &ctrl_reg, 1))
			return false;
	}

	//set mode
	if(mode == kMPL3115A2_barometer)
		ctrl_reg &= ~MPL3115A2_CTRL_REG1_ALT;
	else
		ctrl_reg |= MPL3115A2_CTRL_REG1_ALT;

	if(!I2C_write(MPL_ADDRESS, MPL3115A2_CTRL_REG1, 1, &ctrl_reg, 1))
		return false;

	//initiate measurement
	ctrl_reg |= MPL3115A2_CTRL_REG1_OST;
	if(!I2C_write(MPL_ADDRESS, MPL3115A2_CTRL_REG1, 1, &ctrl_reg, 1))
		return false;

	while(timeout-- && (ctrl_reg & MPL3115A2_CTRL_REG1_OST) != 0)
	{
		if(!I2C_read(MPL_ADDRESS, MPL3115A2_CTRL_REG1, 1, &ctrl_reg, 1))
			return false;

		if(timeout == 0)
			return false;
	}

	uint8_t buffer[5];
	for(uint8_t i = 0; i < 5; i++)
	{
		if(!I2C_read(MPL_ADDRESS, MPL3115A2_OUT_P_MSB + i, 1, &buffer[i], 1))
			return false;
	}

	if(mode == kMPL3115A2_barometer)
	{
		int32_t raw_pressure = ((int32_t)buffer[0] << 16) | ((int32_t)buffer[1] << 8) | (buffer[2] & 0xF0);
		data->pressure = (float)raw_pressure / 64.0f;
	}
	else
	{
		int32_t raw_altitude = ((int32_t)buffer[0] << 16) | ((int32_t)buffer[1] << 8) | (buffer[2] & 0xF0);
		data->altitude = (float)raw_altitude / 256.0f;
	}

	data->temperature = 0;
	return true;
}

static bool MPL3115A2_modify_reg(uint8_t reg, uint8_t mask, uint8_t value)
{
	uint8_t reg_value = 0;
	if(!I2C_read(MPL_ADDRESS, reg, 1, &reg_value, 1))
		return false;

	reg_value = (reg_value & ~mask) | (value & mask);

	return I2C_write(MPL_ADDRESS, reg, 1, &reg_value, 1);
}

static uint8_t MPL3115A2_read_ID(void)
{
	uint8_t id = 0;
	if(!I2C_read(MPL_ADDRESS, MPL3115A2_WHO_AM_I, 1, &id, 1))
		id = 0xFF;

	return id;
}

static bool MPL3115A2_soft_reset(void)
{
	uint8_t reg_value = MPL3115A2_CTRL_REG1_RST;
	uint32_t timeout = 10000;

	if(!I2C_write(MPL_ADDRESS, MPL3115A2_CTRL_REG1, 1, &reg_value, 1))
		return false;

	reg_value = 0;
	do
	{
		delay(1);

		if(!I2C_read(MPL_ADDRESS, MPL3115A2_CTRL_REG1, 1, &reg_value, 1))
			return false;

		timeout--;
		if(timeout == 0)
			return false;

	} while(reg_value & MPL3115A2_CTRL_REG1_RST);

	delay(10);
	return true;
}

static bool MPL3115A2_set_oversample_ratio(mpl3115a2_oversample_t oversample)
{
	uint8_t ctrl_reg1 = 0;

	//make sure device is in standby mode
	if(!I2C_read(MPL_ADDRESS, MPL3115A2_CTRL_REG1, 1, &ctrl_reg1, 1))
		return false;

	//set standby mode if needed
	bool was_active = (ctrl_reg1 & MPL3115A2_CTRL_REG1_SBYB) != 0;
	if(was_active)
	{
		if(!MPL3115A2_set_standby_mode())
			return false;
	}

	uint8_t oversample_bits = MPL3115A2_CTRL_REG1_OS(oversample);
	if(!MPL3115A2_modify_reg(MPL3115A2_CTRL_REG1, MPL3115A2_CTRL_REG1_OS_128, oversample_bits))
		return false;

	if(was_active)
		if(!MPL3115A2_set_active_mode())
			return false;

	return true;
}

