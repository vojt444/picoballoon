/*
 * utils.h
 *
 *  Created on: 24. 3. 2025
 *      Author: vojtech
 */

#ifndef UTILS_H_
#define UTILS_H_

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
	float latitude;
	float longitude;
	float altitude;
} gnss_position_t;

typedef struct
{
	gnss_position_t position;
	float temperature;
	float pressure;
	float humidity;
} measured_data_t;

typedef enum
{
	MOD_NOT_SET,
	MOD_OOK,
	MOD_2FSK,
	MOD_2GFSK,
	MOD_AFSK
} mod_t;

typedef struct
{
	uint8_t ones_in_a_row;
	uint8_t *data;
	uint16_t bit_count;
	uint16_t byte_count;
	uint16_t max_size;
	uint16_t crc;
	mod_t mod;
} ax25_t;

void delay(uint32_t n);
void pad_string(char *str, size_t desired_length);
void uint16_to_little_endian(uint16_t value, uint8_t *buffer);
void uint32_to_little_endian(uint32_t value, uint8_t *buffer);

#ifdef __cplusplus
}
#endif

#endif /* UTILS_H_ */
