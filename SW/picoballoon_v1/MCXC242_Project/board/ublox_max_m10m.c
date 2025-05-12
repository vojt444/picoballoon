/*
 * ublox_max_m10m.c
 *
 *  Created on: 8. 4. 2025
 *      Author: vojtech
 */

#include "ublox_max_m10m.h"

//VARIABLES
static lpuart_config_t uart_config;
static lpuart_handle_t uart_handle;

uint8_t tx_buffer[MAX_M10_BUFFER_SIZE];
uint8_t rx_buffer[MAX_M10_BUFFER_SIZE];
volatile bool rx_buffer_full = false;
volatile bool tx_complete = true;
volatile uint32_t rx_index = 0;

//HEADERS
static bool LPUART_send_data(uint8_t *data, size_t data_len);
static void LPUART_rx_char(void);
static bool max_m10_send_ubx_message(uint8_t msg_class, uint8_t msg_id, uint8_t *data, uint16_t data_len);
static bool max_m10_wait_ack(uint8_t msg_class, uint8_t msg_id);
static bool max_m10_disable_messages(void);

void LPUART0_IRQHandler(void)
{
	if((kLPUART_RxDataRegFullFlag) & LPUART_GetStatusFlags(LPUART0))
		LPUART_rx_char();
	else
		LPUART_TransferHandleIRQ(LPUART0, &uart_handle);

	__DSB();
}

static void LPUART_UserCallback(LPUART_Type *base, lpuart_handle_t *handle, status_t status, void *userData)
{
	if(status == kStatus_LPUART_TxIdle)
		tx_complete = true;
	else if(status == kStatus_LPUART_RxIdle)
		rx_buffer_full = true;
	else if(status == kStatus_LPUART_RxHardwareOverrun || status == kStatus_LPUART_RxRingBufferOverrun)
		rx_index = 0;
}

//FUNCTIONS
bool max_m10_init(void)
{
	LPUART_GetDefaultConfig(&uart_config);

	uart_config.baudRate_Bps = LPUART_BAUDRATE;
	uart_config.enableTx = true;
	uart_config.enableRx = true;

	LPUART_Init(LPUART0, &uart_config, LPUART_CLK_FREQ);
	LPUART_TransferCreateHandle(LPUART0, &uart_handle, LPUART_UserCallback, NULL);
	LPUART_EnableInterrupts(LPUART0, kLPUART_RxDataRegFullInterruptEnable);

	EnableIRQ(LPUART0_IRQn);

	for(uint32_t i = 0; i < MAX_M10_BUFFER_SIZE; i++)
		tx_buffer[i] = 0;

	for(uint32_t i = 0; i < MAX_M10_BUFFER_SIZE; i++)
		rx_buffer[i] = 0;

	for(uint8_t i = 0; i < 10; i++)
	{
		if(max_m10_disable_messages())
			break;
		if(i == 9)
			return false;
	}

	gnss_position_t pos;

	if(!max_m10_get_pos(&pos))
		return false;

	if(!max_m10_set_mode(MAX_M10_MODE_PSMOO))
		return false;

	return true;
}

static bool max_m10_send_ubx_message(uint8_t msg_class, uint8_t msg_id, uint8_t *data, uint16_t data_len)
{
	uint16_t tx_index = 0;

	tx_buffer[tx_index++] = UBX_SYNC_CHAR_1;
	tx_buffer[tx_index++] = UBX_SYNC_CHAR_2;
	tx_buffer[tx_index++] = msg_class;
	tx_buffer[tx_index++] = msg_id;
	tx_buffer[tx_index++] = data_len & 0xFF;
	tx_buffer[tx_index++] = (data_len >> 8) & 0xFF;

	uint8_t ck_a = 0, ck_b = 0;

	for(uint8_t i = 2; i < 6; i++)
	{
		ck_a += tx_buffer[i];
		ck_b += ck_a;
	}

	if(data_len > 0 && data != NULL)
	{
		if(tx_index + data_len >= MAX_M10_BUFFER_SIZE)
			return false;

		for(uint16_t i = 0; i < data_len; i++)
		{
			tx_buffer[tx_index++] = data[i];
			ck_a += data[i];
			ck_b += ck_a;
		}
	}

	tx_buffer[tx_index++] = ck_a;
	tx_buffer[tx_index++] = ck_b;

	if(!LPUART_send_data(tx_buffer, tx_index))
		return false;

	uint32_t timeout = UART_TIMEOUT;
	while(!tx_complete && timeout--)
		for(uint8_t i = 0; i < 50; i++);

	if(!tx_complete)
	{
		LPUART_TransferAbortSend(LPUART_INSTANCE, &uart_handle);
		return false;
	}
	return true;
}

