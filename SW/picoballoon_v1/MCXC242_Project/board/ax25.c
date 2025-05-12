/*
 * ax25.c
 *
 *  Created on: 3. 5. 2025
 *      Author: vojtech
 */

#include "ax25.h"

#define AX25_WRITE_BIT(data, size) data[size >> 3] |= (1 << (size & 7))
#define AX25_CLEAR_BIT(data, size) data[size >> 3] &= ~(1 << (size & 7))

static void ax25_send_flag(ax25_t *packet);
static void update_crc(ax25_t *packet, char bit);
static void ax25_send_byte_no_crc(ax25_t *packet, char byte);

void ax25_init(ax25_t *packet)
{
	packet->bit_count = 0;
	packet->byte_count = 0;
	packet->ones_in_a_row = 0;
}

void ax25_send_header(ax25_t *packet, const char *callsign, uint8_t ssid, const char *path)
{
	uint8_t tmp[8];
	packet->ones_in_a_row = 0;
	packet->crc = 0xffff;

	for(uint8_t i = 0; i < 4; i++)
		ax25_send_flag(packet);

	ax25_send_path(packet, APRS_DEST_CALLSIGN, APRS_DEST_SSID, false);
	ax25_send_path(packet, callsign, ssid, path[0] == 0 || path == NULL);

	uint16_t i, j; //path index, char index
	for(i = 0, j = 0; (path[i - 1] != 0 || i == 0) && path != NULL; i++)
	{
		if(path[i] == ',' || path[i] == 0)
		{
			if(!j)
				break;

			tmp[j] = 0;
			char calls[8];
			uint8_t calls_pos;
			for(calls_pos = 0; i < j && tmp[j] != '-'; calls_pos++)
				calls[calls_pos] = tmp[i];
			calls[calls_pos] = 0;

			uint8_t ssid_val = ((tmp[calls_pos] == '-' ? tmp[++calls_pos] : tmp[--calls_pos]) - 48) & 0x7;
			if(ssid_val != 0)
				ax25_send_path(packet, calls, ssid_val, path[i] == 0);
			j = 0;
		}
		else
			tmp[j++] = path[i];
	}

	ax25_send_byte(packet, 0x03);
	ax25_send_byte(packet, 0xf0);
}

void ax25_send_path(ax25_t *packet, const char *callsign, uint8_t ssid, bool last)
{
	for(uint8_t i = 0; i < 6; i++)
	{
		if(callsign[i])
			ax25_send_byte(packet, callsign[i] << 1);
		else
			ax25_send_byte(packet, ' ' << 1);
	}

	ax25_send_byte(packet, ('0' + ssid) << 1 | (last & 0x1));
}

void ax25_send_byte(ax25_t *packet, char byte)
{
	for(uint8_t i = 0; i < 8; i++)
	{
		update_crc(packet, (byte >> 1) & 1);
		if((byte >> i) & 1)
		{
			if(packet->bit_count >= packet->max_size * 8)
				return;
			AX25_WRITE_BIT(packet->data, packet->bit_count);

			packet->bit_count++;
			packet->ones_in_a_row++;
			if(packet->ones_in_a_row < 5)
				continue;
		}
		if(packet->bit_count >= packet->max_size * 8)
			return;
		AX25_CLEAR_BIT(packet->data, packet->bit_count);
		packet->bit_count++;
		packet->ones_in_a_row = 0;
	}
	packet->byte_count++;
}

static void ax25_send_byte_no_crc(ax25_t *packet, char byte)
{
	for(uint8_t i = 0; i < 8; i++)
	{
		//update_crc(packet, (byte >> 1) & 1);
		if((byte >> i) & 1)
		{
			if(packet->bit_count >= packet->max_size * 8)
				return;
			AX25_WRITE_BIT(packet->data, packet->bit_count);

			packet->bit_count++;
			packet->ones_in_a_row++;
			if(packet->ones_in_a_row < 5)
				continue;
		}
		if(packet->bit_count >= packet->max_size * 8)
			return;
		AX25_CLEAR_BIT(packet->data, packet->bit_count);
		packet->bit_count++;
		packet->ones_in_a_row = 0;
	}
	packet->byte_count++;
}

void ax25_send_string(ax25_t *packet, const char *string)
{
	for(uint32_t i = 0; string[i]; i++)
		ax25_send_byte(packet, string[i]);
}

void ax25_send_footer(ax25_t *packet)
{
	uint16_t crc = packet->crc;
	ax25_send_byte_no_crc(packet, ~(crc & 0xff));
	crc >>= 8;
	ax25_send_byte_no_crc(packet, ~(crc & 0xff));

	packet->crc = crc;

	ax25_send_flag(packet);
}

static void update_crc(ax25_t *packet, char bit)
{
	packet->crc ^= bit;
	if(packet->crc & 1)
		packet->crc = (packet->crc >> 1) ^ 0x8408;
	else
		packet->crc = packet->crc >> 1;
}

void NRZI_encode(ax25_t *packet)
{
	uint8_t ctone = 0;
	for(uint32_t i = 0; i < packet->bit_count; i++)
	{
		if(((packet->data[i >> 3] >> (i & 0x7)) & 0x01) == 0)
			ctone = !ctone;
		if(ctone)
			AX25_WRITE_BIT(packet->data, i);
		else
			AX25_CLEAR_BIT(packet->data, i);
	}
}

static void ax25_send_flag(ax25_t *packet)
{
	unsigned char byte = 0x7E;
	for(uint32_t i = 0; i < 8; i++)
	{
		if(packet->bit_count >= packet->max_size * 8)
			return;
		if((byte >> i) & 1)
			AX25_WRITE_BIT(packet->data, packet->bit_count);
		else
			AX25_CLEAR_BIT(packet->data, packet->bit_count);
		packet->bit_count++;
	}
	packet->byte_count++;
}
