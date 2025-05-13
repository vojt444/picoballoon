/*
 * aprs.h
 *
 *  Created on: 30. 3. 2025
 *      Author: vojtech
 */

#ifndef APRS_H_
#define APRS_H_

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include "ublox_max_m10m.h"
#include "ax25.h"
#include "utils.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define APRS_DEST_CALLSIGN		"APRS"
#define APRS_DEST_SSID			0
#define APRS_FREQUENCY_DEFAULT	144800000UL
#define CALLSIGN_LEN			6

#define METER_TO_FEET(m) 		(((m)*26876)/8192)

typedef enum
{
	TEL_SATS,
	TEL_TTFF,
	TEL_VBAT,
	TEL_VSOL,
	TEL_PBAT,
	TEL_PRESS,
	TEL_TEMP,
	TEL_HUM
} telemetry_t;

typedef struct
{
	char callsign[16];
	uint8_t ssid;
	uint16_t symbol;
	char path[32];
	uint16_t preamble;
	telemetry_t tel[5];
	bool tel_enc;
	uint16_t tel_enc_cycle;
	char tel_comment[64];
} aprs_conf_t;

void aprs_encode_init(ax25_t *packet, uint8_t *data, uint16_t data_size, mod_t mod);
void aprs_encode_message(ax25_t *packet, const aprs_conf_t *config, const char *receiver, const char *text);
void aprs_encode_full_data_packet(ax25_t *packet, const aprs_conf_t *config, measured_data_t *data);
void aprs_encode_test_packet(ax25_t *packet, const aprs_conf_t *config);

#ifdef __cplusplus
}
#endif

#endif /* APRS_H_ */
