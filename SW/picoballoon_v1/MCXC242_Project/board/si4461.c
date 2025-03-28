/*
 * si4461.c
 *
 *  Created on: 24. 3. 2025
 *      Author: vojtech
 */

#include "si4461.h"

//static uint8_t g_src_buff[SPI_BUFFER_SIZE];
//static uint8_t g_dest_buff[SPI_BUFFER_SIZE];
static spi_master_handle_t g_spi_handle;
static volatile bool g_spi_master_finished_flag = false;
static const uint8_t Si4461_startup_config[] = RADIO_CONFIGURATION_DATA_ARRAY;
static uint32_t timeout = 10000;

void SPI0_IRQHandler(void)
{
	SPI_MasterTransferHandleIRQ(SPI_BASE, &g_spi_handle);
}

static void spi_callback(SPI_Type *base, spi_master_handle_t *masterHandle,
						 status_t status, void *userData)
{
	if(status == kStatus_Success)
	{
		g_spi_master_finished_flag = true;
	}
}

static bool Si4461_SPI_transfer(uint8_t *tx_buff, uint8_t *rx_buff,
								size_t data_len)
{
	spi_transfer_t transfer = {0};

	uint8_t dummy_data[data_len];
	memset(dummy_data, 0, data_len * sizeof(dummy_data));

	transfer.txData = tx_buff;
	transfer.rxData = rx_buff;
	transfer.dataSize = data_len;

	if(SPI_MasterTransferNonBlocking(SPI_BASE, &g_spi_handle, &transfer)
			!= kStatus_Success)
		return false;

	timeout = 10000;
	while(!g_spi_master_finished_flag && timeout--);
	if(timeout == 0)
		return false;

	transfer.txData = dummy_data;
	transfer.rxData = rx_buff;
	transfer.dataSize = data_len;
	if(SPI_MasterTransferNonBlocking(SPI_BASE, &g_spi_handle, &transfer)
			!= kStatus_Success)
		return false;

	timeout = 10000;
	while(!g_spi_master_finished_flag && timeout--);
	if(timeout == 0)
		return false;

	return true;
}

static bool Si4461_SPI_transfer_no_rx(uint8_t *tx_buff, size_t tx_data_len)
{
	spi_transfer_t transfer = {0};

	transfer.txData = tx_buff;
	transfer.dataSize = tx_data_len;

	if(SPI_MasterTransferNonBlocking(SPI_BASE, &g_spi_handle, &transfer)
			!= kStatus_Success)
		return false;

	timeout = 10000;
	while(!g_spi_master_finished_flag && timeout--);
	if(timeout == 0)
		return false;

	return true;
}

static bool Si4461_wait_for_CTS(void)
{
	uint8_t tx[2] = {SI4461_CMD_READ_CMD_BUFF, 0x00};
	uint8_t rx[2] = {0};

	spi_transfer_t transfer = {0};
	transfer.txData = tx;
	transfer.rxData = rx;
	transfer.dataSize = 2;

	GPIO_PortClear(SI4461_CS_GPIO, SI4461_CS_PIN_MASK);
	timeout = 10000;
	while(timeout--)
	{
		if(SPI_MasterTransferNonBlocking(SPI_BASE, &g_spi_handle, &transfer)
				!= kStatus_Success)
			return false;
		if(transfer.rxData[0] == 0xff)
		{
			GPIO_PortSet(SI4461_CS_GPIO, SI4461_CS_PIN_MASK);
			return true;
		}
	}
	GPIO_PortSet(SI4461_CS_GPIO, SI4461_CS_PIN_MASK);
	return false;
}

static bool Si4461_write_command(uint8_t cmd, uint8_t *data, uint8_t data_len)
{
	if(!Si4461_wait_for_CTS())
		return false;

	GPIO_PortClear(SI4461_CS_GPIO, SI4461_CS_PIN_MASK);
	if(!Si4461_SPI_transfer_no_rx(&cmd, 1))
		return false;

	if(!Si4461_SPI_transfer_no_rx(data, data_len))
		return false;
	GPIO_PortSet(SI4461_CS_GPIO, SI4461_CS_PIN_MASK);

	return true;
}

static bool Si4461_set_config(const uint8_t *config, uint32_t config_len)
{
	uint8_t cmd_len;
	uint8_t command;
	uint16_t pos = 0;
	uint8_t buff[32] = {0};

	while(pos < config_len)
	{
		cmd_len = config[pos++] - 1;
		command = config[pos++];
		memcpy(buff, config + pos, cmd_len);

		if(!Si4461_write_command(command, buff, cmd_len))
			return false;
		pos += cmd_len;
	}

	return true;
}

bool Si4461_init(void)
{
	spi_master_config_t spi_config = {0};
	uint32_t source_clock = 0;
	SPI_MasterGetDefaultConfig(&spi_config);
	spi_config.baudRate_Bps = 1000000;
	spi_config.outputMode = kSPI_SlaveSelectAsGpio;
	source_clock = SPI_CLOCK_SOURCE;
	SPI_MasterInit(SPI_BASE, &spi_config, source_clock);

	SPI_MasterTransferCreateHandle(SPI_BASE, &g_spi_handle, spi_callback, NULL);
	NVIC_EnableIRQ(SPI0_IRQn);

	//reset device
	GPIO_PortSet(SI4461_SDN_GPIO, SI4461_SDN_PIN_MASK);
	delay(50);
	GPIO_PortClear(SI4461_SDN_GPIO, SI4461_SDN_PIN_MASK);
	delay(50);

	//apply startup config
	if(!Si4461_set_config(Si4461_startup_config, sizeof(Si4461_startup_config)))
		return false;
	//read pending interrupts and clear them

	//set sleep state
	return true;
}

