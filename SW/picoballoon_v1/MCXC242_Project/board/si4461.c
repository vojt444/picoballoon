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

static bool Si4461_fifo_unreset(void);

void SPI0_IRQHandler(void)
{
	SPI_MasterTransferHandleIRQ(SPI_BASE, &g_spi_handle);
}

static void spi_callback(SPI_Type *base, spi_master_handle_t *masterHandle, status_t status, void *userData)
{
	if(status == kStatus_Success)
	{
		g_spi_master_finished_flag = true;
	}
}

static bool Si4461_SPI_transfer(uint8_t *tx_buff, size_t tx_len, uint8_t *rx_buff, size_t rx_len)
{
	spi_transfer_t transfer;
	uint8_t dummy_rx[tx_len];
	memset(dummy_rx, 0, tx_len);

	transfer.txData = tx_buff;
	transfer.rxData = dummy_rx;
	transfer.dataSize = tx_len;

	g_spi_master_finished_flag = false;
	if(SPI_MasterTransferNonBlocking(SPI_BASE, &g_spi_handle, &transfer) != kStatus_Success)
		return false;

	uint32_t timeout = TIMEOUT;
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
	if(SPI_MasterTransferNonBlocking(SPI_BASE, &g_spi_handle, &transfer) != kStatus_Success)
		return false;

	timeout = TIMEOUT;
	while(!g_spi_master_finished_flag && timeout--)
	{
		if(timeout == 0)
			return false;
	}

	return true;
}

static bool Si4461_SPI_transfer_no_tx(uint8_t *rx_buff, size_t rx_len)
{
	spi_transfer_t transfer;
	uint8_t dummy_tx[rx_len];
	memset(dummy_tx, 0x44, rx_len);
	memset(rx_buff, 0, rx_len);

	transfer.txData = dummy_tx;
	transfer.rxData = rx_buff;
	transfer.dataSize = rx_len;

	g_spi_master_finished_flag = false;
	if(SPI_MasterTransferNonBlocking(SPI_BASE, &g_spi_handle, &transfer) != kStatus_Success)
		return false;

	uint32_t timeout = TIMEOUT;
	while(!g_spi_master_finished_flag && timeout--)
	{
		if(timeout == 0)
			return false;
	}

	return true;
}

static bool Si4461_SPI_transfer_no_rx(uint8_t *tx_buff, size_t tx_data_len)
{
	spi_transfer_t transfer;
	transfer.txData = tx_buff;
	transfer.rxData = NULL;
	transfer.dataSize = tx_data_len;

	g_spi_master_finished_flag = false;
	if(SPI_MasterTransferNonBlocking(SPI_BASE, &g_spi_handle, &transfer) != kStatus_Success)
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
	spi_transfer_t transfer;
	uint8_t tx[2] = {SI4461_CMD_READ_CMD_BUFF, 0}; // SI4461_CMD_PART_INFO
	uint8_t rx[2] = {0};

	uint16_t timeout_cts = CTS_TIMEOUT;
	while(timeout_cts--)
	{
		transfer.txData = tx;
		transfer.rxData = rx;
		transfer.dataSize = 2;

		g_spi_master_finished_flag = false;
		CHIP_SELECT();
		if(SPI_MasterTransferNonBlocking(SPI_BASE, &g_spi_handle, &transfer) != kStatus_Success)
			return false;

		uint32_t timeout = TIMEOUT;
		while(!g_spi_master_finished_flag && timeout--)
		{
			for(int i = 0; i < 10; i++);
		}
		CHIP_DESELECT();

		if(timeout == 0)
		{
			CHIP_DESELECT();
			return false;
		}

		if(rx[1] == 0xFF)
			return true;

		for(uint8_t i = 0; i < 100; i++);
	}

	return false;
}

static bool Si4461_write_command(uint8_t cmd, uint8_t *data, size_t data_len)
{
	uint8_t data_send[data_len + 1];
	data_send[0] = cmd;
	memcpy(&data_send[1], data, data_len);

	uint8_t received = 0;
	CHIP_SELECT();
	if(!Si4461_SPI_transfer(data_send, data_len + 1, &received, 1))
		return false;
	CHIP_DESELECT();

	if(received != 0xFF)
		return false;

	return true;
}

