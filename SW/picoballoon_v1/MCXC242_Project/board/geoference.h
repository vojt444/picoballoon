/*
 * geoference.h
 *
 *  Created on: 8. 5. 2025
 *      Author: vojtech
 */

#ifndef GEOFERENCE_H_
#define GEOFERENCE_H_

#define APRS_DEFAULT_FREQ 	144800000

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stdint.h>

extern uint32_t g_APRS_frequency;
extern bool g_APRS_no_tx;

bool point_in_polygon(uint32_t num_of_poly_corners, float *polygon, float latitude, float longitude);
void set_APRS_freq(float latitude, float longitude);

#ifdef __cplusplus
}
#endif

#endif /* GEOFERENCE_H_ */
