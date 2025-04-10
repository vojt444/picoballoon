/*
 * ublox_max_m10m.c
 *
 *  Created on: 8. 4. 2025
 *      Author: vojtech
 */

#include "ublox_max_m10m.h"

//VARIABLES
static lpuart_config_t max_m10_uart_config;
static lpuart_handle_t max_m10_uart_handle;
static uint8_t max_m10_rx_buffer[MAX_M10_BUFFER_SIZE];
static uint8_t max_m10_tx_buffer[MAX_M10_BUFFER_SIZE];
static volatile bool max_m10_rx_complete = true;
static volatile bool max_m10_tx_complete = true;
static uint16_t max_m10_rx_index = 0;

//HEADERS
static uint8_t max_m10_send_ubx_message(uint8_t msg_class, uint8_t msg_id, uint8_t *data, uint16_t data_len);
static void max_m10_calc_checksum(const uint8_t *data, uint16_t len, uint8_t *ck_a, uint8_t *ck_b);

//CALLBACK
void LPUART_callback(LPUART_Type *base, lpuart_handle_t *handle, status_t status, void *userData)
{
	userData = userData;

	if(kStatus_LPUART_TxIdle == status)
	{
		max_m10_tx_complete = true;
	}

	if(kStatus_LPUART_RxIdle == status)
	{
		max_m10_rx_complete = true;
		max_m10_rx_index = handle->rxData - max_m10_rx_buffer;
	}

}

//FUNCTIONS
bool max_m10_init(void)
{
	CLOCK_EnableClock(kCLOCK_Lpuart0);

	LPUART_GetDefaultConfig(&max_m10_uart_config);
	max_m10_uart_config.baudRate_Bps = LPUART_BAUDRATE;
	max_m10_uart_config.enableRx = true;
	max_m10_uart_config.enableTx = true;

	if(LPUART_Init(LPUART_BASE, &max_m10_uart_config, LPUART_CLK_FREQ) != kStatus_Success)
		return false;

	LPUART_TransferCreateHandle(LPUART_BASE, &max_m10_uart_handle, LPUART_callback, NULL);

	for(volatile int i = 0; i < 100000; i++);

	if(!max_m10_verify_comm())
		return false;

	return true;
}

bool max_m10_set_mode(max_m10_mode_t mode)
{
	uint8_t data[8];
	memset(data, 0, sizeof(data));

	switch(mode)
	{
		case MAX_M10_MODE_NORMAL:
			data[0] = UBX_CFG_PMS_FULL_POWER;
			break;
		case MAX_M10_MODE_POWERSAVE:
			data[0] = UBX_CFG_PMS_BALANCED;
			break;
		case MAX_M10_MODE_SLEEP:
			data[0] = UBX_CFG_PMS_PSMOO;
			break;
		default:
			return false;
	}
	return true;
}

bool max_m10_verify_comm(void)
{
	uint8_t retry_count = 5;

	while(retry_count--)
	{
		if(!max_m10_send_ubx_message(UBX_CLASS_MON, UBX_MON_VER, NULL, 0))
			continue;

		lpuart_transfer_t rx_transfer;
		rx_transfer.data = max_m10_rx_buffer;
		rx_transfer.dataSize = MAX_M10_BUFFER_SIZE;

		max_m10_rx_complete = false;
		max_m10_rx_index = 0;
		if(LPUART_TransferReceiveNonBlocking(LPUART_BASE, &max_m10_uart_handle, &rx_transfer, NULL)
				!= kStatus_Success)
			continue;

		uint32_t timeout = MAX_M10_UART_TIMEOUT;
		while(!max_m10_rx_complete && timeout--)
			for(volatile int i = 0; i < 10; i++);

		if(!max_m10_rx_complete)
		{
			LPUART_TransferAbortReceive(LPUART_BASE, &max_m10_uart_handle);
			continue;
		}

		for(uint16_t i = 0; i < max_m10_rx_index; i++)
		{
			if(max_m10_rx_buffer[i] == UBX_SYNC_CHAR_1 && max_m10_rx_buffer[i + 1] == UBX_SYNC_CHAR_2)
				if(i + 6 < max_m10_rx_index)
					if(max_m10_rx_buffer[i + 2] == UBX_CLASS_MON && max_m10_rx_buffer[i + 3] == UBX_MON_VER)
						return true;
		}
	}

	return false;
}