static bool Si4461_read_command(uint8_t command, uint8_t *buff, size_t len)
{
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

	//POWER UP already sent, so skipped
	config_len = config_len - 1;
	cmd_len = config[0];
	pos = cmd_len + 1;

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

static bool Si4461_interrupts(void)
{
	uint8_t tx_data[4] = {SI4461_CMD_GET_INT_STATUS, 0};
	uint8_t rx_data[9] = {0};

	CHIP_SELECT();
	if(!Si4461_SPI_transfer(tx_data, 4, rx_data, 9))
	{
		CHIP_DESELECT();
		return false;
	}
	CHIP_DESELECT();
	return true;
}

static bool Si4461_read_FRR(uint8_t reg, uint8_t *value)
{
	uint8_t tx_data = reg;
	CHIP_SELECT();
	if(!Si4461_SPI_transfer(&tx_data, 1, value, 1))
	{
		CHIP_DESELECT();
		return false;
	}
	CHIP_DESELECT();
	return true;
}

static bool Si4461_set_filter(void)
{
	uint8_t coeffs[9] = {0x1d, 0xe5, 0xb8, 0xaa, 0xc0, 0xf5, 0x36, 0x6b, 0x7f}; // 6dB@1200 Hz, 2400 Hz

	if(!Si4461_set_properties(0x200f, 0x01, &coeffs[8]))
		return false;
	if(!Si4461_set_properties(0x2010, 0x01, &coeffs[7]))
		return false;
	if(!Si4461_set_properties(0x2011, 0x01, &coeffs[6]))
		return false;
	if(!Si4461_set_properties(0x2012, 0x01, &coeffs[5]))
		return false;
	if(!Si4461_set_properties(0x2013, 0x01, &coeffs[4]))
		return false;
	if(!Si4461_set_properties(0x2014, 0x01, &coeffs[3]))
		return false;
	if(!Si4461_set_properties(0x2015, 0x01, &coeffs[2]))
		return false;
	if(!Si4461_set_properties(0x2016, 0x01, &coeffs[1]))
		return false;
	if(!Si4461_set_properties(0x2017, 0x01, &coeffs[0]))
		return false;

	return true;
}

bool Si4461_init(uint32_t frequency)
{
	spi_master_config_t spi_config = {0};
	uint32_t source_clock = 0;

	//init SPI
	SPI_MasterGetDefaultConfig(&spi_config);
	spi_config.baudRate_Bps = 1000000;
	spi_config.outputMode = kSPI_SlaveSelectAsGpio;
	source_clock = SPI_CLOCK_SOURCE;
	SPI_MasterInit(SPI_BASE, &spi_config, source_clock);

	SPI_MasterTransferCreateHandle(SPI_BASE, &g_spi_handle, spi_callback, NULL);
	NVIC_EnableIRQ(SPI0_IRQn);

	//reset device
	GPIO_PortSet(SI4461_SDN_GPIO, SI4461_SDN_PIN_MASK);
	delay_ms(100);
	GPIO_PortClear(SI4461_SDN_GPIO, SI4461_SDN_PIN_MASK);
	delay_ms(50);

	//startup
	uint8_t power_up[] = {RF_POWER_UP};
	uint8_t cts = 0;
	CHIP_SELECT();
	if(!Si4461_SPI_transfer(power_up, 7, &cts, 1))
		return false;
	CHIP_DESELECT();
	if(cts != 0xff)
		return false;
	delay_ms(200);

	//apply startup config
	if(!Si4461_set_config(Si4461_startup_config, sizeof(Si4461_startup_config)))
		return false;

	//set power
	if(!Si4461_set_tx_power(0x10))
		return false;

/*	if(!Si4461_freq_offset(0))
		return false;

	if(!Si4461_deviation((uint32_t)(2*FDEV_APRS)))
		return false;

	if(!Si4461_freq_control((uint8_t)FDIV_INTE_2M, (uint32_t)FDIV_FRAC_2M))
		return false;*/

	//set frequency
	if(!Si4461_set_freq(frequency))
		return false;
	if(!Si4461_set_filter())
		return false;

	//reset fifo
	if(!Si4461_fifo_reset())
		return false;
	if(!Si4461_fifo_unreset())
		return false;

	//test device
	uint8_t state;
	Si4461_get_state(&state);

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

bool Si4461_set_properties(uint16_t start_property, uint8_t length, uint8_t *params)
{
	uint8_t buff[4];
	buff[0] = SI4461_CMD_SET_PROPERTY;
	buff[1] = start_property >> 8;
	buff[2] = length;
	buff[3] = (uint8_t)(start_property && 0xFF);

	uint8_t received = 0;

	CHIP_SELECT();
	if(!Si4461_SPI_transfer_no_rx(buff, 4))
	{
		CHIP_DESELECT();
		return false;
	}
	if(!Si4461_SPI_transfer(params, length, &received, 1))
	{
		CHIP_DESELECT();
		return false;
	}
	CHIP_DESELECT();

	if(received != 0xFF)
		return false;

	return true;
}

bool Si4461_get_properties(uint16_t start_property, uint8_t length, uint8_t *params)
{
	if(!Si4461_wait_for_CTS())
		return false;

	uint8_t buff[4];
	buff[0] = SI4461_CMD_GET_PROPERTY;
	buff[1] = (uint8_t)(start_property >> 8);
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

bool Si4461_gpio_cfg(uint8_t *data)
{
	uint8_t gpio_cfg[] = {0x13, 0x00, 0x44, 0x00, 0x00, 0x00, 0x00, 0x00};
	CHIP_SELECT();
	if(!Si4461_SPI_transfer_no_rx(gpio_cfg, 8))
	{
		CHIP_DESELECT();
		return false;
	}

	CHIP_DESELECT();
	return true;
}

bool Si4461_fifo_reset(void)
{
	uint8_t buff[] = {0x01};
	if(!Si4461_write_command(SI4461_CMD_FIFO_INFO, buff, 1))
		return false;

	return true;
}

static bool Si4461_fifo_unreset(void)
{
	uint8_t buff[] = {0x00};
	if(!Si4461_write_command(SI4461_CMD_FIFO_INFO, buff, 1))
		return false;

	return true;
}

bool Si4461_fifo_info(si4461_fifo_info_t *fifo_info)
{
	uint8_t data[3] = {0};

	if(!Si4461_read_command(SI4461_CMD_FIFO_INFO, data, sizeof(data)))
		return false;

	fifo_info->rx_fifo_count = data[1];
	fifo_info->tx_fifo_count = data[2];

	return true;
}

bool Si4461_fifo_write(uint8_t *packet, size_t packet_length)
{
	if(!Si4461_write_command(SI4461_CMD_WRITE_TX_FIFO, packet, packet_length))
		return false;

	return true;
}

bool Si4461_send_packet(uint8_t *packet, size_t packet_length)
{
	uint8_t state;
	if(!Si4461_get_state(&state))
		return false;
	if(state == SI4461_STATE_TX)
		return false;
	if(!Si4461_fifo_reset())
		return false;
	if(!Si4461_fifo_unreset())
		return false;

	if(!Si4461_fifo_write(packet, packet_length))
		return false;

	uint8_t data[4] = {0, 0x10, (uint8_t)((packet_length >> 8) & 0x1F), (uint8_t)(packet_length & 0xFF)};

	if(!Si4461_start_TX(data))
		return false;

	do
	{
		if(!Si4461_get_state(&state))
			return false;
	} while(state == SI4461_STATE_TX);

	if(!Si4461_set_state(SI4461_STATE_SLEEP))
		return false;

	return true;
}

bool Si4461_get_state(uint8_t *state)
{
	uint8_t data[3] = {0};

	if(!Si4461_read_command(SI4461_CMD_REQUEST_DEVICE_STATE, data, sizeof(data)))
		return false;

	*state = (data[1] & 0x0F);
	return true;
}

bool Si4461_set_state(uint8_t new_state)
{
//	uint8_t data[2] = {SI4461_CMD_CHANGE_STATE, new_state};
//	uint8_t cts = 0;
//
//	if(!Si4461_SPI_transfer(data, 2, &cts, 1))
//		return false;

	if(!Si4461_write_command(SI4461_CMD_CHANGE_STATE, &new_state, 1))
		return false;

//	if(cts != 0xff)
//		return false;

	return true;
}

bool Si4461_NOP(void)
{
	uint8_t response = 0;

	if(!Si4461_read_command(SI4461_CMD_NOP, &response, 1))
		return false;

	return (response == 0xFF);
}

bool Si4461_set_tx_power(uint8_t power)
{
	if(power > 127)
		power = 127;

	return Si4461_set_properties(SI4461_PA_PWR_LVL, 1, &power);
}

bool Si4461_freq_offset(uint16_t offset)
{
	uint8_t data[2] = {(uint8_t)(offset >> 8), (uint8_t)(offset & 0x0F)};

	return Si4461_set_properties(SI4461_MODEM_FREQ_OFFSET, 2, data);
}

bool Si4461_deviation(uint32_t deviation)
{
	uint32_t deviation_val = deviation & 0x1FFFF;
	uint8_t data[3] = {(uint8_t)(deviation_val >> 16), (uint8_t)((deviation_val >> 8) & 0xFF), (uint8_t)((deviation_val) & 0xFF)};

	return Si4461_set_properties(SI4461_MODEM_FREQ_DEV, 3, data);
}

bool Si4461_freq_control(uint8_t inte, uint32_t frac)
{
	uint8_t inte_data = inte & 0x7F;
	if(!Si4461_set_properties(SI4461_FREQ_CONTROL_INTE, 1, &inte_data))
		return false;

	uint8_t frac_data[3] = {(uint8_t)((frac >> 16) & 0x0F), (uint8_t)((frac >> 8) & 0xFF), (uint8_t)(frac & 0xFF)};
	if(!Si4461_set_properties(SI4461_FREQ_CONTROL_FRAC, 3, frac_data))
		return false;

	return true;
}

bool Si4461_set_freq(uint32_t freq)
{
	uint8_t outdiv = 4;
	uint8_t band = 0;
	if(freq < 705000000UL)
	{
		outdiv = 6;
		band = 1;
	};
	if(freq < 525000000UL)
	{
		outdiv = 8;
		band = 2;
	};
	if(freq < 353000000UL)
	{
		outdiv = 12;
		band = 3;
	};
	if(freq < 239000000UL)
	{
		outdiv = 16;
		band = 4;
	};
	if(freq < 177000000UL)
	{
		outdiv = 24;
		band = 5;
	};

	uint32_t f_pfd = 2 * RADIO_CONFIGURATION_DATA_RADIO_XO_FREQ / outdiv;
	uint32_t n = ((uint32_t)(freq / f_pfd)) - 1;
	float ratio = (float)freq / (float)f_pfd;
	float rest = ratio - (float)n;
	uint32_t m = (uint32_t)(rest * 524288UL);
	uint32_t m2 = m >> 16;
	uint32_t m1 = (m - m2 * 0x10000) >> 8;
	uint32_t m0 = (m - m2 * 0x10000 - (m1 << 8));

	uint8_t buf = 8 + band;
	if(!Si4461_set_properties(SI4461_MODEM_CLKGEN_BAND, 1, &buf))
		return false;

	uint8_t buf2[4] = {n, m2, m1, m0};
	if(!Si4461_set_properties(SI4461_FREQ_CONTROL_INTE, sizeof(buf2), buf2))
		return false;

	return true;
}

bool Si4461_start_TX(uint8_t *tx_data)
{
	if(!Si4461_write_command(SI4461_CMD_START_TX, tx_data, 4))
		return false;

	return true;
}

bool Si4461_stop_TX(void)
{
	if(!Si4461_set_state(SI4461_STATE_TX_TUNE))
		return false;
	if(!Si4461_set_state(SI4461_STATE_READY))
		return false;

	return true;
}

void Si4461_shutdown(void)
{
	GPIO_PortSet(SI4461_SDN_GPIO, SI4461_SDN_PIN_MASK);
}

