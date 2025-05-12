/*
 * MCP9802.c
 *
 *  Created on: 23. 3. 2025
 *      Author: vojtech
 */
#include "MCP9802.h"

void MCP9802_init(void)
{
	uint8_t config;
	if(MCP9802_read_config(&config))
	{
		config = MCP9802_INIT_CONFIG;
		MCP9802_set_config(config);
	}
}

bool MCP9802_read_temperature_oneshot(float *temperature)
{
	uint8_t data[2] = {0};
	uint8_t negative = 1;
	float decimal = 0;

	if(!MCP9802_set_config(MCP9802_ONESHOT_READ))
		return false;

	if(!I2C_read(MCP9802_ADDRESS, MCP9802_REG_TEMP, 1, data, 2))
		return false;

	if((data[0] >> 7) == 1)
		negative = -1;

	decimal = (float)(data[1] >> 7) * 0.5 + (float)((data[1] >> 6) & 0x01) * 0.25;
	*temperature = negative * ((float)(data[0] & 0x7F) + decimal);

	return true;
}

bool MCP9802_set_config(uint8_t config)
{
	return I2C_write(MCP9802_ADDRESS, MCP9802_REG_CONFIG, 1, &config, 1);
}

bool MCP9802_read_config(uint8_t *config)
{
	return I2C_read(MCP9802_ADDRESS, MCP9802_REG_CONFIG, 1, config, 1);
}