static uint8_t max_m10_send_ubx_message(uint8_t msg_class, uint8_t msg_id, uint8_t *data, uint16_t data_len)
{
	uint16_t tx_index = 0;

	max_m10_tx_buffer[tx_index++] = UBX_SYNC_CHAR_1;
	max_m10_tx_buffer[tx_index++] = UBX_SYNC_CHAR_2;
	max_m10_tx_buffer[tx_index++] = msg_class;
	max_m10_tx_buffer[tx_index++] = msg_id;

	uint8_t length[2] = {0};
	uint16_to_little_endian(data_len, length);

	max_m10_tx_buffer[tx_index++] = length[0];
	max_m10_tx_buffer[tx_index++] = length[1];

	uint8_t ck_a = 0, ck_b = 0;

	for(uint8_t i = 2; i < 6; i++)
	{
		ck_a += max_m10_tx_buffer[i];
		ck_b += ck_a;
	}

	if(data_len > 0 && data != NULL)
	{
		if(tx_index + data_len >= MAX_M10_BUFFER_SIZE)
			return false;

		for(uint16_t i = 0; i < data_len; i++)
		{
			max_m10_tx_buffer[tx_index++] = data[i];
			ck_a += data[i];
			ck_b += ck_a;
		}
	}

	max_m10_tx_buffer[tx_index++] = ck_a;
	max_m10_tx_buffer[tx_index++] = ck_b;

	lpuart_transfer_t tx_transfer;
	tx_transfer.txData = max_m10_tx_buffer;
	tx_transfer.dataSize = tx_index;

	max_m10_tx_complete = false;
	if(LPUART_TransferSendNonBlocking(LPUART_BASE, &max_m10_uart_handle, &tx_transfer) != kStatus_Success)
		return false;

	uint32_t timeout = MAX_M10_UART_TIMEOUT;
	while(!max_m10_tx_complete && timeout--)
		for(volatile int i = 0; i < 10; i++);

	if(!max_m10_tx_complete)
	{
		LPUART_TransferAbortSend(LPUART_BASE, &max_m10_uart_handle);
		return false;
	}

	return true;
}