bool max_m10_get_pos(gnss_position_t *position)
{
	if(!max_m10_send_ubx_message(UBX_CLASS_NAV, UBX_NAV_POSLLH, NULL, 0))
		return false;

	uint32_t start_time = get_time_ms();

	while((get_time_ms() - start_time) < 3000)
	{
		if(rx_buffer_full || rx_index >= 30)
		{
			for(uint32_t i = 0; i < rx_index - 7; i++)
			{
				if(rx_buffer[i] == UBX_SYNC_CHAR_1 && rx_buffer[i + 1] == UBX_SYNC_CHAR_2
						&& rx_buffer[i + 2] == UBX_CLASS_NAV && rx_buffer[i + 3] == UBX_NAV_POSLLH)
				{
					uint16_t payload_len = rx_buffer[i + 4] | (rx_buffer[i + 5] << 8);
					if(payload_len != 28)
						continue;
					else
					{
						int32_t longitude_int = (int32_t)(rx_buffer[i + 10] | (rx_buffer[i + 11] << 8)
								| (rx_buffer[i + 12] << 16) | (rx_buffer[i + 13] << 24));
						position->longitude = longitude_int * 1e-7f;

						int32_t latitude_int = (int32_t)(rx_buffer[i + 14] | (rx_buffer[i + 15] << 8)
								| (rx_buffer[i + 16] << 16) | (rx_buffer[i + 17] << 24));
						position->latitude = latitude_int * 1e-7f;

						int32_t altitude_int = (int32_t)(rx_buffer[i + 22] | (rx_buffer[i + 23] << 8)
								| (rx_buffer[i + 24] << 16) | (rx_buffer[i + 25] << 24));
						position->altitude = (float)altitude_int / 1000;

						memset(rx_buffer, 0, rx_index);
						rx_index = 0;
						rx_buffer_full = false;
						return true;
					}
				}
				else
					continue;

			}
		}
		else
			continue;
	}

	memset(rx_buffer, 0, rx_index);
	rx_index = 0;
	rx_buffer_full = false;
	return false;
}

