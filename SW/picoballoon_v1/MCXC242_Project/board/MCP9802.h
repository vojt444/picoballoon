/*
 * MCP9802.h
 *
 *  Created on: 23. 3. 2025
 *      Author: vojtech
 */

#ifndef MCP9802_H_
#define MCP9802_H_

#include <i2c_ctrl.h>
#include "board.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MCP9802_REG_TEMP 		0x00U
#define MCP9802_REG_CONFIG		0x01U
#define MCP9802_REG_THYST		0x02U
#define MCP9802_REG_TSET		0x03U

#define MCP9802_ADDRESS			0x48U

#define MCP9802_INIT_CONFIG	 	0x21U //0b0010_0001 init config suitable for picoballoon project
#define MCP9802_ONESHOT_READ 	0xA1U

void MCP9802_init(void);
bool MCP9802_read_temperature_oneshot(float *temperature);
bool MCP9802_set_config(uint8_t config);
bool MCP9802_read_config(uint8_t *config);

#ifdef __cplusplus
}
#endif

#endif /* MCP9802_H_ */
