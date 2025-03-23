/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _BOARD_H_
#define _BOARD_H_

#include "clock_config.h"
#include "fsl_gpio.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/* The board name */
#define BOARD_NAME "FRDM-MCXC242"

/* The LPUART to use for debug messages. */
#define BOARD_DEBUG_UART_TYPE     kSerialPort_Uart
#define BOARD_DEBUG_UART_BASEADDR (uint32_t) LPUART0
#define BOARD_DEBUG_UART_INSTANCE 0U
#define BOARD_DEBUG_UART_CLKSRC   kCLOCK_McgIrc48MClk
#define BOARD_DEBUG_UART_CLK_FREQ CLOCK_GetPeriphClkFreq()
#define BOARD_UART_IRQ            LPUART0_IRQn
#define BOARD_UART_IRQ_HANDLER    LPUART0_IRQHandler

#ifndef BOARD_DEBUG_UART_BAUDRATE
#define BOARD_DEBUG_UART_BAUDRATE 115200
#endif /* BOARD_DEBUG_UART_BAUDRATE */

/* Define the port macros for the board switches */
#ifndef BOARD_SW1_GPIO
#define BOARD_SW1_GPIO GPIOA
#endif
#ifndef BOARD_SW1_PORT
#define BOARD_SW1_PORT PORTA
#endif
#ifndef BOARD_SW1_GPIO_PIN
#define BOARD_SW1_GPIO_PIN 4U
#endif
#define BOARD_SW1_IRQ         PORTA_IRQn
#define BOARD_SW1_IRQ_HANDLER PORTA_IRQHandler
#define BOARD_SW1_NAME        "SW1"

#ifndef BOARD_SW2_GPIO
#define BOARD_SW2_GPIO GPIOC
#endif
#ifndef BOARD_SW2_PORT
#define BOARD_SW2_PORT PORTC
#endif
#ifndef BOARD_SW2_GPIO_PIN
#define BOARD_SW2_GPIO_PIN 1U
#endif
#define BOARD_SW2_IRQ         PORTB_PORTC_PORTD_PORTE_IRQn
#define BOARD_SW2_IRQ_HANDLER PORTB_PORTC_PORTD_PORTE_IRQHandler
#define BOARD_SW2_NAME        "SW2"

#define LLWU_SW_GPIO        BOARD_SW2_GPIO
#define LLWU_SW_PORT        BOARD_SW2_PORT
#define LLWU_SW_GPIO_PIN    BOARD_SW2_GPIO_PIN
#define LLWU_SW_IRQ         BOARD_SW2_IRQ
#define LLWU_SW_IRQ_HANDLER BOARD_SW2_IRQ_HANDLER
#define LLWU_SW_NAME        BOARD_SW2_NAME

/* Board led color mapping */
#define LOGIC_LED_ON  0U
#define LOGIC_LED_OFF 1U
#ifndef BOARD_LED_RED_GPIO
#define BOARD_LED_RED_GPIO GPIOB
#endif
#define BOARD_LED_RED_GPIO_PORT PORTB
#ifndef BOARD_LED_RED_GPIO_PIN
#define BOARD_LED_RED_GPIO_PIN 18U
#endif
#ifndef BOARD_LED_GREEN_GPIO
#define BOARD_LED_GREEN_GPIO GPIOB
#endif
#define BOARD_LED_GREEN_GPIO_PORT PORTB
#ifndef BOARD_LED_GREEN_GPIO_PIN
#define BOARD_LED_GREEN_GPIO_PIN 19U
#endif
#ifndef BOARD_LED_BLUE_GPIO
#define BOARD_LED_BLUE_GPIO GPIOA
#endif
#define BOARD_LED_BLUE_GPIO_PORT PORTA
#ifndef BOARD_LED_BLUE_GPIO_PIN
#define BOARD_LED_BLUE_GPIO_PIN 13U
#endif

#define BOARD_ARDUINO_INT_IRQ   (PORTB_PORTC_PORTD_PORTE_IRQn)
#define BOARD_ARDUINO_I2C_IRQ   (I2C1_IRQn)
#define BOARD_ARDUINO_I2C_INDEX (1)

#define LED_RED_INIT(output)                                           \
    GPIO_PinWrite(BOARD_LED_RED_GPIO, BOARD_LED_RED_GPIO_PIN, output); \
    BOARD_LED_RED_GPIO->PDDR |= (1U << BOARD_LED_RED_GPIO_PIN)                         /*!< Enable target LED_RED */
