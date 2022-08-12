/** @file

  Implementation of EDID Utility for Elan I2C-HID Touchscreen.

  Copyright (c) ELAN microelectronics corp. 2022, All Rights Reserved

  Module Name:
	ElanTsEdidUtility.cpp

  Environment:
	All kinds of Linux-like Platform.

**/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "ErrCode.h"
#include "ElanTsEdidUtility.h"

/***************************************************
 * Global Variable Declaration
 ***************************************************/

/***************************************************
 * Function Implements
 ***************************************************/

// EDID
int get_edid_manufacturer_product_code(unsigned short *p_manufacturer_code, unsigned short *p_product_code)
{
    int err = TP_SUCCESS;
    char buf[1024] = {0};
    char *p_edid_header = NULL,
         *p_edid_manufacturer_code = NULL,
         *p_edid_product_code = NULL;
    char edid_manufacturer_code[5] = {0},
         edid_product_code[5] = {0};
    bool edid_header_found = false;
    FILE *fd_process_pipe;

    // Check if Parameter Invalid
    if ((p_manufacturer_code == NULL) || (p_product_code == NULL))
    {
        ERROR_PRINTF("%s: Invalid Parameter! (p_manufacturer_code=0x%p, p_product_code=0x%p)\r\n", __func__, p_manufacturer_code, p_product_code);
        err = TP_ERR_INVALID_PARAM;
        goto GET_EDID_MANUFACTURER_PRODUCT_CODE_EXIT;
    }

    /* Open the command for reading. */
    fd_process_pipe = popen(MODETEST_CMD, "r");
    if (fd_process_pipe == NULL)
    {
        ERROR_PRINTF("%s: Command \"%s\" failed! errno=%d.\r\n", __func__, MODETEST_CMD, errno);
        err = ERR_SYSTEM_COMMAND_FAIL;
        goto GET_EDID_MANUFACTURER_PRODUCT_CODE_EXIT;
    }

    /* Read the output line by line */
    while (fgets(buf, sizeof(buf)-1, fd_process_pipe) != NULL)
    {
        //DEBUG_PRINTF("%s", buf);

        /* Search for EDID Header */

        // Pattern 1: Standard EDID Header
        p_edid_header = strstr(buf, STANDARD_EDID_HEADER);
        if(p_edid_header != NULL)
        {
            DEBUG_PRINTF("%s: Standard EDID header found!\r\n", __func__);
            edid_header_found = true;
            break;
        }

        // Pattern 2: AUO EDID Header
        p_edid_header = strstr(buf, AUO_EDID_HEADER);
        if(p_edid_header != NULL)
        {
            DEBUG_PRINTF("%s: AUO EDID header found!\r\n", __func__);
            edid_header_found = true;
            break;
        }

        // Pattern 3: BOE EDID Header
        p_edid_header = strstr(buf, BOE_EDID_HEADER);
        if(p_edid_header != NULL)
        {
            DEBUG_PRINTF("%s: BOE EDID header found!\r\n", __func__);
            edid_header_found = true;
            break;
        }

        // If no known EDID headers found, continue to search next line.
        if(edid_header_found == false)
            continue;
    }

    /* close */
    pclose(fd_process_pipe);

    DEBUG_PRINTF("%s: EDID Header Found? %s.\r\n", __func__, (edid_header_found) ? "true" : "false");
    if(edid_header_found == false)
    {
        // Terminate the process and return error code
        err = TP_ERR_DATA_NOT_FOUND;
        goto GET_EDID_MANUFACTURER_PRODUCT_CODE_EXIT;
    }

    /* Parse found EDID information */

    // Line output with EDID Header
    DEBUG_PRINTF("%s: %s", __func__, buf);

    // Manufacturer Code
    p_edid_manufacturer_code = p_edid_header + strlen(STANDARD_EDID_HEADER);
    strncpy(edid_manufacturer_code, p_edid_manufacturer_code, 4);
    *p_manufacturer_code = strtol(edid_manufacturer_code, NULL, 16);
    DEBUG_PRINTF("%s: manufacturer_code: %04x.\r\n", __func__, *p_manufacturer_code);

    // Product Code
    p_edid_product_code = p_edid_manufacturer_code + 4;
    strncpy(edid_product_code, p_edid_product_code, 4);
    *p_product_code = strtol(edid_product_code, NULL, 16);
    DEBUG_PRINTF("%s: product_code: %04x.\r\n", __func__, *p_product_code);

    // Success
    err = TP_SUCCESS;

GET_EDID_MANUFACTURER_PRODUCT_CODE_EXIT:
    return err;
}

int show_edid_manufacturer_product_code(unsigned short manufacturer_code, unsigned short product_code)
{
    printf("--------------------------------------\r\n");
    printf("Panel EDID Information:\r\n");
    printf("Manufacturer Code: %04x.\r\n", manufacturer_code);
    printf("Product Code: %04x.\r\n", product_code);

    return TP_SUCCESS;
}

