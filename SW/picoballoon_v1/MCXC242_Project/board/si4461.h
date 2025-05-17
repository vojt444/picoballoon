/*
 * si4461.h
 *
 *  Created on: 24. 3. 2025
 *      Author: vojtech
 */
#ifndef SI4461_H_
#define SI4461_H_

#include "radio_config.h"
#include <string.h>
#include <stdbool.h>
#include "fsl_spi.h"
#include "clock_config.h"
#include "fsl_gpio.h"
#include "utils.h"
#include "si4461_desc.h"
#include "aprs.h"

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

#define CTS_TIMEOUT					5000
#define TIMEOUT						10000

#define XO_FREQ						30000000UL
#define OUTDIV						24U
#define RF_DEV_HZ					1300.0f
#define FDEV_APRS					((((uint32_t)1 << 19) * OUTDIV * RF_DEV_HZ)/(2*XO_FREQ))
#define F_INT_2M					(2 * XO_FREQ / 24)
#define FDIV_INTE_2M				((144800000 / F_INT_2M) - 1)
#define FDIV_FRAC_2M				((144800000 - F_INT_2M * (int)FDIV_INTE_2M)*((uint32_t)1 << 19)) / F_INT_2M

typedef struct
{
	uint8_t chip_rev;
	uint16_t part;
	uint8_t part_build;
	uint16_t id;
	uint8_t customer;
	uint8_t rom_id;
} si4461_info_t;

typedef struct
{
	uint8_t rx_fifo_count;
	uint8_t tx_fifo_count;
} si4461_fifo_info_t;

bool Si4461_init(uint32_t frequency);
bool Si4461_get_info(si4461_info_t *info);
bool Si4461_set_properties(uint16_t start_property, uint8_t length,
						   uint8_t *params);
bool Si4461_get_properties(uint16_t start_property, uint8_t length,
						   uint8_t *params);
bool Si4461_get_state(uint8_t *state);
bool Si4461_set_state(uint8_t new_state);
bool Si4461_NOP(void);
bool Si4461_gpio_cfg(uint8_t *data);
bool Si4461_set_tx_power(uint8_t power);
bool Si4461_freq_offset(uint16_t offset);
bool Si4461_deviation(uint32_t deviation);
bool Si4461_freq_control(uint8_t inte, uint32_t frac);
bool Si4461_set_freq(uint32_t freq);
bool Si4461_fifo_reset(void);
bool Si4461_fifo_info(si4461_fifo_info_t *fifo_info);
bool Si4461_fifo_write(uint8_t *packet, size_t packet_length);
bool Si4461_send_packet(uint8_t *packet, size_t packet_length);
bool Si4461_start_TX(uint8_t *tx_data);
bool Si4461_stop_TX(void);
void Si4461_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* SI4461_H_ */
