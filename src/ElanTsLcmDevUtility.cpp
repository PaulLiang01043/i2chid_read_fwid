/** @file

  Implementation of LCM Device Utility for Elan I2C-HID Touchscreen.

  Copyright (c) ELAN microelectronics corp. 2022, All Rights Reserved

  Module Name:
	ElanTsLcmDevUtility.cpp

  Environment:
	All kinds of Linux-like Platform.

**/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ErrCode.h"
#include "ElanTsLcmDevUtility.h"

/***************************************************
 * Global Variable Declaration
 ***************************************************/

/***************************************************
 * Function Implements
 ***************************************************/

int parse_fwid_mapping_file(FILE *fd_mapping_file, struct lcm_dev_info *p_dev_info, size_t dev_info_size)
{
    int err = 0,
        line_index = 0,
        token_index = 0,
        dev_info_index = 0;
    char line[1024] = {0},
         *token = NULL,
         chrome_fwid[FWID_LENGTH_MAX] = {0},
         windows_fwid[FWID_LENGTH_MAX] = {0};

    if (fd_mapping_file == NULL)
    {
        ERROR_PRINTF("%s: NULL file pointer for FWID mapping file!\r\n", __func__);
        err = TP_ERR_INVALID_PARAM;
        goto PARSE_FWID_MAPPING_FILE_EXIT;
    }

    if ((p_dev_info == NULL) || (dev_info_size == 0))
    {
        ERROR_PRINTF("%s: Invalid Parameter! (p_dev_info=0x%p, dev_info_size=%zd)\r\n", __func__, p_dev_info, dev_info_size);
        err = TP_ERR_INVALID_PARAM;
        goto PARSE_FWID_MAPPING_FILE_EXIT;
    }

    // Read mapping file line by line
    while (fgets(line, sizeof(line), fd_mapping_file) != NULL)
    {
        token_index = 0;
        token = strtok(line, ",");
        while (token != NULL)
        {
            if ((strcmp(token, "\n") != 0) && (strcmp(token, "") != 0))
            {
                switch (token_index)
                {
                    case 0: // panel_info[PANEL_INFO_LENGTH_MAX]
                        if((strcmp(token, "") != 0) && (strlen(token) < sizeof(p_dev_info[dev_info_index].panel_info)))
                            strncpy(p_dev_info[dev_info_index].panel_info, token, strlen(token));
                        break;
                    case 1: // chrome_fwid[FWID_LENGTH_MAX]
                        if((strcmp(token, "") != 0) && (strlen(token) < sizeof(chrome_fwid)))
                        {
                            strncpy(chrome_fwid, token, strlen(token));
                            p_dev_info[dev_info_index].chrome_fwid = strtol(chrome_fwid, NULL, 16);
                        }
                        break;
                    case 2: // windows_fwid[FWID_LENGTH_MAX]
                        if((strcmp(token, "") != 0) && (strlen(token) < sizeof(windows_fwid)))
                        {
                            strncpy(windows_fwid, token, strlen(token));
                            p_dev_info[dev_info_index].windows_fwid = strtol(windows_fwid, NULL, 16);
                        }
                        break;
                    default: // Unknown
                        DEBUG_PRINTF("%s: [%d] Known token \"%s\"\r\n", __func__, token_index, token);
                        break;
                }
            }

            token = strtok(NULL, ",");
            token_index++;
        }

        /* Debug */
        if(strcmp(p_dev_info[dev_info_index].panel_info, "") != 0)
        {
            DEBUG_PRINTF("%s: Device %d - panel_info: \"%s\", chrome_fwid: 0x%04x, windows_fwid: 0x%04x.\r\n", __func__, dev_info_index, \
                         p_dev_info[dev_info_index].panel_info, p_dev_info[dev_info_index].chrome_fwid, p_dev_info[dev_info_index].windows_fwid);
        }

        memset(line, 0, sizeof(line));
        line_index++;
        dev_info_index++;
    }

PARSE_FWID_MAPPING_FILE_EXIT:
    return err;
}

