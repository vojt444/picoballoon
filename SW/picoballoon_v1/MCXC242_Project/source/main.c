/*
 * Copyright 2016-2025 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file    MCXC242_Project.c
 * @brief   Application entry point.
 */
#include <i2c_ctrl.h>
#include <stdio.h>
//#include "board.h"
#include "utils.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "fsl_adc16.h"
#include "fsl_lpuart.h"
#include "fsl_clock.h"
#include "fsl_cop.h"

#include "i2c_ctrl.h"
#include "ublox_max_m10m.h"
#include "tpm_timer.h"
#include "MCP9802.h"
#include "MPL3115A2.h"
#include "htu21d.h"
#include "si4461.h"
#include "aprs.h"
#include "sleep.h"
#include "geoference.h"
/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define DEBUGING				1

#define BOARD_LED_GPIO 		GPIOE
#define BOARD_LED_PIN 		29U

#define GNSS_EN_PORT		GPIOD
#define GNSS_EN_PIN			4U

#define ADC_CHANNEL 		0U
#define ADC_REF_VOLTAGE 	3.3F
#define ADC_RESOLUTION 		(1U << 12)

#define SLEEP_TIME			30U //30 minutes
#define VOLTAGE_THRESHOLD	0.5
/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*******************************************************************************
 * Variables
 ******************************************************************************/
volatile bool g_ADC_conversion_done_flag = false;
volatile uint32_t g_ADC_value = 0;
volatile bool g_watchdog_enabled = false;

bool si4461_inited = false;
bool max_m10_inited = false;

uint32_t g_APRS_frequency = 144800000;
bool g_APRS_no_tx = false;
/*******************************************************************************
 * Code
 ******************************************************************************/
void enable_low_power_mode(uint32_t seconds);
float ADC_measure(void);

void ADC0_IRQHandler(void)
{
	g_ADC_value = ADC16_GetChannelConversionValue(ADC0, ADC_CHANNEL);
	g_ADC_conversion_done_flag = true;
}