#define LED_RED_ON()  GPIO_PortClear(BOARD_LED_RED_GPIO, 1U << BOARD_LED_RED_GPIO_PIN) /*!< Turn on target LED_RED */
#define LED_RED_OFF() GPIO_PortSet(BOARD_LED_RED_GPIO, 1U << BOARD_LED_RED_GPIO_PIN)   /*!< Turn off target LED_RED */
#define LED_RED_TOGGLE() \
    GPIO_PortToggle(BOARD_LED_RED_GPIO, 1U << BOARD_LED_RED_GPIO_PIN)                  /*!< Toggle on target LED_RED */

#define LED_GREEN_INIT(output)                                             \
    GPIO_PinWrite(BOARD_LED_GREEN_GPIO, BOARD_LED_GREEN_GPIO_PIN, output); \
    BOARD_LED_GREEN_GPIO->PDDR |= (1U << BOARD_LED_GREEN_GPIO_PIN)        /*!< Enable target LED_GREEN */
#define LED_GREEN_ON() \
    GPIO_PortClear(BOARD_LED_GREEN_GPIO, 1U << BOARD_LED_GREEN_GPIO_PIN)  /*!< Turn on target LED_GREEN */
#define LED_GREEN_OFF() \
    GPIO_PortSet(BOARD_LED_GREEN_GPIO, 1U << BOARD_LED_GREEN_GPIO_PIN)    /*!< Turn off target LED_GREEN */
#define LED_GREEN_TOGGLE() \
    GPIO_PortToggle(BOARD_LED_GREEN_GPIO, 1U << BOARD_LED_GREEN_GPIO_PIN) /*!< Toggle on target LED_GREEN */

#define LED_BLUE_INIT(output)                                            \
    GPIO_PinWrite(BOARD_LED_BLUE_GPIO, BOARD_LED_BLUE_GPIO_PIN, output); \
    BOARD_LED_BLUE_GPIO->PDDR |= (1U << BOARD_LED_BLUE_GPIO_PIN)        /*!< Enable target LED_BLUE */
#define LED_BLUE_ON()                                                                                \
    GPIO_PortClear(BOARD_LED_BLUE_GPIO, 1U << BOARD_LED_BLUE_GPIO_PIN)  /*!< Turn on target LED_BLUE \
                                                                         */
#define LED_BLUE_OFF()                                                                                \
    GPIO_PortSet(BOARD_LED_BLUE_GPIO, 1U << BOARD_LED_BLUE_GPIO_PIN)    /*!< Turn off target LED_BLUE \
                                                                         */
#define LED_BLUE_TOGGLE() \
    GPIO_PortToggle(BOARD_LED_BLUE_GPIO, 1U << BOARD_LED_BLUE_GPIO_PIN) /*!< Toggle on target LED_BLUE */

#define BOARD_ACCEL_I2C_BASEADDR   I2C1
#define BOARD_ACCEL_I2C_CLOCK_FREQ CLOCK_GetFreq(I2C1_CLK_SRC)

/* ERPC SPI configuration */
#define ERPC_BOARD_SPI_SLAVE_READY_USE_GPIO (1)
#define ERPC_BOARD_SPI_BASEADDR             SPI0
#define ERPC_BOARD_SPI_BAUDRATE             500000U
#define ERPC_BOARD_SPI_CLKSRC               SPI0_CLK_SRC
#define ERPC_BOARD_SPI_CLK_FREQ             CLOCK_GetFreq(SPI0_CLK_SRC)
#define ERPC_BOARD_SPI_INT_GPIO             GPIOB
#define ERPC_BOARD_SPI_INT_PIN              1U
#define ERPC_BOARD_SPI_INT_PIN_IRQ          PORTB_PORTC_PORTD_PORTE_IRQn
#define ERPC_BOARD_SPI_INT_PIN_IRQ_HANDLER  PORTB_PORTC_PORTD_PORTE_IRQHandler

/* Board accelerometer driver */
#define BOARD_ACCEL_MMA

/* Serial MWM Wi-Fi */
#define BOARD_SERIAL_MWM_PORT_CLK_FREQ     CLOCK_GetFreq(kCLOCK_McgPeriphClk)
#define BOARD_SERIAL_MWM_PORT              LPUART1
#define BOARD_SERIAL_MWM_PORT_IRQn         LPUART1_IRQn
#define BOARD_SERIAL_MWM_RST_GPIO          GPIOE
#define BOARD_SERIAL_MWM_RST_PIN           30U
#define BOARD_SERIAL_MWM_RST_WRITE(output) GPIO_PinWrite(BOARD_SERIAL_MWM_RST_GPIO, BOARD_SERIAL_MWM_RST_PIN, output)

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

/*******************************************************************************
 * API
 ******************************************************************************/

void BOARD_InitDebugConsole(void);

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif /* _BOARD_H_ */
