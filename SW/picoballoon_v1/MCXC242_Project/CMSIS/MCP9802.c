/*
 * MCP9802.c
 *
 *  Created on: 23. 3. 2025
 *      Author: vojtech
 */
#include "MCP9802.h"

static i2c_master_handle_t g_i2c_handle;
static volatile bool g_i2c_transfer_completed_flag = false;
static volatile status_t g_i2c_transfer_status = kStatus_Success;

static void MCP9802_callback(I2C_Type *base, i2c_master_handle_t *handler,
							 status_t status, void *user_data)
{
	if(status == kStatus_Success)
	{
		g_i2c_transfer_completed_flag = true;
	}
}

static bool MCP9802_write(uint8_t reg, uint8_t *data, size_t data_size)
{
	i2c_master_transfer_t transfer = {0};
	transfer.slaveAddress = MCP9802_ADDRESS;
	transfer.direction = kI2C_Write;
	transfer.subaddress = reg;
	transfer.subaddressSize = 1;
	transfer.data = data;
	transfer.dataSize = data_size;
	transfer.flags = kI2C_TransferDefaultFlag;

	g_i2c_transfer_completed_flag = false;

	if(I2C_MasterTransferNonBlocking(MCP9802_I2C, &g_i2c_handle, &transfer)
			!= kStatus_Success)
	{
		return false;
	}
	while(!g_i2c_transfer_completed_flag);

	return (g_i2c_transfer_status == kStatus_Success);
}

static bool MCP9802_read(uint8_t reg, uint8_t *data, size_t data_size)
{
	i2c_master_transfer_t transfer = {0};

	transfer.slaveAddress = MCP9802_ADDRESS;
	transfer.direction = kI2C_Read;
	transfer.subaddress = reg;
	transfer.subaddressSize = 1;
	transfer.data = data;
	transfer.dataSize = data_size;
	transfer.flags = kI2C_TransferDefaultFlag;

	g_i2c_transfer_completed_flag = false;
	if(I2C_MasterTransferNonBlocking(MCP9802_I2C, &g_i2c_handle, &transfer)
			!= kStatus_Success)
	{
		return false;
	}
	while(!g_i2c_transfer_completed_flag);

	return (g_i2c_transfer_status == kStatus_Success);
}

void MCP9802_init(void)
{
	i2c_master_config_t masterConfig;
	I2C_MasterGetDefaultConfig(&masterConfig);
	uint32_t sourceClock = I2C_MASTER_CLK_FREQ;
	I2C_MasterInit(MCP9802_I2C, &masterConfig, sourceClock);

	I2C_MasterTransferCreateHandle(MCP9802_I2C, &g_i2c_handle, MCP9802_callback,
								   NULL);

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

	if(!MCP9802_read(MCP9802_REG_TEMP, data, 2))
		return false;

	if((data[0] >> 7) == 1)
		negative = -1;

	decimal = (float)(data[1] >> 7) * 0.5 + (float)((data[1] >> 6) & 0x01) * 0.25;
	*temperature = negative * ((float)(data[0] & 0x7F) + decimal);

	return true;
}

bool MCP9802_set_config(uint8_t config)
{
	return MCP9802_write(MCP9802_REG_CONFIG, &config, 1);
}

bool MCP9802_read_config(uint8_t *config)
{
	return MCP9802_read(MCP9802_REG_CONFIG, config, 1);
}

