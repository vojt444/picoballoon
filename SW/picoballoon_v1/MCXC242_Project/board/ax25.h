/*
 * ax25.h
 *
 *  Created on: 3. 5. 2025
 *      Author: vojtech
 */

#ifndef AX25_H_
#define AX25_H_

#include <string.h>
#include "utils.h"
#include "aprs.h"

#ifdef __cplusplus
extern "C"
{
#endif

void ax25_init(ax25_t *packet);
void ax25_send_header(ax25_t *packet, const char *callsign, uint8_t ssid, const char *path);
void ax25_send_path(ax25_t *packet, const char *callsign, uint8_t ssid, bool last);
void ax25_send_byte(ax25_t *packet, char byte);
void ax25_send_string(ax25_t *packet, const char *string);
void ax25_send_footer(ax25_t *packet);
void NRZI_encode(ax25_t *packet);

#ifdef __cplusplus
}
#endif

#endif /* AX25_H_ */
