/*
 * aprs.h
 *
 *  Created on: 30. 3. 2025
 *      Author: vojtech
 */

#ifndef APRS_H_
#define APRS_H_

#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define CALLSIGN_LEN	6

void ax25_encode_callsign(char *callsign, uint8_t ssid, uint8_t *encoded);
uint16_t ax25_fcs(const uint8_t *data, size_t length);
void create_aprs_packet(uint8_t *packet, size_t *packet_len, char *src,
						uint8_t src_ssid, char *dst, uint8_t dst_ssid,
						char *digipeaters[][2], int digi_count, char *payload);

#ifdef __cplusplus
}
#endif

#endif /* APRS_H_ */
