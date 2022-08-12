/** @file

  Header of Function APIs for Elan Gen8 I2C-HID Touchscreen.

  Copyright (c) ELAN microelectronics corp. 2022, All Rights Reserved

  Module Name:
	ElanGen8TsFuncApi.h

  Environment:
	All kinds of Linux-like Platform.

********************************************************************
 Revision History

**/

#ifndef _ELAN_GEN8_TS_FUNC_API_H_
#define _ELAN_GEN8_TS_FUNC_API_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

/***************************************************
 * Definitions
 ***************************************************/

// Information Page Address
#ifndef ELAN_GEN8_INFO_ROM_FWID_MEMORY_ADDR
#define	ELAN_GEN8_INFO_ROM_FWID_MEMORY_ADDR	    0x40000
#endif //ELAN_GEN8_INFO_ROM_FWID_MEMORY_ADDR

/***************************************************
 * Macros
 ***************************************************/

// High Word
#ifndef HIGH_WORD
#define HIGH_WORD(data_integer)	((unsigned short)((data_integer & 0xFFFF0000) >> 16))
#endif //HIGH_WORD

// Low Word
#ifndef LOW_WORD
#define LOW_WORD(data_integer)	((unsigned short)(data_integer & 0x0000FFFF))
#endif //LOW_WORD

// Low Byte
#ifndef LOW_BYTE
#define LOW_BYTE(data_word)    ((unsigned char)(data_word & 0x000000FF))
#endif //LOW_BYTE

/***************************************************
 * Global Data Structure Declaration
 ***************************************************/

/***************************************************
 * Global Variables Declaration
 ***************************************************/

/***************************************************
 * Extern Variables Declaration
 ***************************************************/

/***************************************************
 * Function Prototype
 ***************************************************/

// ROM Data
int gen8_get_rom_data(unsigned int addr, unsigned char data_len, unsigned int *p_data);

// Information FWID
int gen8_read_info_fwid(unsigned short *p_info_fwid, bool recovery);

#endif //_ELAN_GEN8_TS_FUNC_API_H_
