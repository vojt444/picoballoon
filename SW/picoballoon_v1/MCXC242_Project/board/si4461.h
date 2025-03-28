/*
 * si4461.h
 *
 *  Created on: 24. 3. 2025
 *      Author: vojtech
 */
#ifndef SI4461_H_
#define SI4461_H_

#include <string.h>
#include "fsl_spi.h"
#include "board.h"
#include "utils.h"
#include "radio_config.h"
#include "si4461_desc.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define SPI_BASE 					SPI0
#define SPI_CLOCK_SOURCE     		CLOCK_GetFreq(kCLOCK_BusClk)
#define SPI_BUFFER_SIZE 			64U

#define SI4461_SDN_GPIO				GPIOC
#define SI4461_SDN_PIN				3U
#define SI4461_SDN_PIN_MASK			(1 << SI4461_SDN_PIN)
#define SI4461_CS_GPIO				GPIOC
#define SI4461_CS_PIN				4U
#define SI4461_CS_PIN_MASK			(1 << SI4461_CS_PIN)

typedef struct
{
	uint8_t chip_rev;
	uint16_t part;
	uint8_t part_build;
	uint16_t id;
	uint8_t customer;
	uint8_t rom_id;
} si4461_info_t;

typedef enum
{
	SI4461_STATE_NO_CHANGE 	= 0,
	SI4461_STATE_SLEEP 		= 1,
	SI4461_STATE_SPI_ACTIVE = 2,
	SI4461_STATE_READY 		= 3,
	SI4461_STATE_READY2 	= 4,
	SI4461_STATE_TUNE_TX 	= 5,
	SI4461_STATE_TUNE_RX 	= 6,
	SI4461_STATE_TX 		= 7,
	SI4461_STATE_RX 		= 8
} si4461_state_t;

bool Si4461_init(void);
void Si4461_get_info(si4461_info_t *info);


#ifdef __cplusplus
}
#endif

#endif /* SI4461_H_ */
