/** @file

  Implementation of HID Device Utility for Elan I2C-HID Touchscreen.

  Copyright (c) ELAN microelectronics corp. 2022, All Rights Reserved

  Module Name:
	ElanTsHidDevUtility.cpp

  Environment:
	All kinds of Linux-like Platform.

**/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>     	/* close */
#include <dirent.h>         /* opendir, readdir, closedir */
#include <sys/stat.h>
#include <sys/types.h>
#include <linux/input.h>	/* BUS_TYPE */
#include "ErrCode.h"
#include "ElanTsHidDevUtility.h"

/***************************************************
 * Global Variable Declaration
 ***************************************************/

/***************************************************
 * Function Implements
 ***************************************************/

const char* bus_str(int bus)
{
    switch (bus)
    {
        case BUS_USB:
            return "USB";

        case BUS_HIL:
            return "HIL";

        case BUS_BLUETOOTH:
            return "Bluetooth";

#if 0 // Disable this convert since the definition is not include in input.h of android sdk.
        case BUS_VIRTUAL:
            return "Virtual";
#endif // 0

        case BUS_I2C:
            return "I2C";

        default:
            return "Other";
    }
}

int get_hid_dev_info(struct hidraw_devinfo *p_hid_dev_info, size_t dev_info_size)
{
    int err = TP_SUCCESS,
        ret = 0,
        index = 0,
        fd = 0;
    DIR *pDirectory = NULL;
    struct dirent *pDirEntry = NULL;
    const char *pszPath = "/dev";
    char szFile[64] = {0};
    struct hidraw_devinfo dev_info;

    // Check if Parameter Invalid
    if ((p_hid_dev_info == NULL) || (dev_info_size == 0))
    {
        ERROR_PRINTF("%s: Invalid Parameter! (p_hid_dev_info=0x%p, dev_info_size=%zd)\r\n", __func__, p_hid_dev_info, dev_info_size);
        err = TP_ERR_INVALID_PARAM;
        goto GET_HID_DEV_INFO_EXIT;
    }

    // Open Directory
    pDirectory = opendir(pszPath);
    if (pDirectory == NULL)
    {
        ERROR_PRINTF("%s: Fail to Open Directory %s.\r\n", __func__, pszPath);
        err = TP_ERR_NOT_FOUND_DEVICE;
        goto GET_HID_DEV_INFO_EXIT;
    }

    // Traverse Directory Elements
    while ((pDirEntry = readdir(pDirectory)) != NULL)
    {
        // Only reserve hidraw devices
        if (strncmp(pDirEntry->d_name, "hidraw", 6))
            continue;

        memset(szFile, 0, sizeof(szFile));
        sprintf(szFile, "%s/%s", pszPath, pDirEntry->d_name);
        DEBUG_PRINTF("%s: file=\"%s\".\r\n", __func__, szFile);

        /* Open the Device with non-blocking reads */
        fd = open(szFile, O_RDWR | O_NONBLOCK);
        if (fd < 0)
        {
            DEBUG_PRINTF("%s: Fail to Open Device %s! errno=%d.\r\n", __func__, pDirEntry->d_name, fd);
            continue;
        }

        /* Get Raw Info */
        ret = ioctl(fd, HIDIOCGRAWINFO, &dev_info);
        if (ret >= 0)
        {
            DEBUG_PRINTF("--------------------------------\r\n");
            DEBUG_PRINTF("\tbustype: 0x%02x (%s)\r\n", dev_info.bustype, bus_str(dev_info.bustype));
            DEBUG_PRINTF("\tvendor: 0x%04hx\r\n", dev_info.vendor);
            DEBUG_PRINTF("\tproduct: 0x%04hx\r\n", dev_info.product);

            if(index < DEV_INFO_SET_MAX)
            {
                memcpy(&p_hid_dev_info[index], &dev_info, sizeof(struct hidraw_devinfo));
                index++;
            }
        }

        // Close Device
        close(fd);
    }

    // Close Directory
    closedir(pDirectory);

GET_HID_DEV_INFO_EXIT:
    return err;
}

int show_hid_dev_info(struct hidraw_devinfo *p_hid_dev_info, size_t dev_info_size)
{
    int err = TP_SUCCESS,
        index = 0;

    // Check if Parameter Invalid
    if ((p_hid_dev_info == NULL) || (dev_info_size == 0))
    {
        ERROR_PRINTF("%s: Invalid Parameter! (hid_dev_info=0x%p, dev_info_size=%zd)\r\n", __func__, p_hid_dev_info, dev_info_size);
        err = TP_ERR_INVALID_PARAM;
        goto SHOW_HID_DEV_INFO_EXIT;
    }

    printf("--------------------------------------\r\n");
    printf("HID Device Information:\r\n");

    for(index = 0; index < DEV_INFO_SET_MAX; index++)
    {
        // Check for Device Data Valid
        if(p_hid_dev_info[index].bustype <= 0)
            continue;

        printf("Bus %03x(%s): VID %04x, PID %04x.\r\n", p_hid_dev_info[index].bustype, bus_str(p_hid_dev_info[index].bustype), p_hid_dev_info[index].vendor, p_hid_dev_info[index].product);
    }

SHOW_HID_DEV_INFO_EXIT:
    return err;
}

int validate_elan_hid_device(struct hidraw_devinfo *p_hid_dev_info, size_t dev_info_size, int pid, bool silent_mode)
{
    int err = TP_SUCCESS,
        index = 0;
    bool dev_exist = false;

    // Check if Parameter Invalid
    if ((p_hid_dev_info == NULL) || (dev_info_size == 0))
    {
        ERROR_PRINTF("%s: Invalid Parameter! (p_hid_dev_info=0x%p, dev_info_size=%zd)\r\n", __func__, p_hid_dev_info, dev_info_size);
        err = TP_ERR_INVALID_PARAM;
        goto VALIDATE_ELAN_HID_DEVICE_EXIT;
    }

    if(silent_mode == false)
        printf("--------------------------------------\r\n");

    // Compare all hidraw device info. with VID/PID
    for(index = 0; index < DEV_INFO_SET_MAX; index++)
    {
        // Check for Device Data Valid
        if(p_hid_dev_info[index].bustype <= 0)
            continue;

        DEBUG_PRINTF("%s: Bus_Type=%03x, VID=%04x, PID=%04x.\r\n", __func__, p_hid_dev_info[index].bustype, p_hid_dev_info[index].vendor, p_hid_dev_info[index].product);
        if((p_hid_dev_info[index].bustype == BUS_I2C) && \
           (p_hid_dev_info[index].vendor == ELAN_USB_VID) && \
           ((p_hid_dev_info[index].product == pid) || (p_hid_dev_info[index].product == ELAN_USB_RECOVERY_PID)))
        {
            if(silent_mode == false)
                printf("Device exists! (VID: %04x, PID: %04x)\r\n", p_hid_dev_info[index].vendor, p_hid_dev_info[index].product);
            dev_exist = true;
            break;
        }
    }

    if(dev_exist == false)
    {
        if(silent_mode == false)
            printf("Device does not exist! (VID: %04x, PID: %04x)\r\n", p_hid_dev_info[index].vendor, p_hid_dev_info[index].product);
    }

    // Success
    err = TP_SUCCESS;

VALIDATE_ELAN_HID_DEVICE_EXIT:
    return err;
}

