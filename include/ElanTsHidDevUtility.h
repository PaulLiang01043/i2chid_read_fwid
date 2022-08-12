/** @file

  Header of HID Device Utility for Elan I2C-HID Touchscreen.

  Copyright (c) ELAN microelectronics corp. 2022, All Rights Reserved

  Module Name:
	ElanTsHidDevUtility.h

  Environment:
	All kinds of Linux-like Platform.

********************************************************************
 Revision History

**/

#ifndef _ELAN_TS_HID_DEVICE_UTILITY_H_
#define _ELAN_TS_HID_DEVICE_UTILITY_H_

#include <linux/hidraw.h>	/* hidraw */

/*******************************************
 * Definitions
 ******************************************/

// Device Information Set
#ifndef DEV_INFO_SET_MAX
#define DEV_INFO_SET_MAX		100
#endif //DEV_INFO_SET_MAX

// ELAN Default VID
#ifndef ELAN_USB_VID
#define ELAN_USB_VID			0x04F3
#endif //ELAN_USB_VID

// ELAN Recovery PID
#ifndef ELAN_USB_RECOVERY_PID
#define ELAN_USB_RECOVERY_PID	0x0732
#endif //ELAN_USB_RECOVERY_PID

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

// HID Device Info.
const char *bus_str(int bus);
int get_hid_dev_info(struct hidraw_devinfo *p_hid_dev_info, size_t dev_info_size);
int show_hid_dev_info(struct hidraw_devinfo *p_hid_dev_info, size_t dev_info_size);

// Validate Elan Device
int validate_elan_hid_device(struct hidraw_devinfo *p_hid_dev_info, size_t dev_info_size, int pid, bool silent_mode);

#endif //_ELAN_TS_HID_DEVICE_UTILITY_H_