int main(void)
{
	//basic init
	BOARD_InitBootPins();
	BOARD_InitBootClocks();
	BOARD_InitBootPeripherals();
	SMC_SetPowerModeProtection(SMC, kSMC_AllowPowerModeAll);
	I2C_init();
	tpm_timer_init();
	delay_us_init();

	for(uint8_t i = 0; i < 5; i++)
	{
		GPIO_PortClear(BOARD_LED_GPIO, 1u << BOARD_LED_PIN);
		delay_us(833);
		GPIO_PortSet(BOARD_LED_GPIO, 1u << BOARD_LED_PIN);
		delay_us(833);
	}

	//watchdog
	cop_config_t config_cop;
	COP_GetDefaultConfig(&config_cop);
	COP_Init(SIM, &config_cop);
	g_watchdog_enabled = true;

	//adc variables
	adc16_config_t ADC_config;
	float voltage = 0;

	//adc init
	if(g_watchdog_enabled)
		COP_Refresh(SIM);
	ADC16_GetDefaultConfig(&ADC_config);
	ADC_config.enableLowPower = true;
	ADC_config.hardwareAverageMode = kADC16_HardwareAverageCount16;
	ADC16_Init(ADC0, &ADC_config);
	ADC16_EnableHardwareTrigger(ADC0, false);
	EnableIRQ(ADC0_IRQn);
	if(kStatus_Success != ADC16_DoAutoCalibration(ADC0))
		voltage = 99;

	SysTick_Config(SystemCoreClock / 1000U);

	//init Si4461
	if(g_watchdog_enabled)
		COP_Refresh(SIM);
	si4461_info_t si4461_info = {0};
	if(Si4461_init(g_APRS_frequency))
		si4461_inited = true;
	Si4461_get_info(&si4461_info);

//GNSS
	if(g_watchdog_enabled)
		COP_Refresh(SIM);
	gnss_position_t pos = {0};
	if(max_m10_init())
		max_m10_inited = true;
	delay_ms(100);
	if(max_m10_get_pos(&pos) && si4461_inited)
	{
		set_APRS_freq(pos.latitude, pos.longitude);
		Si4461_set_freq(g_APRS_frequency);
	}
//humidity
	if(g_watchdog_enabled)
		COP_Refresh(SIM);
	float rh = 0;
	HTU21D_init();
	HTU21D_get_humidity(&rh);

//temperature
	if(g_watchdog_enabled)
		COP_Refresh(SIM);
	float temperature = 0;
	MCP9802_init();
	MCP9802_read_temperature_oneshot(&temperature);

//pressure
	if(g_watchdog_enabled)
		COP_Refresh(SIM);
	mpl3115a2_data_t mpl_data;
	MPL_init();
	MPL_one_shot_measurement(&mpl_data, kMPL3115A2_barometer);

	//APRS
	if(g_watchdog_enabled)
		COP_Refresh(SIM);
	ax25_t packet = {0};
	aprs_conf_t aprs_config;
	strncpy(aprs_config.callsign, "OK4VP", sizeof(aprs_config.callsign));
	aprs_config.ssid = 11;
	strncpy(aprs_config.path, "WIDE1-1,WIDE2-1", sizeof(aprs_config.path));
	aprs_config.preamble = 0;

	uint8_t buffer[128] = {0};
	packet.data = buffer;
	packet.max_size = sizeof(buffer);
	packet.mod = MOD_2GFSK;

	measured_data_t all_data = {0};
	all_data.position = pos;
	all_data.temperature = temperature;
	all_data.humidity = rh;
	all_data.pressure = mpl_data.pressure;

	//make APRS packet
	if(g_watchdog_enabled)
		COP_Refresh(SIM);
	ax25_init(&packet);
	aprs_encode_full_data_packet(&packet, &aprs_config, &all_data);
//	aprs_encode_test_packet(&packet, &aprs_config);
//	NRZI_encode(&packet);

//send APRS packet
	if(g_watchdog_enabled)
		COP_Refresh(SIM);
	if(si4461_inited)
	{
		set_APRS_freq(pos.latitude, pos.longitude);
		Si4461_set_freq(g_APRS_frequency);
	}

	bell202_init_timer();
	aprs_send_packet(&packet);
	Si4461_shutdown();

	while(1)
	{
		//enable_low_power_mode(SLEEP_TIME);
		for(uint8_t i = 0; i < 3; i++)
		{
			GPIO_PortClear(BOARD_LED_GPIO, 1u << BOARD_LED_PIN);
			delay_ms(1000);
			GPIO_PortSet(BOARD_LED_GPIO, 1u << BOARD_LED_PIN);
			delay_ms(1000);
		}

		if(g_watchdog_enabled)
			COP_Refresh(SIM);
		voltage = ADC_measure();
		if(voltage > VOLTAGE_THRESHOLD)
		{
			//temperature
			if(g_watchdog_enabled)
				COP_Refresh(SIM);
			if(!MCP9802_read_temperature_oneshot(&temperature))
			{
				delay_ms(5);
				if(g_watchdog_enabled)
					COP_Refresh(SIM);
				MCP9802_init();
				if(!MCP9802_read_temperature_oneshot(&temperature))
					temperature = 0;
			}
			//pressure
			if(g_watchdog_enabled)
				COP_Refresh(SIM);
			if(!MPL_one_shot_measurement(&mpl_data, kMPL3115A2_barometer))
			{
				delay_ms(5);
				if(g_watchdog_enabled)
					COP_Refresh(SIM);
				MPL_init();
				if(!MPL_one_shot_measurement(&mpl_data, kMPL3115A2_barometer))
					mpl_data.pressure = 0;
			}
			//humidity
			if(!HTU21D_get_humidity(&rh))
			{

				HTU21D_init();
				if(!HTU21D_get_humidity(&rh))
					rh = 0;
			}
			//position
			if(!max_m10_get_pos(&pos))
			{
				if(!max_m10_init())
					max_m10_inited = false;
				if(!max_m10_get_pos(&pos))
				{
					pos.altitude = 0;
					pos.latitude = 0;
					pos.longitude = 0;
				}
			}
			if(si4461_inited)
			{
				set_APRS_freq(pos.latitude, pos.longitude);
				Si4461_set_freq(g_APRS_frequency);
			}
		}
		all_data.humidity = rh;
		all_data.position = pos;
		all_data.pressure = mpl_data.pressure;
		all_data.temperature = temperature;
		ax25_init(&packet);
		aprs_encode_full_data_packet(&packet, &aprs_config, &all_data);
		NRZI_encode(&packet);
		voltage = ADC_measure();
		/*if(voltage > VOLTAGE_THRESHOLD)
		 Si4461_send_packet(packet.data, packet.byte_count);*/
	}
}

void enable_low_power_mode(uint32_t seconds)
{
	Si4461_shutdown();
	si4461_inited = false;
	max_m10_set_mode(MAX_M10_MODE_PSMOO);
	BOARD_InitPins_deinit();
	if(g_watchdog_enabled)
		COP_Refresh(SIM);
#if DEBUGING == 1
	delay_ms(seconds * 1000);
#elif DEBUGING == 0
	SLEEP_enter(seconds);
#endif
	if(g_watchdog_enabled)
		COP_Refresh(SIM);
	BOARD_InitPins();
	if(Si4461_init(144780000))
		si4461_inited = true;
}

float ADC_measure(void)
{
	adc16_channel_config_t ADC_channel_config;
	ADC_channel_config.channelNumber = 0;
	ADC_channel_config.enableDifferentialConversion = false;
	ADC_channel_config.enableInterruptOnConversionCompleted = true;

	g_ADC_conversion_done_flag = false;
	ADC16_SetChannelConfig(ADC0, 0U, &ADC_channel_config);

	uint32_t g_ADC_timeout = 10000;
	while(!g_ADC_conversion_done_flag && g_ADC_timeout--)
	{
		if(g_ADC_timeout == 0)
		{
			g_ADC_value = 0;
			break;
		}
	}
	return (2 * (((float)g_ADC_value * 3.3) / ((1 << 12U) - 1)));
}
