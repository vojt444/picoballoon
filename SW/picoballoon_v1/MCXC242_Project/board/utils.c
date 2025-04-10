/*
 * utils.c
 *
 *  Created on: 24. 3. 2025
 *      Author: vojtech
 */
#include "utils.h"

volatile uint32_t g_systick_counter;

void SysTick_Handler(void)
{
	if(g_systick_counter != 0U)
		g_systick_counter--;
}

void delay(uint32_t n)
{
	g_systick_counter = n;
	while(g_systick_counter != 0U);
}

void pad_string(char *str, size_t desired_length)
{
	size_t current_length = strlen(str);

	if(current_length < desired_length)
	{
		memset(str + current_length, ' ', desired_length - current_length);
		str[desired_length] = '\0';
	}
	else if(current_length > desired_length)
	{
		str[desired_length] = '\0';
	}
}

void uint16_to_little_endian(uint16_t value, uint8_t *buffer)
{
	buffer[0] = (uint8_t)(value & 0xFF);
	buffer[1] = (uint8_t)((value >> 8) & 0xFF);
}
