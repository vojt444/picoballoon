/*
 * si4461.c
 *
 *  Created on: 24. 3. 2025
 *      Author: vojtech
 */

#include "si4461.h"

#define CHIP_SELECT() GPIO_PortClear(SI4461_CS_GPIO, SI4461_CS_PIN_MASK)
#define CHIP_DESELECT() GPIO_PortSet(SI4461_CS_GPIO, SI4461_CS_PIN_MASK)

static spi_master_handle_t g_spi_handle;
static volatile bool g_spi_master_finished_flag = false;
static const uint8_t Si4461_startup_config[] = RADIO_CONFIGURATION_DATA_ARRAY;

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

static bool Si4461_SPI_transfer(uint8_t *tx_buff, size_t tx_len,
								uint8_t *rx_buff, size_t rx_len)
{
	spi_transfer_t transfer = {0};
	uint8_t dummy_rx[tx_len];
	memset(dummy_rx, 0, tx_len);

	transfer.txData = tx_buff;
	transfer.rxData = dummy_rx;
	transfer.dataSize = tx_len;

	g_spi_master_finished_flag = false;
	if(SPI_MasterTransferNonBlocking(SPI_BASE, &g_spi_handle, &transfer)
			!= kStatus_Success)
		return false;

	uint32_t timeout = 10000;
	while(!g_spi_master_finished_flag && timeout--)
	{
		if(timeout == 0)
			return false;
	}

	uint8_t dummy_tx[rx_len];
	memset(dummy_tx, 0, rx_len);
	memset(rx_buff, 0, rx_len);
	transfer.txData = dummy_tx;
	transfer.rxData = rx_buff;
	transfer.dataSize = rx_len;

	g_spi_master_finished_flag = false;
	if(SPI_MasterTransferNonBlocking(SPI_BASE, &g_spi_handle, &transfer)
			!= kStatus_Success)
		return false;

	timeout = 10000;
	while(!g_spi_master_finished_flag && timeout--)
	{
		if(timeout == 0)
			return false;
	}

	return true;
}

static bool Si4461_SPI_transfer_no_tx(uint8_t *rx_buff, size_t rx_len)
{
	spi_transfer_t transfer = {0};
	uint8_t dummy_tx[rx_len];
	memset(dummy_tx, 0x44, rx_len);
	memset(rx_buff, 0, rx_len);

	transfer.txData = dummy_tx;
	transfer.rxData = rx_buff;
	transfer.dataSize = rx_len;

	g_spi_master_finished_flag = false;
	if(SPI_MasterTransferNonBlocking(SPI_BASE, &g_spi_handle, &transfer)
			!= kStatus_Success)
		return false;

	uint32_t timeout = 10000;
	while(!g_spi_master_finished_flag && timeout--)
	{
		if(timeout == 0)
			return false;
	}

	return true;
}

static bool Si4461_SPI_transfer_no_rx(uint8_t *tx_buff, size_t tx_data_len)
{
	spi_transfer_t transfer = {0};
	uint8_t rx[tx_data_len];
	memset(rx, 0, tx_data_len);
	transfer.txData = tx_buff;
	transfer.rxData = rx;
	transfer.dataSize = tx_data_len;

	g_spi_master_finished_flag = false;
	if(SPI_MasterTransferNonBlocking(SPI_BASE, &g_spi_handle, &transfer)
			!= kStatus_Success)
		return false;

	uint32_t timeout = TIMEOUT;
	while(!g_spi_master_finished_flag && timeout--)
	{
		if(timeout == 0)
			return false;
	}

	return true;
}

static bool Si4461_wait_for_CTS(void)
{
	uint8_t tx = SI4461_CMD_READ_CMD_BUFF; // SI4461_CMD_PART_INFO
	uint8_t rx = 0;

	uint16_t timeout = CTS_TIMEOUT;
	while(timeout--)
	{
		CHIP_SELECT();
		if(!Si4461_SPI_transfer(&tx, 1, &rx, 1))
		{
			CHIP_DESELECT();
			return false;
		}
		if(rx == 0xFF)
		{
			CHIP_DESELECT();
			return true;
		}
		CHIP_DESELECT();
	}
	return false;
}

static bool Si4461_write_command(uint8_t cmd, uint8_t *data, size_t data_len)
{
	if(!Si4461_wait_for_CTS())
		return false;

	CHIP_SELECT();
	if(!Si4461_SPI_transfer_no_rx(&cmd, 1))
		return false;
	if(!Si4461_SPI_transfer_no_rx(data, data_len))
		return false;
	CHIP_DESELECT();

	return true;
}

static bool Si4461_read_command(uint8_t command, uint8_t *buff, size_t len)
{
	if(!Si4461_wait_for_CTS())
		return false;

	CHIP_SELECT();
	if(!Si4461_SPI_transfer_no_rx(&command, 1))
	{
		CHIP_DESELECT();
		return false;
	}
	CHIP_DESELECT();

	if(!Si4461_wait_for_CTS())
		return false;

	CHIP_SELECT();
	uint8_t read_cmd = SI4461_CMD_READ_CMD_BUFF;
	if(!Si4461_SPI_transfer_no_rx(&read_cmd, 1))
	{
		CHIP_DESELECT();
		return false;
	}

	if(!Si4461_SPI_transfer_no_tx(buff, len))
	{
		CHIP_DESELECT();
		return false;
	}
	CHIP_DESELECT();
	return true;
}

