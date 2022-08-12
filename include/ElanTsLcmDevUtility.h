/** @file

  Header of LCM Device Utility for Elan I2C-HID Touchscreen.

  Copyright (c) ELAN microelectronics corp. 2022, All Rights Reserved

  Module Name:
	ElanTsLcmDevUtility.h

  Environment:
	All kinds of Linux-like Platform.

********************************************************************
 Revision History

**/

#ifndef _ELAN_TS_LCM_DEVICE_UTILITY_H_
#define _ELAN_TS_LCM_DEVICE_UTILITY_H_

/*******************************************
 * Definitions
 ******************************************/

// Device Information Set
#ifndef DEV_INFO_SET_MAX
#define DEV_INFO_SET_MAX		100
#endif //DEV_INFO_SET_MAX

// File Length
#ifndef FILE_NAME_LENGTH_MAX
#define FILE_NAME_LENGTH_MAX	256
#endif //FILE_NAME_LENGTH_MAX

// Panel Information Length
#ifndef PANEL_INFO_LENGTH_MAX
#define PANEL_INFO_LENGTH_MAX	16
#endif //PANEL_INFO_LENGTH_MAX

// FWID Length
#ifndef FWID_LENGTH_MAX
#define FWID_LENGTH_MAX			8
#endif //FWID_LENGTH_MAX

// System Name Length
#ifndef SYSTEM_NAME_LENGTH
#define SYSTEM_NAME_LENGTH		16
#endif //SYSTEM_NAME_LENGTH

/*******************************************
 * Data Structure Declaration
 ******************************************/

// LCM Device Info.
typedef struct lcm_dev_info
{
    char panel_info[PANEL_INFO_LENGTH_MAX];
    unsigned short chrome_fwid;
    unsigned short windows_fwid;
} DEV_INFO, *PDEV_INFO;

// System Type
enum system_type
{
    UNKNOWN = 0,
    CHROME,
    WINDOWS
};

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

// FWID Mapping File & LCM Dev Info.
int parse_fwid_mapping_file(FILE *fd_mapping_file, struct lcm_dev_info *dev_info, size_t dev_info_size);
int show_lcm_dev_info(struct lcm_dev_info *p_dev_info, size_t dev_info_size);

// FWID
int get_fwid_from_edid(struct lcm_dev_info *p_dev_info, size_t dev_info_size, unsigned short manufacturer_code, unsigned short product_code, system_type system, unsigned short *p_fwid);
int show_fwid(system_type system, unsigned short fwid, bool silent_mode);

int get_fwid_from_rom(unsigned short *p_fwid, bool recovery);
int show_fwid_from_rom(unsigned short fwid);

#endif //_ELAN_TS_LCM_DEVICE_UTILITY_H_
