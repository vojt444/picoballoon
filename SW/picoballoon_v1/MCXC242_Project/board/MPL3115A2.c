/*
 * MPL3115A2.c
 *
 *  Created on: 10. 4. 2025
 *      Author: vojtech
 */

#include "MPL3115A2.h"

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

void MPL_init(void)
{

}