static bool Si4461_set_config(const uint8_t *config, size_t config_len)
{
	uint8_t cmd_len;
	uint8_t command;
	uint16_t pos = 0;
	uint8_t buff[32] = {0};

	config_len = config_len - 1;
	cmd_len = config[0];
	pos = cmd_len + 1;

	CHIP_SELECT();
	while(pos < config_len)
	{
		cmd_len = config[pos++] - 1;
		command = config[pos++];
		memcpy(buff, config + pos, cmd_len);

		if(!Si4461_write_command(command, buff, cmd_len))
			return false;
		pos += cmd_len;
	}
	CHIP_DESELECT();
	return true;
}

static bool Si4461_interrupts(void)
{
	uint8_t command = SI4461_CMD_GET_INT_STATUS;
	uint8_t data[8] = {0};

	CHIP_SELECT();
	if(!Si4461_SPI_transfer(&command, 1, data, 8))
	{
		CHIP_DESELECT();
		return false;
	}
	CHIP_DESELECT();
	return true;
}

static bool Si4461_set_state(si4461_state_t new_state)
{
	uint8_t data[2] = {SI4461_CMD_CHANGE_STATE, new_state};

	if(!Si4461_SPI_transfer_no_rx(data, 2))
		return false;

	return true;
}

static bool Si4461_NOP(void)
{
	uint8_t response = 0;

	if(!Si4461_read_command(SI4461_CMD_NOP, &response, 1))
		return false;

	return (response == 0xFF);
}

static bool Si4461_set_tx_power(uint8_t power)
{
	if(power > 127)
		power = 127;

	uint8_t buff[4] = {0x20, 0x00, 0x00, 0x1D};
	buff[1] = power;

	return Si4461_set_properties(SI4463_PROPERTY_PA_MODE, sizeof(buff), buff);
}

bool Si4461_set_properties(uint16_t start_property, uint8_t length,
						   uint8_t *params)
{
	if(!Si4461_wait_for_CTS())
		return false;

	uint8_t buff[4];
	buff[0] = SI4461_CMD_SET_PROPERTY;
	buff[1] = start_property >> 8;
	buff[2] = length;
	buff[3] = (uint8_t)(start_property && 0xFF);

	CHIP_SELECT();
	if(!Si4461_SPI_transfer_no_rx(buff, 4))
	{
		CHIP_DESELECT();
		return false;
	}
	if(!Si4461_SPI_transfer_no_rx(params, length))
	{
		CHIP_DESELECT();
		return false;
	}

	CHIP_DESELECT();
	return true;
}

bool Si4461_get_properties(uint16_t start_property, uint8_t length,
						   uint8_t *params)
{
	if(!Si4461_wait_for_CTS())
		return false;

	uint8_t buff[4];
	buff[0] = SI4461_CMD_GET_PROPERTY;
	buff[1] = start_property >> 8;
	buff[2] = length;
	buff[3] = (uint8_t)(start_property && 0xFF);

	CHIP_SELECT();
	if(!Si4461_SPI_transfer_no_rx(buff, 4))
	{
		CHIP_DESELECT();
		return false;
	}

	if(!Si4461_wait_for_CTS())
	{
		CHIP_DESELECT();
		return false;
	}

	if(!Si4461_SPI_transfer_no_rx((uint8_t*)SI4461_CMD_READ_CMD_BUFF, 1))
	{
		CHIP_DESELECT();
		return false;
	}

	if(!Si4461_SPI_transfer_no_tx(params, length))
	{
		CHIP_DESELECT();
		return false;
	}

	CHIP_DESELECT();
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
	uint8_t power_up[] = {RF_POWER_UP};
	GPIO_PortSet(SI4461_SDN_GPIO, SI4461_SDN_PIN_MASK);
	delay(100);
	GPIO_PortClear(SI4461_SDN_GPIO, SI4461_SDN_PIN_MASK);
	delay(100);

	CHIP_SELECT();
	Si4461_SPI_transfer_no_rx(power_up, sizeof(power_up));
	CHIP_DESELECT();
	delay(100);

	//apply startup config
	if(!Si4461_set_config(Si4461_startup_config, sizeof(Si4461_startup_config)))
		return false;
	//set power
	if(!Si4461_set_tx_power(127))
		return false;
	//read pending interrupts and clear them
	if(!Si4461_interrupts())
		return false;
	//set sleep state
	if(!Si4461_set_state(SI4461_STATE_SLEEP))
		return false;

	//test device
	si4461_info_t info;
	if(!Si4461_get_info(&info))
		return false;

	if(info.chip_rev != 0x22 && info.part != 0x4461)
		return false;

	return true;
}

bool Si4461_get_info(si4461_info_t *info)
{
	uint8_t data[9] = {0};

	if(!Si4461_read_command(SI4461_CMD_PART_INFO, data, sizeof(data)))
		return false;

	info->chip_rev = data[1];
	info->part = (data[2] << 8) | data[3];
	info->part_build = data[4];
	info->id = (data[5] << 8) | data[6];
	info->customer = data[7];
	info->rom_id = data[8];

	return true;
}

