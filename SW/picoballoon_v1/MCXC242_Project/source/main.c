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
#include <stdio.h>
#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "fsl_debug_console.h"
#include "fsl_adc16.h"

#include "MCP9802.h"
/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define BOARD_LED_GPIO GPIOE
#define BOARD_LED_PIN 29U

#define ADC_CHANNEL 0U
#define ADC_REF_VOLTAGE 3.3F
#define ADC_RESOLUTION (1U << 12)
/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*******************************************************************************
 * Variables
 ******************************************************************************/
volatile uint32_t g_systick_counter;
volatile bool g_ADC_conversion_done_flag = false;
volatile uint32_t g_ADC_value = 0;
/*******************************************************************************
 * Code
 ******************************************************************************/

void SysTick_Handler(void)
{
	if(g_systick_counter != 0U)
		g_systick_counter--;
}

void ADC0_IRQHandler(void)
{
	g_ADC_value = ADC16_GetChannelConversionValue(ADC0, ADC_CHANNEL);
	g_ADC_conversion_done_flag = true;
}

//delay in milliseconds
void delay(uint32_t n)
{
	g_systick_counter = n;
	while(g_systick_counter != 0U);
}

int main(void)
{
	//basic init
	BOARD_InitBootPins();
	BOARD_InitBootClocks();
	BOARD_InitBootPeripherals();

	//adc variables
	adc16_config_t ADC_config;
	adc16_channel_config_t ADC_channel_config;
	float voltage = 0;

	//adc init
	ADC16_GetDefaultConfig(&ADC_config);
	ADC_config.enableLowPower = true;
	ADC16_Init(ADC0, &ADC_config);
	ADC16_EnableHardwareTrigger(ADC0, false);
	EnableIRQ(ADC0_IRQn);

	ADC_channel_config.channelNumber = 0;
	ADC_channel_config.enableDifferentialConversion = false;
	ADC_channel_config.enableInterruptOnConversionCompleted = true;

	//temp sensor vars
	float temperature = 0;
	//I2C for temp sensor init
	MCP9802_init();

	//read the temperature
	MCP9802_read_temperature_oneshot(&temperature);

	if(SysTick_Config(SystemCoreClock / 1000U))
	{
		while(1);
	}

	for(int i = 0; i < 9; ++i)
	{
		delay(300U);
		GPIO_PortToggle(BOARD_LED_GPIO, 1u << BOARD_LED_PIN);
	}

	while(1)
	{
		MCP9802_read_temperature_oneshot(&temperature);
//
//		ADC16_SetChannelConfig(ADC0, ADC_CHANNEL, &ADC_channel_config);
//
//		if(g_ADC_conversion_done_flag)
//		{
//			g_ADC_conversion_done_flag = false;
//			voltage = 2 * ((float)g_ADC_value / 4096) * 3.3;
//		}
		delay(100);

	}
}