static bool max_m10_disable_messages(void)
{
	uint8_t data[128] = {0};
	uint16_t data_len = 0;

	data[data_len++] = 0;
	data[data_len++] = UBX_CFG_LAYER_BOTH;
	data[data_len++] = 0;
	data[data_len++] = 0;

	uint32_to_little_endian(CFG_MSGOUT_NMEA_ID_GLL_UART1, &data[data_len]);
	data_len += 4;
	uint32_to_little_endian(0, &data[data_len]);
	data_len += 4;
	if(!max_m10_send_ubx_message(UBX_CLASS_CFG, UBX_CFG_VALSET, data, data_len))
		return false;
	if(!max_m10_wait_ack(UBX_CLASS_CFG, UBX_CFG_VALSET))
		return false;

	data_len = 4;
	uint32_to_little_endian(CFG_MSGOUT_NMEA_ID_GGA_UART1, &data[data_len]);
	data_len += 4;
	uint32_to_little_endian(0, &data[data_len]);
	data_len += 4;
	if(!max_m10_send_ubx_message(UBX_CLASS_CFG, UBX_CFG_VALSET, data, data_len))
		return false;
	if(!max_m10_wait_ack(UBX_CLASS_CFG, UBX_CFG_VALSET))
		return false;

	data_len = 4;
	uint32_to_little_endian(CFG_MSGOUT_NMEA_ID_GSA_UART1, &data[data_len]);
	data_len += 4;
	uint32_to_little_endian(0, &data[data_len]);
	data_len += 4;
	if(!max_m10_send_ubx_message(UBX_CLASS_CFG, UBX_CFG_VALSET, data, data_len))
		return false;
	if(!max_m10_wait_ack(UBX_CLASS_CFG, UBX_CFG_VALSET))
		return false;

	data_len = 4;
	uint32_to_little_endian(CFG_MSGOUT_NMEA_ID_GSV_UART1, &data[data_len]);
	data_len += 4;
	uint32_to_little_endian(0, &data[data_len]);
	data_len += 4;
	if(!max_m10_send_ubx_message(UBX_CLASS_CFG, UBX_CFG_VALSET, data, data_len))
		return false;
	if(!max_m10_wait_ack(UBX_CLASS_CFG, UBX_CFG_VALSET))
		return false;

	data_len = 4;
	uint32_to_little_endian(CFG_MSGOUT_NMEA_ID_RMC_UART1, &data[data_len]);
	data_len += 4;
	uint32_to_little_endian(0, &data[data_len]);
	data_len += 4;
	if(!max_m10_send_ubx_message(UBX_CLASS_CFG, UBX_CFG_VALSET, data, data_len))
		return false;
	if(!max_m10_wait_ack(UBX_CLASS_CFG, UBX_CFG_VALSET))
		return false;

	data_len = 4;
	uint32_to_little_endian(CFG_MSGOUT_NMEA_ID_VTG_UART1, &data[data_len]);
	data_len += 4;
	uint32_to_little_endian(0, &data[data_len]);
	data_len += 4;
	if(!max_m10_send_ubx_message(UBX_CLASS_CFG, UBX_CFG_VALSET, data, data_len))
		return false;
	if(!max_m10_wait_ack(UBX_CLASS_CFG, UBX_CFG_VALSET))
		return false;

	data_len = 4;
	uint32_to_little_endian(CFG_MSGOUT_NMEA_ID_ZDA_UART1, &data[data_len]);
	data_len += 4;
	uint32_to_little_endian(0, &data[data_len]);
	data_len += 4;
	if(!max_m10_send_ubx_message(UBX_CLASS_CFG, UBX_CFG_VALSET, data, data_len))
		return false;
	if(!max_m10_wait_ack(UBX_CLASS_CFG, UBX_CFG_VALSET))
		return false;

	data_len = 4;
	uint32_to_little_endian(CFG_MSGOUT_NMEA_ID_GST_UART1, &data[data_len]);
	data_len += 4;
	uint32_to_little_endian(0, &data[data_len]);
	data_len += 4;
	if(!max_m10_send_ubx_message(UBX_CLASS_CFG, UBX_CFG_VALSET, data, data_len))
		return false;
	if(!max_m10_wait_ack(UBX_CLASS_CFG, UBX_CFG_VALSET))
		return false;

	return true;
}

static bool LPUART_send_data(uint8_t *data, size_t data_len)
{
	lpuart_transfer_t transfer;

	if(tx_complete)
	{
		transfer.data = data;
		transfer.dataSize = data_len;

		tx_complete = false;

		if(LPUART_TransferSendNonBlocking(LPUART0, &uart_handle, &transfer) != kStatus_Success)
			return false;
	}
	return true;
}

static void LPUART_rx_char(void)
{
	uint8_t data = LPUART_ReadByte(LPUART0);

	if(rx_index < MAX_M10_BUFFER_SIZE)
	{
		rx_buffer[rx_index++] = data;
		if(data == '\n' || data == '\r' || rx_index >= MAX_M10_BUFFER_SIZE)
			rx_buffer_full = true;
		else if(rx_index >= 8)
		{
			for(uint32_t i = 0; i < rx_index - 8; i++)
			{
				if(rx_buffer[i] == UBX_SYNC_CHAR_1 && rx_buffer[i + 1] == UBX_SYNC_CHAR_2)
				{
					uint16_t payload_len = rx_buffer[i + 4] | (rx_buffer[i + 5] << 8);
					if(i + 8 + payload_len <= rx_index)
					{
						rx_buffer_full = true;
						break;
					}
				}
			}
		}
		if(rx_index >= MAX_M10_BUFFER_SIZE - 10)
			rx_buffer_full = true;
	}
	else
		rx_buffer_full = true;
}