/*
static bool parse_nmea_sentence(char *sentence)
{

	if(sentence == NULL || sentence[0] != NMEA_START_CHAR)
		return false;

//check for GGA sentence
//$xxGGA,time,lat,NS,lon,EW,quality,numSV,HDOP,alt,altUnit,sep,sepUnit,diffAge,diffStation*cs\r\n
	if(strncmp(sentence + 1, "GPGGA", 5) == 0 || strncmp(sentence + 1, "GNGGA", 5) == 0)
	{
		char *ptr = sentence;
		char *field[15];
		uint8_t field_count = 0;

		while((sentence = strchr(sentence, ',')) != NULL && field_count < 15)
		{
			*ptr = '\0';
			field[field_count++] = ++ptr;
		}

		char *checksum = strchr(field[field_count - 1], '*');

		if(checksum != NULL)
			*checksum = '\0';

		if(field_count < 14)
			return false;

		//parse time
		float time = strtof(field[0], NULL);
		current_pos.timestamp = (uint32_t)time;

		//parse latitude
		if(strlen(field[1]) > 0)
		{
			float latitude = strtof(field[1], NULL);
			int latitude_degrees = (int)(latitude / 100);
			float latitude_minutes = latitude - (latitude_degrees * 100);
			current_pos.latitude = latitude_degrees + (latitude_minutes / 60.0f);

			if(field[2][0] == 'S')
				current_pos.latitude = -current_pos.latitude;
		}

		//parse longitude
		if(strlen(field[3]) > 0)
		{
			float longitude = strtof(field[3], NULL);
			int longitude_degrees = (int)(longitude / 100);
			float longitude_minutes = longitude - (longitude_degrees * 100);
			current_pos.longitude = longitude_degrees + (longitude_minutes / 60.0f);

			if(field[4][0] == 'W')
				current_pos.longitude = -current_pos.longitude;
		}

		//parse quality
		int quality = atoi(field[5]);
		switch(quality)
		{
			case 0:
				current_pos.fix = UBLOX_FIX_NONE;
				break;
			case 1:
				current_pos.fix = UBLOX_FIX_3D;
				break;
			case 2:
				current_pos.fix = UBLOX_FIX_3D_DGPS;
				break;
			case 4:
				current_pos.fix = UBLOX_FIX_RTK_FIXED;
				break;
			case 5:
				current_pos.fix = UBLOX_FIX_RTK_FLOAT;
				break;
			default:
				current_pos.fix = UBLOX_FIX_NONE;
		}

		//parse number of satellites
		current_pos.satellites = atoi(field[6]);

		//parse HDOP
		current_pos.hdop = strtof(field[7], NULL);

		//parse altitude
		current_pos.altitude = strtof(field[8], NULL);

		//new pos available
		new_pos_available = true;

		return true;
	}
//check for GLL sentence
//$xxGLL,lat,NS,lon,EW,time,status,posMode*cs\r\n
	else if(strncmp(sentence + 1, "GPGLL", 5) == 0 || strncmp(sentence + 1, "GNGLL", 5) == 0)
	{
		char *ptr = sentence;
		char *field[8];
		uint8_t field_count = 0;

		while((sentence = strchr(sentence, ',')) != NULL && field_count < 8)
		{
			*ptr = '\0';
			field[field_count++] = ++ptr;
		}

		char *checksum = strchr(field[field_count - 1], '*');

		if(checksum != NULL)
			*checksum = '\0';

		if(field_count < 6)
			return false;

		//parse latitude
		if(strlen(field[0]) > 0)
		{
			float latitude = strtof(field[0], NULL);
			int latitude_degrees = (int)(latitude / 100);
			float latitude_minutes = latitude - (latitude_degrees * 100);
			current_pos.latitude = latitude_degrees + (latitude_minutes / 60.0f);

			if(field[1][0] == 'S')
				current_pos.latitude = -current_pos.latitude;
		}

		//parse longitude
		if(strlen(field[2]) > 0)
		{
			float longitude = strtof(field[2], NULL);
			int longitude_degrees = (int)(longitude / 100);
			float longitude_minutes = longitude - (longitude_degrees * 100);
			current_pos.longitude = longitude_degrees + (longitude_minutes / 60.0f);

			if(field[3][0] == 'W')
				current_pos.longitude = -current_pos.longitude;
		}

		//parse time
		float time = strtof(field[4], NULL);
		current_pos.timestamp = (uint32_t)time;

		//parse status
		if(field[5][0] != 'A')
		{
			current_pos.fix = UBLOX_FIX_NONE;
			return false;
		}

		//2D fix since GLL doesn't provide altitude
		current_pos.fix = UBLOX_FIX_2D;

		//new pos available
		new_pos_available = true;

		return true;
	}
//check for RMC
//$xxRMC,time,status,lat,NS,lon,EW,spd,cog,date,mv,mvEW,posMode,navStatus*cs\r\n
	else if(strncmp(sentence + 1, "GPRMC", 5) == 0 || strncmp(sentence + 1, "GNRMC", 5) == 0)
	{
		char *ptr = sentence;
		char *field[14];
		uint8_t field_count = 0;

		while((sentence = strchr(sentence, ',')) != NULL && field_count < 14)
		{
			*ptr = '\0';
			field[field_count++] = ++ptr;
		}

		char *checksum = strchr(field[field_count - 1], '*');

		if(checksum != NULL)
			*checksum = '\0';

		if(field_count < 14)
			return false;

		//parse time
		float time = strtof(field[0], NULL);
		current_pos.timestamp = (uint32_t)time;

		//parse status
		if(field[1][0] != 'A')
		{
			current_pos.fix = UBLOX_FIX_NONE;
			return false;
		}
		//parse latitude
		if(strlen(field[2]) > 0)
		{
			float latitude = strtof(field[2], NULL);
			int latitude_degrees = (int)(latitude / 100);
			float latitude_minutes = latitude - (latitude_degrees * 100);
			current_pos.latitude = latitude_degrees + (latitude_minutes / 60.0f);

			if(field[3][0] == 'S')
				current_pos.latitude = -current_pos.latitude;
		}

		//parse longitude
		if(strlen(field[4]) > 0)
		{
			float longitude = strtof(field[4], NULL);
			int longitude_degrees = (int)(longitude / 100);
			float longitude_minutes = longitude - (longitude_degrees * 100);
			current_pos.longitude = longitude_degrees + (longitude_minutes / 60.0f);

			if(field[5][0] == 'W')
				current_pos.longitude = -current_pos.longitude;
		}

		//parse speed
		if(strlen(field[6]) > 0)
			current_pos.speed = strtof(field[6], NULL) * 0.51444f; //knots to m/s

		//parse course
		if(strlen(field[7]) > 0)
			current_pos.course = strtof(field[7], NULL);

		//parse date
		if(strlen(field[8]) > 0)
			current_pos.date = (uint32_t)atoi(field[8]);

		//3D fix since RMC doesn't provide fix type
		current_pos.fix = UBLOX_FIX_3D;

		//new pos available
		new_pos_available = true;

		return true;
	}

	return false;
}
*/

static bool max_m10_disable_messages(void)
{
	uint8_t data[128] = {0};
	uint16_t data_len = 0;



}

static void max_m10_calc_checksum(const uint8_t *data, uint16_t len, uint8_t *ck_a, uint8_t *ck_b)
{
	*ck_a = 0;
	*ck_b = 0;
	for(int i = 0; i < len; i++)
	{
		*ck_a += data[i];
		*ck_b += *ck_a;
	}
}

