/*
 * i2c.c
 *
 *  Created on: 1. 5. 2025
 *      Author: vojtech
 */

#include "i2c_ctrl.h"

static i2c_master_handle_t i2c_handle;
static volatile bool i2c_transfer_completed_flag = false;
static volatile status_t i2c_transfer_status = kStatus_Success;

static void I2C_callback(I2C_Type *base, i2c_master_handle_t *handler, status_t status, void *user_data)
{
	if(status == kStatus_Success)
		i2c_transfer_completed_flag = true;
}

void I2C_init(void)
{
	i2c_master_config_t master_config;
	I2C_MasterGetDefaultConfig(&master_config);
	I2C_MasterInit(I2C0, &master_config, I2C_CLK);

	I2C_MasterTransferCreateHandle(I2C0, &i2c_handle, I2C_callback, NULL);
}

bool I2C_read(uint8_t address, uint8_t reg, uint8_t reg_size, uint8_t *data, size_t data_size)
{
	i2c_master_transfer_t transfer = {0};

	transfer.slaveAddress = address;
	transfer.direction = kI2C_Read;
	transfer.subaddress = reg;
	transfer.subaddressSize = reg_size;
	transfer.data = data;
	transfer.dataSize = data_size;
	transfer.flags = kI2C_TransferDefaultFlag;

	i2c_transfer_completed_flag = false;
	status_t status = I2C_MasterTransferNonBlocking(I2C0, &i2c_handle, &transfer);// != kStatus_Success)
		//return false;

	if(status != kStatus_Success)
		return false;
	uint32_t timeout = 10000;
	while(!i2c_transfer_completed_flag && timeout--)
		if(timeout == 0)
			return false;

	return (i2c_transfer_status == kStatus_Success);
}

bool I2C_write(uint8_t address, uint8_t reg, uint8_t reg_size, uint8_t *data, size_t data_size)
{
	i2c_master_transfer_t transfer = {0};
	transfer.slaveAddress = address;
	transfer.direction = kI2C_Write;
	transfer.subaddress = reg;
	transfer.subaddressSize = reg_size;
	transfer.data = data;
	transfer.dataSize = data_size;
	transfer.flags = kI2C_TransferDefaultFlag;

	i2c_transfer_completed_flag = false;
	if(I2C_MasterTransferNonBlocking(I2C0, &i2c_handle, &transfer) != kStatus_Success)
		return false;

	uint32_t timeout = 10000;
	while(!i2c_transfer_completed_flag && timeout--)
		if(timeout == 0)
			return false;

	return (i2c_transfer_status == kStatus_Success);
}

bool I2C_write_read(uint8_t address, uint8_t cmd, uint8_t *data, size_t data_size)
{
	i2c_master_transfer_t transfer;
	memset(&transfer, 0, sizeof(transfer));
	transfer.slaveAddress = address;
	transfer.direction = kI2C_Write;
	transfer.subaddress = 0;
	transfer.subaddressSize = 0;
	transfer.data = &cmd;
	transfer.dataSize = 1;
	transfer.flags = kI2C_TransferNoStopFlag;

	i2c_transfer_completed_flag = false;
	if(I2C_MasterTransferNonBlocking(I2C0, &i2c_handle, &transfer) != kStatus_Success)
		return false;
	while(!i2c_transfer_completed_flag);

	transfer.direction = kI2C_Read;
	transfer.data = data;
	transfer.dataSize = data_size;
	transfer.flags = kI2C_TransferRepeatedStartFlag;

	i2c_transfer_completed_flag = false;
	if(I2C_MasterTransferNonBlocking(I2C0, &i2c_handle, &transfer) != kStatus_Success)
		return false;
	while(!i2c_transfer_completed_flag);

	return true;
}