static bool max_m10_wait_ack(uint8_t msg_class, uint8_t msg_id)
{
	rx_buffer_full = false;
	rx_index = 0;
	bool is_ack = false;
	bool is_nak = false;
	uint32_t timeout = 50000;

	while(timeout--)
	{
		if(rx_buffer_full || rx_index >= 9)
		{
			for(uint32_t i = 0; i < rx_index - 7; i++)
			{
				if(rx_buffer[i] == UBX_SYNC_CHAR_1 && rx_buffer[i + 1] == UBX_SYNC_CHAR_2)
					if(rx_buffer[i + 2] == UBX_CLASS_ACK)
					{
						if(rx_buffer[i + 3] == UBX_ACK_ACK)
						{
							if(rx_buffer[i + 6] == msg_class && rx_buffer[i + 7] == msg_id)
							{
								is_ack = true;
								break;
							}
						}
						else if(rx_buffer[i + 3] == UBX_ACK_NAK)
						{
							if(rx_buffer[i + 6] == msg_class && rx_buffer[i + 7] == msg_id)
							{
								is_nak = true;
								break;
							}
						}
					}
			}
		}
		if(is_ack)
		{
			memset(rx_buffer, 0, rx_index);
			rx_index = 0;
			rx_buffer_full = false;
			return true;
		}
		if(is_nak)
		{
			memset(rx_buffer, 0, rx_index);
			rx_index = 0;
			rx_buffer_full = false;
			return true;
		}
		if(timeout == 0)
		{
			memset(rx_buffer, 0, rx_index);
			rx_index = 0;
			rx_buffer_full = false;
			return false;
		}

		if(rx_index > MAX_M10_BUFFER_SIZE / 2)
			rx_index = 0;
		rx_buffer_full = false;
	}

	return false;
}

bool max_m10_sleep(void)
{
	uint8_t data[32] = {0};
	uint16_t data_len = 0;

	//version + reserved = 4 bytes
	data[data_len++] = 0;
	data[data_len++] = 0;
	data[data_len++] = 0;
	data[data_len++] = 0;
	//duration (0 for wakeup signal) = 4 bytes
	data[data_len++] = 0;
	data[data_len++] = 0;
	data[data_len++] = 0;
	data[data_len++] = 0;
	//flags = 4 bytes
	data[data_len++] = 0x40; //minimum power consumption
	data[data_len++] = 0;
	data[data_len++] = 0;
	data[data_len++] = 0;
	//wakeup source = 4 bytes
	data[data_len++] = 0x20; //wakeup when there is an edge on EXTINT pin
	data[data_len++] = 0;
	data[data_len++] = 0;
	data[data_len++] = 0;

	if(!max_m10_send_ubx_message(UBX_CLASS_RXM, UBX_RXM_PMREQ, data, data_len))
		return false;

	return true;
}

bool max_m10_set_mode(max_m10_mode_t mode)
{
	uint8_t data[64] = {0};
	uint16_t data_len = 0;

	data[data_len++] = 0;
	data[data_len++] = UBX_CFG_LAYER_BOTH;
	data[data_len++] = 0;
	data[data_len++] = 0;

	uint32_to_little_endian(mode, &data[data_len]);
	data_len += 4;
	if(!max_m10_send_ubx_message(UBX_CLASS_CFG, UBX_CFG_VALSET, data, data_len))
		return false;
	if(!max_m10_wait_ack(UBX_CLASS_CFG, UBX_CFG_VALSET))
		return false;

	return true;
}

