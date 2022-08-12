/** @file

  Implementation of Function APIs for Elan Gen8 I2C-HID Touchscreen.

  Copyright (c) ELAN microelectronics corp. 2022, All Rights Reserved

  Module Name:
	ElanGen8TsFuncApi.cpp

  Environment:
	All kinds of Linux-like Platform.

**/

#include "InterfaceGet.h"
#include "ElanGen8TsI2chidUtility.h"
#include "ElanGen8TsFuncApi.h"

/***************************************************
 * Global Variable Declaration
 ***************************************************/

/***************************************************
 * Function Implements
 ***************************************************/

// ROM Data
int gen8_get_rom_data(unsigned int addr, unsigned char data_len, unsigned int *p_data)
{
    int err = TP_SUCCESS;
    unsigned int data = 0;

    // Check if Parameter Invalid
    if (p_data == NULL)
    {
        ERROR_PRINTF("%s: Invalid Parameter! (p_data=0x%p)\r\n", __func__, p_data);
        err = TP_ERR_INVALID_PARAM;
        goto GEN8_GET_ROM_DATA_EXIT;
    }

    /* Read Data from ROM */

    // Send New Read ROM Data Command (New 0x96 Command)
    err = gen8_send_read_rom_data_command(addr, data_len);
    if(err != TP_SUCCESS)
    {
        ERROR_PRINTF("%s: Fail to Send New Read ROM Data Command! err=0x%x.\r\n", __func__, err);
        goto GEN8_GET_ROM_DATA_EXIT;
    }

    // Receive ROM Data
    err = gen8_receive_rom_data(&data);
    if(err != TP_SUCCESS)
    {
        ERROR_PRINTF("%s: Fail to Receive ROM Data! err=0x%x.\r\n", __func__, err);
        goto GEN8_GET_ROM_DATA_EXIT;
    }

    // Load ROM Data to Input Buffer
    *p_data = data;

    // Success
    err = TP_SUCCESS;

GEN8_GET_ROM_DATA_EXIT:
    return err;
}

// Information FWID
int gen8_read_info_fwid(unsigned short *p_info_fwid, bool recovery)
{
    int err = TP_SUCCESS;
    unsigned char info_fwid_low_byte = 0,
                  info_fwid_high_byte = 0;
    unsigned short info_fwid = 0;
    unsigned int rom_data = 0;

    // Check if Parameter Invalid
    if (p_info_fwid == NULL)
    {
        ERROR_PRINTF("%s: Invalid Parameter! (p_info_fwid=0x%p)\r\n", __func__, p_info_fwid);
        err = TP_ERR_INVALID_PARAM;
        goto GEN8_READ_INFO_FWID_EXIT;
    }

    // Read ROM Data from Information ROM Address (0x40000)
    if(!recovery) // Normal Mode
    {
        /*
         * Touch firmware supports Read  8-bit RAM/ROM Data Command,
         *                         Read 16-bit RAM/ROM Data Command, and
         *                         Read 32-bit RAM/ROM Data Command.
         */

        err = gen8_get_rom_data(ELAN_GEN8_INFO_ROM_FWID_MEMORY_ADDR, 2, &rom_data); // Word Data / 16-bit
        if(err != TP_SUCCESS)
        {
            ERROR_PRINTF("%s: Fail to Get ROM Data of MEM[0x%08x]! err=0x%x.\r\n", \
                         __func__, ELAN_GEN8_INFO_ROM_FWID_MEMORY_ADDR, err);
            goto GEN8_READ_INFO_FWID_EXIT;
        }

        // Get Low Word (16-bit) of ROM Data
        info_fwid = LOW_WORD(rom_data);
        DEBUG_PRINTF("%s: Information FWID: 0x%04x.\r\n", __func__, info_fwid);
    }
    else // Recovery Mode
    {
        /*
         * Touch boot code only supports Read 8-bit RAM/ROM Data Command.
         */

        // Low Byte of Information FWID
        err = gen8_get_rom_data(ELAN_GEN8_INFO_ROM_FWID_MEMORY_ADDR, 1, &rom_data); // Byte Data / 8-bit
        if(err != TP_SUCCESS)
        {
            ERROR_PRINTF("%s: Fail to Get ROM Data of MEM[0x%08x]! err=0x%x.\r\n", \
                         __func__, ELAN_GEN8_INFO_ROM_FWID_MEMORY_ADDR, err);
            goto GEN8_READ_INFO_FWID_EXIT;
        }
        info_fwid_low_byte = LOW_BYTE(rom_data);
        DEBUG_PRINTF("%s: MEM[0x%08x]=0x%02x.\r\n", __func__, ELAN_GEN8_INFO_ROM_FWID_MEMORY_ADDR, info_fwid_low_byte);

        // High Byte of Information FWID
        err = gen8_get_rom_data((ELAN_GEN8_INFO_ROM_FWID_MEMORY_ADDR + 1), 1, &rom_data); // Byte Data / 8-bit
        if(err != TP_SUCCESS)
        {
            ERROR_PRINTF("%s: Fail to Get ROM Data of MEM[0x%08x]! err=0x%x.\r\n", \
                         __func__, (ELAN_GEN8_INFO_ROM_FWID_MEMORY_ADDR + 1), err);
            goto GEN8_READ_INFO_FWID_EXIT;
        }
        info_fwid_high_byte = LOW_BYTE(rom_data);
        DEBUG_PRINTF("%s: MEM[0x%08x]=0x%02x.\r\n", __func__, (ELAN_GEN8_INFO_ROM_FWID_MEMORY_ADDR + 1), info_fwid_high_byte);

        // Get Low Word (16-bit) of ROM Data
        info_fwid = ((unsigned short)info_fwid_high_byte << 8) | (unsigned short)info_fwid_low_byte;
        DEBUG_PRINTF("%s: Information FWID: 0x%04x.\r\n", __func__, info_fwid);
    }

    // Load Information FWID to Input Buffer
    *p_info_fwid = info_fwid;

    // Success
    err = TP_SUCCESS;

GEN8_READ_INFO_FWID_EXIT:
    return err;
}

