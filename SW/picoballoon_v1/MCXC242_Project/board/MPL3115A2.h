/*
 * MPL3115A2.h
 *
 *  Created on: 10. 4. 2025
 *      Author: vojtech
 */

#ifndef MPL3115A2_H_
#define MPL3115A2_H_

#include "board.h"
#include "utils.h"
#include "i2c_ctrl.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define MPL_ADDRESS					0x60

#define MPL3115A2_STATUS                0x00
#define MPL3115A2_OUT_P_MSB             0x01
#define MPL3115A2_OUT_P_CSB             0x02
#define MPL3115A2_OUT_P_LSB             0x03
#define MPL3115A2_OUT_T_MSB             0x04
#define MPL3115A2_OUT_T_LSB             0x05
#define MPL3115A2_DR_STATUS             0x06
#define MPL3115A2_OUT_P_DELTA_MSB       0x07
#define MPL3115A2_OUT_P_DELTA_CSB       0x08
#define MPL3115A2_OUT_P_DELTA_LSB       0x09
#define MPL3115A2_OUT_T_DELTA_MSB       0x0A
#define MPL3115A2_OUT_T_DELTA_LSB       0x0B
#define MPL3115A2_WHO_AM_I              0x0C
#define MPL3115A2_F_STATUS              0x0D
#define MPL3115A2_F_DATA                0x0E
#define MPL3115A2_F_SETUP               0x0F
#define MPL3115A2_TIME_DLY              0x10
#define MPL3115A2_SYSMOD                0x11
#define MPL3115A2_INT_SOURCE            0x12
#define MPL3115A2_PT_DATA_CFG           0x13
#define MPL3115A2_BAR_IN_MSB            0x14
#define MPL3115A2_BAR_IN_LSB            0x15
#define MPL3115A2_P_TGT_MSB             0x16
#define MPL3115A2_P_TGT_LSB             0x17
#define MPL3115A2_T_TGT                 0x18
#define MPL3115A2_P_WND_MSB             0x19
#define MPL3115A2_P_WND_LSB             0x1A
#define MPL3115A2_T_WND                 0x1B
#define MPL3115A2_P_MIN_MSB             0x1C
#define MPL3115A2_P_MIN_CSB             0x1D
#define MPL3115A2_P_MIN_LSB             0x1E
#define MPL3115A2_T_MIN_MSB             0x1F
#define MPL3115A2_T_MIN_LSB             0x20
#define MPL3115A2_P_MAX_MSB             0x21
#define MPL3115A2_P_MAX_CSB             0x22
#define MPL3115A2_P_MAX_LSB             0x23
#define MPL3115A2_T_MAX_MSB             0x24
#define MPL3115A2_T_MAX_LSB             0x25
#define MPL3115A2_CTRL_REG1             0x26
#define MPL3115A2_CTRL_REG2             0x27
#define MPL3115A2_CTRL_REG3             0x28
#define MPL3115A2_CTRL_REG4             0x29
#define MPL3115A2_CTRL_REG5             0x2A
#define MPL3115A2_OFF_P                 0x2B
#define MPL3115A2_OFF_T                 0x2C
#define MPL3115A2_OFF_H                 0x2D

/* CTRL_REG1 bits */
#define MPL3115A2_CTRL_REG1_SBYB        0x01
#define MPL3115A2_CTRL_REG1_OST         0x02
#define MPL3115A2_CTRL_REG1_RST         0x04
#define MPL3115A2_CTRL_REG1_OS_MASK     0x38
#define MPL3115A2_CTRL_REG1_OS_SHIFT    3
#define MPL3115A2_CTRL_REG1_RAW         0x40
#define MPL3115A2_CTRL_REG1_ALT         0x80

/* CTRL_REG1 oversample ratios */
#define MPL3115A2_CTRL_REG1_OS_1        0x00
#define MPL3115A2_CTRL_REG1_OS_2        0x08
#define MPL3115A2_CTRL_REG1_OS_4        0x10
#define MPL3115A2_CTRL_REG1_OS_8        0x18
#define MPL3115A2_CTRL_REG1_OS_16       0x20
#define MPL3115A2_CTRL_REG1_OS_32       0x28
#define MPL3115A2_CTRL_REG1_OS_64       0x30
#define MPL3115A2_CTRL_REG1_OS_128      0x38

/* PT_DATA_CFG bits */
#define MPL3115A2_PT_DATA_CFG_PDEFE     0x01
#define MPL3115A2_PT_DATA_CFG_TDEFE     0x02
#define MPL3115A2_PT_DATA_CFG_DREM      0x04

/* STATUS bits */
#define MPL3115A2_STATUS_PTDR           0x08
#define MPL3115A2_STATUS_PDR            0x04
#define MPL3115A2_STATUS_TDR            0x02

//who am I value
#define MPL3115A2_WHO_AM_I_VALUE     	0xC4

//control reg bits
#define MPL3115A2_CTRL_REG1_OS(x)    	(((x) & 0x7U) << 3) /* Oversample ratio */

typedef enum _mpl3115a2_mode
{
	kMPL3115A2_barometer = 0U, //Altitude measurement mode
	kMPL3115A2_altitude = 1U //Pressure measurement mode
} mpl3115a2_mode_t;

typedef enum _mpl3115a2_oversample
{
	kMPL3115A2_OS_1 = 0U,
	kMPL3115A2_OS_2 = 1U,
	kMPL3115A2_OS_4 = 2U,
	kMPL3115A2_OS_8 = 3U,
	kMPL3115A2_OS_16 = 4U,
	kMPL3115A2_OS_32 = 5U,
	kMPL3115A2_OS_64 = 6U,
	kMPL3115A2_OS_128 = 7U
} mpl3115a2_oversample_t;

typedef struct _mpl3115a2_data
{
	union
	{
		float altitude; //Altitude in meters
		float pressure; //Pressure in pascals
	};
	float temperature; //Temperature in degrees Celsius
} mpl3115a2_data_t;

bool MPL_init(void);
bool MPL3115A2_set_standby_mode(void);
bool MPL3115A2_set_active_mode(void);
bool MPL_one_shot_measurement(mpl3115a2_data_t *data, mpl3115a2_mode_t mode);

#ifdef __cplusplus
}
#endif

#endif /* MPL3115A2_H_ */
