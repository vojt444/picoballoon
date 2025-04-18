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

void delay(uint32_t n);
void pad_string(char *str, size_t desired_length);
void uint16_to_little_endian(uint16_t value, uint8_t *buffer);


#ifdef __cplusplus
}
#endif

#endif /* UTILS_H_ */
