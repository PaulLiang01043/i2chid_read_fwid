/** @file

  Header of EDID Utility for Elan I2C-HID Touchscreen.

  Copyright (c) ELAN microelectronics corp. 2022, All Rights Reserved

  Module Name:
	ElanTsEdidUtility.h

  Environment:
	All kinds of Linux-like Platform.

********************************************************************
 Revision History

**/

#ifndef _ELAN_TS_EDID_UTILITY_H_
#define _ELAN_TS_EDID_UTILITY_H_

/*******************************************
 * Definitions
 ******************************************/

// Path of modetest command
#ifndef MODETEST_CMD
#define MODETEST_CMD			"/usr/bin/modetest -a"
#endif //MODETEST_CMD

// EDID Header
#ifndef STANDARD_EDID_HEADER
#define STANDARD_EDID_HEADER	"00ffffffffffff00"
#endif //STANDARD_EDID_HEADER

#ifndef AUO_EDID_HEADER
#define AUO_EDID_HEADER			"b36f0200b004ec04"
#endif //AUO_EDID_HEADER

#ifndef BOE_EDID_HEADER
#define BOE_EDID_HEADER			"ac700200b0040005"
#endif //BOE_EDID_HEADER

/*******************************************
 * Global Data Structure Declaration
 ******************************************/

/*******************************************
 * Global Variables Declaration
 ******************************************/

// Debug
extern bool g_debug;

#ifndef DEBUG_PRINTF
#define DEBUG_PRINTF(fmt, argv...) if(g_debug) printf(fmt, ##argv)
#endif //DEBUG_PRINTF

#ifndef ERROR_PRINTF
#define ERROR_PRINTF(fmt, argv...) fprintf(stderr, fmt, ##argv)
#endif //ERROR_PRINTF

/*******************************************
 * Extern Variables Declaration
 ******************************************/

/*******************************************
 * Function Prototype
 ******************************************/

// EDID Info.
int get_edid_manufacturer_product_code(unsigned short *p_manufacturer_code, unsigned short *p_product_code);
int show_edid_manufacturer_product_code(unsigned short manufacturer_code, unsigned short product_code);

#endif //_ELAN_TS_EDID_UTILITY_H_