int show_lcm_dev_info(struct lcm_dev_info *p_dev_info, size_t dev_info_size)
{
    int err = TP_SUCCESS,
        index = 0;

    // Check if Parameter Invalid
    if ((p_dev_info == NULL) || (dev_info_size == 0))
    {
        ERROR_PRINTF("%s: Invalid Parameter! (p_dev_info=0x%p, dev_info_size=%zd)\r\n", __func__, p_dev_info, dev_info_size);
        err = TP_ERR_INVALID_PARAM;
        goto SHOW_LCM_DEV_INFO_EXIT;
    }

    printf("--------------------------------------\r\n");
    printf("LCM Devices:\r\n");

    for(index = 0; index < DEV_INFO_SET_MAX; index++)
    {
        // Check for Device Data Valid
        if(strcmp(p_dev_info[index].panel_info, "") == 0)
            continue;

        printf("Device %d: panel_info \"%s\", chrome_fwid %04x, windows_fwid %04x.\r\n", index, p_dev_info[index].panel_info, p_dev_info[index].chrome_fwid, p_dev_info[index].windows_fwid);
    }

SHOW_LCM_DEV_INFO_EXIT:
    return err;
}

int get_fwid_from_edid(struct lcm_dev_info *p_dev_info, size_t dev_info_size, unsigned short manufacturer_code, unsigned short product_code, system_type system, unsigned short *p_fwid)
{
    int err = TP_ERR_DATA_NOT_FOUND,
        index = 0;
    char target_panel_info[PANEL_INFO_LENGTH_MAX] = {0};

    // Check if Parameter Invalid
    if ((p_dev_info == NULL) || (dev_info_size == 0) || (manufacturer_code == 0) || (product_code == 0) || (system == UNKNOWN) || (p_fwid == NULL))
    {
        ERROR_PRINTF("%s: Invalid Parameter! (p_dev_info=0x%p, dev_info_size=%zd, manufacturer_code=0x%x, product_code=0x%x, system=%d, p_fwid=0x%p)\r\n", \
                     __func__, p_dev_info, dev_info_size, manufacturer_code, product_code, system, p_fwid);
        err = TP_ERR_INVALID_PARAM;
        goto GET_FWID_FROM_EDID_EXIT;
    }

    // Set Target Panel Info.
    sprintf(target_panel_info, "%04x.%04x", manufacturer_code, product_code);
    DEBUG_PRINTF("%s: target_panel_info: \"%s\".\r\n", __func__, target_panel_info);

    // Look for Device Info. with Matched Panel Info.
    for(index = 0; index < DEV_INFO_SET_MAX; index++)
    {
        // Check for Device Data Valid
        if(strcmp(p_dev_info[index].panel_info, "") == 0)
            continue;

        DEBUG_PRINTF("%s: [%d] panel_info: \"%s\", chrome_fwid: %04x, windows_fwid: %04x.\r\n", \
                     __func__, index, p_dev_info[index].panel_info, p_dev_info[index].chrome_fwid, p_dev_info[index].windows_fwid);
        if(strcmp(p_dev_info[index].panel_info, target_panel_info) == 0)
        {
            if(system == CHROME)
            {
                DEBUG_PRINTF("%s: [Chrome] FWID: %04x.\r\n", __func__, p_dev_info[index].chrome_fwid);
                if(p_dev_info[index].chrome_fwid == 0x0) // NULL FWID
                {
                    DEBUG_PRINTF("%s: NULL FWID.\r\n", __func__);
                    err = TP_ERR_DATA_NOT_FOUND; // Not Found
                }
                else // Valid FWID
                {
                    *p_fwid = p_dev_info[index].chrome_fwid;
                    err = TP_SUCCESS; // Found
                }
                break;
            }
            else if (system == WINDOWS)
            {
                DEBUG_PRINTF("%s: [Windows] FWID: %04x.\r\n", __func__, p_dev_info[index].windows_fwid);
                if(p_dev_info[index].windows_fwid == 0x0) // NULL FWID
                {
                    DEBUG_PRINTF("%s: NULL FWID.\r\n", __func__);
                    err = TP_ERR_DATA_NOT_FOUND; // Not Found
                }
                else // Valid FWID
                {
                    *p_fwid = p_dev_info[index].windows_fwid;
                    err = TP_SUCCESS; // Found
                }
                break;
            }
        }
    }

GET_FWID_FROM_EDID_EXIT:
    return err;
}

