/*
 * aprs.c
 *
 *  Created on: 30. 3. 2025
 *      Author: vojtech
 */

#include "aprs.h"

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

void create_aprs_packet(uint8_t *packet, size_t *packet_len, char *src,
						uint8_t src_ssid, char *dst, uint8_t dst_ssid,
						char *digipeaters[][2], int digi_count, char *payload)
{
	uint8_t frame[256];
	*packet_len = 0;

	frame[(*packet_len)++] = 0x7E;

	ax25_encode_callsign(dst, dst_ssid, &frame[(*packet_len)]);
	*packet_len += 7;

	ax25_encode_callsign(src, src_ssid, &frame[(*packet_len)]);
	*packet_len += 7;

	for(int i = 0; i < digi_count; i++)
	{
		ax25_encode_callsign(digipeaters[i][0], atoi(digipeaters[i][1]),
							 &frame[*packet_len]);
		if(i == digi_count - 1)
			frame[(*packet_len) + 6] |= 0x01;

		*packet_len += 7;
	}

	frame[(*packet_len)++] = 0x03;
	frame[(*packet_len)++] = 0xF0;

	strncpy((char*)&frame[*packet_len], payload, strlen(payload));
	(*packet_len) += strlen(payload);

	uint16_t crc = ax25_fcs(frame, *packet_len);
	frame[(*packet_len)++] = crc & 0xFF;
	frame[(*packet_len)++] = (crc >> 8) & 0xFF;

	frame[(*packet_len)++] = 0x7E;

	memcpy(packet, frame, *packet_len);
}

