/*
 * ublox_max_m10m.h
 *
 *  Created on: 8. 4. 2025
 *      Author: vojtech
 */
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "fsl_gpio.h"
#include "fsl_lpuart.h"
#include "utils.h"

#ifndef UBLOX_MAX_M10M_H_
#define UBLOX_MAX_M10M_H_

#define NMEA_START_CHAR 	'$'
#define NMEA_END_CHAR_1 	'\r'
#define NMEA_END_CHAR_2 	'\n'

#define MAX_M10_UART_TIMEOUT    10000
#define MAX_M10_BUFFER_SIZE 	256
#define MAX_M10_CMD_TIMEOUT		500

#define LPUART_BASE 			LPUART0
#define LPUART_CLKSRC 			kCLOCK_McgIrc48MClk
#define LPUART_CLK_FREQ 		CLOCK_GetFreq(kCLOCK_McgIrc48MClk)
#define LPUART_BAUDRATE			9600

//RAM/BBR/FLASH
#define UBX_CFG_LAYER_RAM         0x01
#define UBX_CFG_LAYER_BBR         0x02
#define UBX_CFG_LAYER_FLASH       0x04

// UBX protocol message lengths
#define UBX_CFG_PRT_LEN        20
#define UBX_CFG_MSG_LEN        8
#define UBX_CFG_RATE_LEN       6
#define UBX_CFG_RST_LEN        4
#define UBX_CFG_CFG_LEN        12
#define UBX_NAV_PVT_LEN        92
#define UBX_HEADER_LEN         6
#define UBX_CHECKSUM_LEN       2

//UBX protocol constants
#define UBX_SYNC_CHAR_1 		0xB5
#define UBX_SYNC_CHAR_2 		0x62

/* NMEA sentence identifiers */
#define NMEA_GGA_ID               "$GNGGA"  /* Global Positioning System Fix Data */
#define NMEA_RMC_ID               "$GNRMC"  /* Recommended Minimum Navigation Information */

/* UBX protocol defines */
#define UBX_SYNC_CHAR_1           	0xB5
#define UBX_SYNC_CHAR_2           	0x62

/* UBX classes */
#define UBX_CLASS_RXM				0x02
#define UBX_CLASS_CFG				0x06
#define UBX_CLASS_ACK				0x05
#define UBX_CLASS_MON				0x0A

/* UBX message IDs */
#define UBX_RXM_PMREQ             	0x41
#define UBX_CFG_PMS					0x86
#define UBX_MON_VER               	0x04
#define UBX_ACK_ACK               	0x01
#define UBX_ACK_NAK               	0x00

/* UART transfer status */
#define MAX_M10_UART_RX_IDLE		0
#define MAX_M10_UART_RX_BUSY		1
#define MAX_M10_UART_RX_COMPLETE	2

/* UBX CFG-PMS power modes */
#define UBX_CFG_PMS_FULL_POWER		0x00      /* Full power mode */
#define UBX_CFG_PMS_BALANCED		0x01      /* Balanced power mode */
#define UBX_CFG_PMS_INTERVAL		0x02      /* Interval power mode */
#define UBX_CFG_PMS_AGGRESSIVE_1HZ	0x03     /* Aggressive with 1Hz */
#define UBX_CFG_PMS_AGGRESSIVE_2HZ	0x04     /* Aggressive with 2Hz */
#define UBX_CFG_PMS_AGGRESSIVE_4HZ	0x05     /* Aggressive with 4Hz */
#define UBX_CFG_PMS_PSMOO			0xFF      /* Power Save Mode */

#ifdef __cplusplus
extern "C"
{
#endif

/* MAX-M10 operating modes */
typedef enum
{
	MAX_M10_MODE_NORMAL, /* Full power, standard operation */
	MAX_M10_MODE_POWERSAVE, /* Cyclic tracking, reduced power */
	MAX_M10_MODE_SLEEP /* Minimal power consumption */
} max_m10_mode_t;

/* GPS position data structure */
typedef struct
{
	float latitude; /* Degrees, North positive */
	float longitude; /* Degrees, East positive */
	float altitude; /* Meters above mean sea level */
	uint8_t satellites; /* Number of satellites used in fix */
	bool valid_fix; /* Whether the position fix is valid */
	uint8_t fix_type; /* 0=No fix, 1=Dead reckoning, 2=2D, 3=3D, etc. */
	uint32_t timestamp; /* UTC time in milliseconds from midnight */
} max_m10_position_t;

bool max_m10_init(void);
bool max_m10_verify_comm(void);
bool max_m10_set_mode(max_m10_mode_t mode);

#ifdef __cplusplus
}
#endif

#endif /* UBLOX_MAX_M10M_H_ */
