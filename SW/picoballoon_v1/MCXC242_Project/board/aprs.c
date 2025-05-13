/*
 * aprs.c
 *
 *  Created on: 30. 3. 2025
 *      Author: vojtech
 */

#include "aprs.h"

static uint16_t msg_id = 0;

void aprs_encode_init(ax25_t *packet, uint8_t *data, uint16_t data_size, mod_t mod)
{
	packet->data = data;
	packet->max_size = data_size;
	packet->mod = mod;

	ax25_init(packet);
}

void aprs_encode_full_data_packet(ax25_t *packet, const aprs_conf_t *config, measured_data_t *data)
{
	char tmp[64] = {0};
	char tmp2[64] = {0};

	ax25_send_header(packet, config->callsign, config->ssid, config->path);
	ax25_send_byte(packet, '!');

	uint32_t lat = 380926 * (90 - data->position.latitude);
	uint32_t lat1 = lat / 753571;
	uint32_t lat1r = lat % 753571;

	uint32_t lat2 = lat1r / 8281;
	uint32_t lat2r = lat1r % 8281;

	uint32_t lat3 = lat2r / 91;
	uint32_t lat3r = lat2r % 91;

	uint32_t lon = 380926 * (180 + data->position.longitude);
	uint32_t lon1 = lon / 753571;
	uint32_t lon1r = lon % 753571;

	uint32_t lon2 = lon1r / 8281;
	uint32_t lon2r = lon1r % 8281;

	uint32_t lon3 = lon2r / 91;
	uint32_t lon3r = lon2r % 91;

	tmp[0] = lat1 + 33;
	tmp[1] = lat2 + 33;
	tmp[2] = lat3 + 33;
	tmp[3] = lat3r + 33;

	tmp[4] = lon1 + 33;
	tmp[5] = lon2 + 33;
	tmp[6] = lon3 + 33;
	tmp[7] = lon3r + 33;

	tmp[8] = '/';
	tmp[9] = 'O';

	ax25_send_string(packet, tmp);

	sprintf(tmp2, "A=%.1f,T=%.1f,P=%.1f,H=%.1f,%d", data->position.altitude, data->temperature,
			data->pressure, data->humidity, msg_id++);
	ax25_send_string(packet, tmp2);

	ax25_send_footer(packet);

	while(packet->byte_count < packet->max_size && packet->data[packet->byte_count] != 0)
		packet->byte_count++;
}

void aprs_encode_test_packet(ax25_t *packet, const aprs_conf_t *config)
{
	char test_string[11] = "Test packet";
	ax25_send_header(packet, config->callsign, config->ssid, config->path);
	ax25_send_byte(packet, ':');
	ax25_send_string(packet, test_string);
	ax25_send_footer(packet);

	while(packet->data[packet->byte_count] != 0)
		packet->byte_count++;
}

void aprs_encode_message(ax25_t *packet, const aprs_conf_t *config, const char *receiver, const char *text)
{
	char tmp[10];
	ax25_send_header(packet, config->callsign, config->ssid, config->path);
	ax25_send_byte(packet, ':');

	snprintf(tmp, sizeof(tmp), "%-9s", receiver);
	ax25_send_string(packet, tmp);

	ax25_send_string(packet, text);
	ax25_send_byte(packet, '{');

	snprintf(tmp, sizeof(tmp), "%d", ++msg_id);
	ax25_send_string(packet, tmp);

	ax25_send_footer(packet);
}

void ax25_encode_callsign(char *callsign, uint8_t ssid, uint8_t *encoded)
{
	memset(encoded, ' ' << 1, 6);

	for(uint8_t i = 0; i < 6; i++)
		encoded[i] = (callsign[i] << 1);

	encoded[6] = ((ssid & 0x0F) << 1);
}

uint16_t ax25_fcs(const uint8_t *data, size_t length)
{
	uint16_t crc = 0xFFFF;
	for(size_t i = 0; i < length; i++)
	{
		crc ^= (uint16_t)data[i];
		for(uint8_t j = 0; j < 8; j++)
		{
			if(crc & 0x0001)
				crc = (crc >> 1) ^ 0x8408;
			else
				crc >>= 1;
		}
	}
	return crc;
}

