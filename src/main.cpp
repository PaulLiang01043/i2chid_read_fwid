/******************************************************************************
 * Copyright (c) 2019 ELAN Technology Corp. All Rights Reserved.
 *
 * Implementation of Elan I2C-HID Read FWID Tool
 *
 *Release:
 *		2019/3
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <getopt.h>
#include "I2CHIDLinuxGet.h"
#include "ElanTsI2chidUtility.h"
#include "ElanTsFuncApi.h"
#include "ElanTsHidDevUtility.h"
#include "ElanTsEdidUtility.h"
#include "ElanTsLcmDevUtility.h"
#include "ElanGen8TsI2chidHwParameters.h"
#include "ElanGen8TsFuncApi.h"

/*******************************************
 * Definitions
 ******************************************/

// SW Version
#ifndef ELAN_TOOL_SW_VERSION
#define	ELAN_TOOL_SW_VERSION 	"1.1"
#endif //ELAN_TOOL_SW_VERSION

// SW Release Date
#ifndef ELAN_TOOL_SW_RELEASE_DATE
#define ELAN_TOOL_SW_RELEASE_DATE	"2022-10-28"
#endif //ELAN_TOOL_SW_RELEASE_DATE

// Error Retry Count
#ifndef ERROR_RETRY_COUNT
#define ERROR_RETRY_COUNT	3
#endif //ERROR_RETRY_COUNT

/*******************************************
 * Data Structure Declaration
 ******************************************/

/*******************************************
 * Global Variables Declaration
 ******************************************/

// Debug
bool g_debug = false;

#ifndef DEBUG_PRINTF
#define DEBUG_PRINTF(fmt, argv...) if(g_debug) printf(fmt, ##argv)
#endif //DEBUG_PRINTF

#ifndef ERROR_PRINTF
#define ERROR_PRINTF(fmt, argv...) fprintf(stderr, fmt, ##argv)
#endif //ERROR_PRINTF

// InterfaceGet Class
CI2CHIDLinuxGet *g_pIntfGet = NULL;		// Pointer to I2CHID Inteface Class (CI2CHIDLinuxGet)

// Validate Touchscreen Device
bool g_validate_dev = false;

// PID
int g_pid = 0; //ELAN_USB_FORCE_CONNECT_PID;

// Look-up FWID Function
bool g_lookup_fwid = false;

// FWID Mapping Table
char g_fwid_mapping_file_path[FILE_NAME_LENGTH_MAX] = {0};

// System Inforamtion
bool g_show_system_info = false;

// System Type
system_type g_system_type = UNKNOWN;

// Silent Mode (Quiet)
bool g_silent_mode = false;

// Help Info.
bool g_help = false;

// Parameter Option Settings
const char* const short_options = "p:P:f:s:iqdh";
const struct option long_options[] =
{
    { "pid",				1, NULL, 'p'},
    { "pid_hex",			1, NULL, 'P'},
    { "mapping_file_path",	1, NULL, 'f'},
    { "system",				1, NULL, 's'},
    { "dev_info",			0, NULL, 'i'},
    { "quiet",				0, NULL, 'q'},
    { "debug",				0, NULL, 'd'},
    { "help",				0, NULL, 'h'},
};

/*******************************************
 * Function Prototype
 ******************************************/

// Information FWID
int show_info_fwid(unsigned short info_fwid);

// FWID
int show_fwid(system_type system, unsigned short fwid, bool silent_mode);

// System Info. (HID Device / Panel EDID / LCM Device)
int get_system_info(struct hidraw_devinfo *p_hid_dev_info, size_t hid_dev_info_size, \
                    unsigned short *p_edid_manufacturer_code, unsigned short *p_edid_product_code, \
                    char* p_fwid_mapping_file_path, size_t fwid_mapping_file_path_size, \
                    struct lcm_dev_info *p_lcm_dev_info, size_t lcm_dev_info_size);

int show_system_info(struct hidraw_devinfo *p_hid_dev_info, size_t hid_dev_info_size, \
                     bool edid_info_found, unsigned short edid_manufacturer_code, unsigned short edid_product_code, \
                     bool lookup_fwid, struct lcm_dev_info *p_lcm_dev_info, size_t lcm_dev_info_size, \
                     unsigned short info_fwid);

// Help
void show_help_information(void);

// HID Raw I/O Function
int __hidraw_write(unsigned char* buf, int len, int timeout_ms);
int __hidraw_read(unsigned char* buf, int len, int timeout_ms);

// Abstract Device I/O Function
int write_cmd(unsigned char *cmd_buf, int len, int timeout_ms);
int read_data(unsigned char *data_buf, int len, int timeout_ms);
int write_vendor_cmd(unsigned char *cmd_buf, int len, int timeout_ms);
int open_device(void);
int close_device(void);

// Default Function
int process_parameter(int argc, char **argv);
int resource_init(void);
int resource_free(void);
int main(int argc, char **argv);

/*******************************************
 * HID Raw I/O Functions
 ******************************************/

int __hidraw_write(unsigned char* buf, int len, int timeout_ms)
{
    int nRet = TP_SUCCESS;

    if(g_pIntfGet == NULL)
    {
        nRet = TP_ERR_COMMAND_NOT_SUPPORT;
        goto __HIDRAW_WRITE_EXIT;
    }

    nRet = g_pIntfGet->WriteRawBytes(buf, len, timeout_ms);

__HIDRAW_WRITE_EXIT:
    return nRet;
}

int __hidraw_read(unsigned char* buf, int len, int timeout_ms)
{
    int nRet = TP_SUCCESS;

    if(g_pIntfGet == NULL)
    {
        nRet = TP_ERR_COMMAND_NOT_SUPPORT;
        goto __HIDRAW_READ_EXIT;
    }

    nRet = g_pIntfGet->ReadRawBytes(buf, len, timeout_ms);

__HIDRAW_READ_EXIT:
    return nRet;
}

int __hidraw_write_command(unsigned char* buf, int len, int timeout_ms)
{
    int nRet = TP_SUCCESS;

    if(g_pIntfGet == NULL)
    {
        nRet = TP_ERR_COMMAND_NOT_SUPPORT;
        goto __HIDRAW_WRITE_EXIT;
    }

    nRet = g_pIntfGet->WriteCommand(buf, len, timeout_ms);

__HIDRAW_WRITE_EXIT:
    return nRet;
}

int __hidraw_read_data(unsigned char* buf, int len, int timeout_ms)
{
    int nRet = TP_SUCCESS;

    if(g_pIntfGet == NULL)
    {
        nRet = TP_ERR_COMMAND_NOT_SUPPORT;
        goto __HIDRAW_READ_EXIT;
    }

    nRet = g_pIntfGet->ReadData(buf, len, timeout_ms);

__HIDRAW_READ_EXIT:
    return nRet;
}

/***************************************************
 * Abstract I/O Functions
 ***************************************************/

int write_cmd(unsigned char *cmd_buf, int len, int timeout_ms)
{
    //write_bytes_from_buffer_to_i2c(cmd_buf); //pseudo function

    /*** example *********************/
    return __hidraw_write_command(cmd_buf, len, timeout_ms);
    /*********************************/
}

int read_data(unsigned char *data_buf, int len, int timeout_ms)
{
    //read_bytes_from_i2c_to_buffer(data_buf, len, timeout); //pseudo function

    /*** example *********************/
    return __hidraw_read_data(data_buf, len, timeout_ms);
    /*********************************/
}

int write_vendor_cmd(unsigned char *cmd_buf, int len, int timeout_ms)
{
    unsigned char vendor_cmd_buf[ELAN_I2CHID_OUTPUT_BUFFER_SIZE] = {0};

    // Add HID Header
    vendor_cmd_buf[0] = ELAN_HID_OUTPUT_REPORT_ID;
    memcpy(&vendor_cmd_buf[1], cmd_buf, len);

    return __hidraw_write(vendor_cmd_buf, sizeof(vendor_cmd_buf), timeout_ms);
}

/*******************************************
 * Function Implementation
 ******************************************/

int show_info_fwid(unsigned short info_fwid)
{
    int err = TP_SUCCESS;

    printf("--------------------------------------\r\n");
    printf("Touch Information:\r\n");
    printf("Information FWID: %04x.\r\n", info_fwid);

    return err;
}

int show_fwid(system_type system, unsigned short fwid, bool silent_mode)
{
    int err = TP_SUCCESS;

    // Check if Parameter Invalid
    if(system == UNKNOWN)
    {
        ERROR_PRINTF("%s: Unknown System Type (%d)!\r\n", __func__, system);
        err = TP_ERR_INVALID_PARAM;
        goto SHOW_FWID_EXIT;
    }

    if(silent_mode == true)
    {
        printf("%04x", fwid);
    }
    else //if(silent_mode == false)
    {
        printf("--------------------------------------\r\n");
        printf("System: %s.\r\n", (system == CHROME) ? "Chrome" : "Windows");
        printf("FWID: %04x.\r\n", fwid);
    }

SHOW_FWID_EXIT:
    return err;
}

// System Info. (HID Device / Panel EDID / LCM Device)
int get_system_info(struct hidraw_devinfo *p_hid_dev_info, size_t hid_dev_info_size, \
                    unsigned short *p_edid_manufacturer_code, unsigned short *p_edid_product_code, bool *p_edid_info_found, \
                    char* p_fwid_mapping_file_path, size_t fwid_mapping_file_path_size, \
                    struct lcm_dev_info *p_lcm_dev_info, size_t lcm_dev_info_size)
{
    int err = TP_SUCCESS;
    bool edid_info_found = false;
    FILE *p_fd_fwid_mapping_file = NULL;

    // Validate HID Device Buffer
    if((p_hid_dev_info == NULL) || (hid_dev_info_size == 0))
    {
        ERROR_PRINTF("%s: Invalid HID Device Buffer! (p_hid_dev_info=0x%p, hid_dev_info_size=%ld)\r\n", \
                     __func__, p_hid_dev_info, hid_dev_info_size);
        err = TP_ERR_INVALID_PARAM;
        goto GET_SYSTEM_INFO_EXIT;
    }

    // Validate Panel EDID Info.
    if((p_edid_manufacturer_code == NULL) || (p_edid_product_code == NULL) || (p_edid_info_found == NULL))
    {
        ERROR_PRINTF("%s: Invalid Panel EDID Data Buffer! (p_edid_manufacturer_code=0x%p, p_edid_product_code=0x%p, p_edid_info_found=0x%p)\r\n", \
                     __func__, p_edid_manufacturer_code, p_edid_product_code, p_edid_info_found);
        err = TP_ERR_INVALID_PARAM;
        goto GET_SYSTEM_INFO_EXIT;
    }

    // Validate FWID Mapping Table File Path
    if((p_fwid_mapping_file_path == NULL) || (fwid_mapping_file_path_size == 0))
    {
        ERROR_PRINTF("%s: Invalid FWID Mapping Table File Path! (p_fwid_mapping_file_path=0x%p, fwid_mapping_file_path_size=%ld)\r\n", \
                     __func__, p_fwid_mapping_file_path, fwid_mapping_file_path_size);
        err = TP_ERR_INVALID_PARAM;
        goto GET_SYSTEM_INFO_EXIT;
    }

    // Validate LCM Device Buffer
    if((p_lcm_dev_info == NULL) || (lcm_dev_info_size == 0))
    {
        ERROR_PRINTF("%s: Invalid LCM Device Buffer! (p_lcm_dev_info=0x%p, lcm_dev_info_size=%ld)\r\n", \
                     __func__, p_lcm_dev_info, lcm_dev_info_size);
        err = TP_ERR_INVALID_PARAM;
        goto GET_SYSTEM_INFO_EXIT;
    }

    // Get HID Device Info.
    err = get_hid_dev_info(p_hid_dev_info, hid_dev_info_size);
    if(err != TP_SUCCESS)
    {
        ERROR_PRINTF("%s: Fail to Get HID Dev Info.! err=0x%x.\r\n", __func__, err);
        goto GET_SYSTEM_INFO_EXIT;
    }

    // Get EDID Manufacturer Code & Product Code
    err = get_edid_manufacturer_product_code(p_edid_manufacturer_code, p_edid_product_code);
    if (err != TP_SUCCESS)
    {
        //ERROR_PRINTF("%s: Fail to get EDID manufacturer code & product code ! err=0x%x.\r\n", __func__, err);
        if(err == TP_ERR_DATA_NOT_FOUND)
        {
            /* Set Flag for EDID Info. Not Found */

            // [Note] 2020/10/27
            // Since EDID information of BOE NV133FHM-N62 V8.1 can not be read by modetest command (used in Acer Karben project),
            // we add a new flag an flow to avoid process terminated by edid information read fail.
            edid_info_found = false;
        }
        else // Other
        {
            edid_info_found = false;
            goto GET_SYSTEM_INFO_EXIT;
        }
    }
    edid_info_found = true;
    DEBUG_PRINTF("%s: EDID Info. Found? %s.\r\n", __func__, (edid_info_found) ? "true" : "false");
    *p_edid_info_found = edid_info_found;

    // Get LCM Device Info.
    if(strcmp(g_fwid_mapping_file_path, "") != 0) // File path has been configured
    {
        // Open FWID Mapping File
        p_fd_fwid_mapping_file = fopen(p_fwid_mapping_file_path, "r");
        DEBUG_PRINTF("%s: p_fd_fwid_mapping_file=0x%p.\r\n", __func__, p_fd_fwid_mapping_file);
        if (p_fd_fwid_mapping_file == NULL)
        {
            ERROR_PRINTF("%s: Fail to open FWID mapping table file \"%s\"!\r\n", __func__, g_fwid_mapping_file_path);
            err = TP_ERR_FILE_NOT_FOUND;
            goto GET_SYSTEM_INFO_EXIT;
        }

        /* Parse FWID Mapping File */
        err = parse_fwid_mapping_file(p_fd_fwid_mapping_file, p_lcm_dev_info, lcm_dev_info_size);
        if (err != TP_SUCCESS)
        {
            ERROR_PRINTF("%s: Fail to parse FWID mapping file, err=%d.", __func__, err);
            //goto GET_SYSTEM_INFO_EXIT;
        }

        // Close FWID Mapping File
        fclose(p_fd_fwid_mapping_file);
        p_fd_fwid_mapping_file = NULL;
    }

    // Success
    err = TP_SUCCESS;

GET_SYSTEM_INFO_EXIT:
    return err;
}

int show_system_info(struct hidraw_devinfo *p_hid_dev_info, size_t hid_dev_info_size, \
                     bool edid_info_found, unsigned short edid_manufacturer_code, unsigned short edid_product_code, \
                     bool lookup_fwid, struct lcm_dev_info *p_lcm_dev_info, size_t lcm_dev_info_size, \
                     unsigned short info_fwid)
{
    int err = TP_SUCCESS;

    // Validate HID Device Buffer
    if((p_hid_dev_info == NULL) || (hid_dev_info_size == 0))
    {
        ERROR_PRINTF("%s: Invalid HID Device Buffer! (p_hid_dev_info=0x%p, hid_dev_info_size=%ld)\r\n", \
                     __func__, p_hid_dev_info, hid_dev_info_size);
        err = TP_ERR_INVALID_PARAM;
        goto SHOW_SYSTEM_INFO_EXIT;
    }

    // Validate LCM Device Buffer
    if((p_lcm_dev_info == NULL) || (lcm_dev_info_size == 0))
    {
        ERROR_PRINTF("%s: Invalid LCM Device Buffer! (p_lcm_dev_info=0x%p, lcm_dev_info_size=%ld)\r\n", \
                     __func__, p_lcm_dev_info, lcm_dev_info_size);
        err = TP_ERR_INVALID_PARAM;
        goto SHOW_SYSTEM_INFO_EXIT;
    }

    // Show HID Device Info.
    err = show_hid_dev_info(p_hid_dev_info, hid_dev_info_size);
    if (err != TP_SUCCESS)
    {
        ERROR_PRINTF("%s: Fail to show elan HID Device! err=0x%x.\r\n", __func__, err);
        goto SHOW_SYSTEM_INFO_EXIT;
    }

    // SHow EDID Info.
    if(edid_info_found == true) // EDID Info. Found
    {
        show_edid_manufacturer_product_code(edid_manufacturer_code, edid_product_code);
    }
    else // EDID Info. Not Found
    {
        ERROR_PRINTF("%s: EDID Info. Not Found!\r\n", __func__);
    }

    // Show LCM Device Info.
    if(lookup_fwid == true) // Lookup FWID has been Requested
    {
        err = show_lcm_dev_info(p_lcm_dev_info, lcm_dev_info_size);
        if (err != TP_SUCCESS)
        {
            ERROR_PRINTF("%s: Fail to show LCM Device Information! err=0x%x.\r\n", __func__, err);
            goto SHOW_SYSTEM_INFO_EXIT;
        }
    }

    // Show Information FWID
    show_info_fwid(info_fwid);

    // Success
    err = TP_SUCCESS;

SHOW_SYSTEM_INFO_EXIT:
    return err;
}

/*******************************************
 * Help
 ******************************************/

void show_help_information(void)
{
    printf("--------------------------------------\r\n");
    printf("SYNOPSIS:\r\n");

    // PID
    printf("\n[PID]\r\n");
    printf("-p <pid in decimal>.\r\n");
    printf("Ex: i2chid_read_fwid -p 1842\r\n");
    printf("-P <PID in hex>.\r\n");
    printf("Ex: i2chid_read_fwid -P 732 (0x732)\r\n");

    // FWID Mapping Table
    printf("\n[FWID Mapping Table]\r\n");
    printf("-f <fwid_mapping_table_file_path>.\r\n");
    printf("Ex: i2chid_read_fwid -f fwid_mapping_table.txt\r\n");
    printf("Ex: i2chid_read_fwid -f /tmp/fwid_mapping_table.txt\r\n");

    // System/Platform
    printf("\n[System Platform]\r\n");
    printf("-s <system>.\r\n");
    printf("Ex: i2chid_read_fwid -s chrome\r\n");
    printf("Ex: i2chid_read_fwid -s windows\r\n");

    // Device Information
    printf("\n[Device Information]\r\n");
    printf("-i.\r\n");
    printf("Ex: i2chid_read_fwid -i\r\n");

    // Silent (Quiet) Mode
    printf("\n[Silent Mode]\r\n");
    printf("-q.\r\n");
    printf("Ex: i2chid_read_fwid -q\r\n");

    // Debug Information
    printf("\n[Debug]\r\n");
    printf("-d.\r\n");
    printf("Ex: i2chid_read_fwid -d\r\n");

    // Help Information
    printf("\n[Help]\r\n");
    printf("-h.\r\n");
    printf("Ex: i2chid_read_fwid -h\r\n");

    return;
}

/*******************************************
 *  Open & Close Device
 ******************************************/
int open_device(void)
{
    int err = TP_SUCCESS;

    // open specific device on i2c bus //pseudo function

    /*** example *********************/
    if(g_pIntfGet == NULL)
    {
        err = TP_ERR_COMMAND_NOT_SUPPORT;
        goto OPEN_DEVICE_EXIT;
    }

    // Connect to Device
    DEBUG_PRINTF("Get I2C-HID Device Handle (VID=0x%x, PID=0x%x).\r\n", ELAN_USB_VID, g_pid);
    err = g_pIntfGet->GetDeviceHandle(ELAN_USB_VID, g_pid);
    if (err != TP_SUCCESS)
        ERROR_PRINTF("Device can't connected! err=0x%x.\n", err);

OPEN_DEVICE_EXIT:
    /*********************************/

    return err;
}

int close_device(void)
{
    int err = TP_SUCCESS;

    // close opened i2c device; //pseudo function

    /*** example *********************/
    if(g_pIntfGet == NULL)
    {
        err = TP_ERR_COMMAND_NOT_SUPPORT;
        goto CLOSE_DEVICE_EXIT;
    }

    // Release acquired touch device handler
    g_pIntfGet->Close();

CLOSE_DEVICE_EXIT:
    /*********************************/

    return err;
}

/*******************************************
 *  Initialize & Free Resource
 ******************************************/

int resource_init(void)
{
    int err = TP_SUCCESS;

    //initialize_resource(); //pseudo function

    /*** example *********************/

    // Initialize I2C-HID Interface
    g_pIntfGet = new CI2CHIDLinuxGet();
    DEBUG_PRINTF("g_pIntfGet=%p.\n", g_pIntfGet);
    if (g_pIntfGet == NULL)
    {
        ERROR_PRINTF("Fail to initialize I2C-HID Interface!");
        err = TP_ERR_NO_INTERFACE_CREATE;
        goto RESOURCE_INIT_EXIT;
    }

    // Success
    err = TP_SUCCESS;

RESOURCE_INIT_EXIT:
    /*********************************/

    return err;
}

int resource_free(void)
{
    int err = TP_SUCCESS;

    //release_resource(); //pseudo function

    /*** example *********************/

    // Release Interface
    if (g_pIntfGet)
    {
        delete dynamic_cast<CI2CHIDLinuxGet *>(g_pIntfGet);
        g_pIntfGet = NULL;
    }

    /*********************************/

    return err;
}

/***************************************************
* Parser command
***************************************************/

int process_parameter(int argc, char **argv)
{
    int err = TP_SUCCESS,
        opt = 0,
        option_index = 0,
        pid = 0,
        pid_str_len = 0,
        file_path_len = 0,
        system_str_len = 0;
    char file_path[FILE_NAME_LENGTH_MAX] = {0},
                                           system[SYSTEM_NAME_LENGTH] = {0};

    while (1)
    {
        opt = getopt_long(argc, argv, short_options, long_options, &option_index);
        if (opt == EOF)	break;

        switch (opt)
        {
            case 'p': /* PID (Decimal) */

                // Make Sure Data Valid
                pid = atoi(optarg);
                if (pid < 0)
                {
                    ERROR_PRINTF("%s: Invalid PID: %d!\n", __func__, pid);
                    err = TP_ERR_INVALID_PARAM;
                    goto PROCESS_PARAM_EXIT;
                }

                // Enable Device Check
                g_validate_dev = true;

                // Set PID
                g_pid = pid;
                DEBUG_PRINTF("%s: Validate Device: %s, PID: %d(0x%x).\r\n", __func__, (g_validate_dev) ? "Enable": "Disable", g_pid, g_pid);
                break;

            case 'P': /* PID (Hex) */

                // Make Sure Format Valid
                pid_str_len = strlen(optarg);
                if ((pid_str_len == 0) || (pid_str_len > 4))
                {
                    ERROR_PRINTF("%s: Invalid String Length for PID: %d!\n", __func__, pid_str_len);
                    err = TP_ERR_INVALID_PARAM;
                    goto PROCESS_PARAM_EXIT;
                }

                // Make Sure Data Valid
                pid = strtol(optarg, NULL, 16);
                if (pid < 0)
                {
                    ERROR_PRINTF("%s: Invalid PID: %d!\n", __func__, pid);
                    err = TP_ERR_INVALID_PARAM;
                    goto PROCESS_PARAM_EXIT;
                }

                // Enable Device Check
                g_validate_dev = true;

                // Set PID
                g_pid = pid;
                DEBUG_PRINTF("%s: Check Device: %s, PID: 0x%x.\r\n", __func__, (g_validate_dev) ? "Enable": "Disable", g_pid);
                break;

            case 'f': /* FWID Mapping Table File Path */

                // Check if FWID mapping table file is valid
                file_path_len = strlen(optarg);
                if ((file_path_len == 0) || ((size_t)file_path_len > sizeof(file_path)))
                {
                    ERROR_PRINTF("%s: Firmware Path (%s) Invalid!\r\n", __func__, optarg);
                    err = TP_ERR_INVALID_PARAM;
                    goto PROCESS_PARAM_EXIT;
                }

                // Check if file path is valid
                strcpy(file_path, optarg);
                if(strncmp(file_path, "", strlen(file_path)) == 0)
                {
                    ERROR_PRINTF("%s: NULL Firmware Path!\r\n", __func__);
                    err = TP_ERR_INVALID_PARAM;
                    goto PROCESS_PARAM_EXIT;
                }

                // Enable FWID Lookup
                g_lookup_fwid = true;

                // Set Global File Path
                strncpy(g_fwid_mapping_file_path, file_path, strlen(file_path));
                DEBUG_PRINTF("%s: FWID Lookup: %s, Mapping File: \"%s\".\r\n", __func__, (g_lookup_fwid) ? "Enable" : "Disable", g_fwid_mapping_file_path);
                break;

            case 's': /* System Type */

                // Make Sure Format Valid
                system_str_len = strlen(optarg);
                if ((system_str_len == 0) || ((size_t)system_str_len > sizeof(system)))
                {
                    ERROR_PRINTF("%s: Invalid String Length for System: %d!\n", __func__, system_str_len);
                    err = TP_ERR_INVALID_PARAM;
                    goto PROCESS_PARAM_EXIT;
                }

                // Check if system name is valid
                strcpy(system, optarg);
                if((strcmp(system, "chrome") == 0) || (strcmp(system, "CHROME") == 0))
                {
                    DEBUG_PRINTF("%s: System Type: Chrome.\r\n", __func__);
                    g_system_type = CHROME;
                }
                else if((strcmp(system, "windows") == 0) || (strcmp(system, "WINDOWS") == 0))
                {
                    DEBUG_PRINTF("%s: System Type: Windows.\r\n", __func__);
                    g_system_type = WINDOWS;
                }
                else
                {
                    ERROR_PRINTF("%s: System Type: Unknown (\"%s\")!\r\n", __func__, system);
                    g_system_type = UNKNOWN;
                    err = TP_ERR_INVALID_PARAM;
                    goto PROCESS_PARAM_EXIT;
                }
                break;

            case 'i': /* Sytem Information */

                // Show System Information
                g_show_system_info = true;
                DEBUG_PRINTF("%s: Show System Inforamtion: %s.\r\n", __func__, (g_show_system_info) ? "Enable" : "Disable");
                break;

            case 'q': /* Silent Mode (Quiet) */

                // Enable Silent Mode (Quiet)
                g_silent_mode = true;
                DEBUG_PRINTF("%s: Silent Mode: %s.\r\n", __func__, (g_silent_mode) ? "Enable" : "Disable");
                break;

            case 'd': /* Debug Option */

                // Set debug
                g_debug = true;
                DEBUG_PRINTF("Debug: %s.\r\n", (g_debug) ? "Enable" : "Disable");
                break;

            case 'h': /* Help */

                // Set debug
                g_help = true;
                DEBUG_PRINTF("Help Information: %s.\r\n", (g_help) ? "Enable" : "Disable");
                break;

            default:
                ERROR_PRINTF("%s: Unknow Command!\r\n", __func__);
                break;
        }
    }

    // Make sure mapping file and system type set at the same time
    if((g_lookup_fwid == true) && (g_system_type == UNKNOWN))
    {
        ERROR_PRINTF("%s: Please Set System Type!\r\n", __func__);
        err = TP_ERR_INVALID_PARAM;
        goto PROCESS_PARAM_EXIT;
    }
    if((g_system_type != UNKNOWN) && (g_lookup_fwid == false))
    {
        ERROR_PRINTF("%s: Please Input FW Mapping File!\r\n", __func__);
        err = TP_ERR_INVALID_PARAM;
        goto PROCESS_PARAM_EXIT;
    }

    return TP_SUCCESS;

PROCESS_PARAM_EXIT:
    DEBUG_PRINTF("[ELAN] ParserCmd: Exit because of an error occurred, err=0x%x.\r\n", err);
    return err;
}

/*******************************************
 * Main Function
 ******************************************/

int main(int argc, char **argv)
{
    int err = TP_SUCCESS;
    bool recovery = false,		// True if Recovery Mode
         gen8_touch = false,	// True if Gen8 Touch
         edid_info_found = true;
    unsigned char hello_packet = 0;
    unsigned short fw_bc_version = 0,
                   bc_bc_version = 0,
                   fwid = 0,
                   info_fwid = 0, /* Information FWID: FWID from Information ROM */
                   fwid_from_edid = 0,
                   edid_manufacturer_code = 0,
                   edid_product_code = 0;
    struct hidraw_devinfo hid_dev_info[DEV_INFO_SET_MAX];
    struct lcm_dev_info   lcm_panel_info[DEV_INFO_SET_MAX];

    // Initialize Data Variables
    memset(hid_dev_info, 0, sizeof(hid_dev_info));
    memset(lcm_panel_info, 0, sizeof(lcm_panel_info));

    /* Process Parameter */
    err = process_parameter(argc, argv);
    if (err != TP_SUCCESS)
    {
        goto EXIT;
    }

    if (g_silent_mode == false) // Normal Mode
        printf("i2chid_read_fwid v%s %s.\r\n", ELAN_TOOL_SW_VERSION, ELAN_TOOL_SW_RELEASE_DATE);

    /* Show Help Information */
    if(g_help == true)
    {
        show_help_information();
        goto EXIT;
    }

    /* Initialize Resource */
    err = resource_init();
    if (err != TP_SUCCESS)
        goto EXIT1;

    /* Open Device */
    err = open_device() ;
    if (err != TP_SUCCESS)
    {
        ERROR_PRINTF("Fail to Open Device! err=0x%x.\r\n", err);
        goto EXIT2;
    }

    /* Detect Touch State */

    // Get Hello Packet
    err = get_hello_packet_bc_version_with_error_retry(&hello_packet, &bc_bc_version, ERROR_RETRY_COUNT);
    if(err != TP_SUCCESS)
    {
        ERROR_PRINTF("Fail to Get Hello Packet (& BC Version)! err=0x%x.\r\n", err);
        goto EXIT2;
    }
    DEBUG_PRINTF("Hello Packet: 0x%02x, Recovery Mode BC Version: 0x%04x.\r\n", hello_packet, bc_bc_version);

    // Identify HW Series & Touch State
    switch (hello_packet)
    {
        case ELAN_I2CHID_NORMAL_MODE_HELLO_PACKET:
            // BC Version (Normal Mode)
            err = get_boot_code_version(&fw_bc_version);
            if(err != TP_SUCCESS)
            {
                ERROR_PRINTF("%s: Fail to Get BC Version (Normal Mode)! err=0x%x.\r\n", __func__, err);
                goto EXIT2;
            }
            DEBUG_PRINTF("Normal Mode BC Version: 0x%04x.\r\n", fw_bc_version);

            // Special Case: First BC of EM32F901 / EM32F902
            if((HIGH_BYTE(fw_bc_version) == BC_VER_H_BYTE_FOR_EM32F901_I2CHID) /* EM32F901 */ ||
               (HIGH_BYTE(fw_bc_version) == BC_VER_H_BYTE_FOR_EM32F902_I2CHID) /* FM32F902 */)
                gen8_touch = true;	// Gen8 Touch
            else
                gen8_touch = false;	// Gen5/6/7 Touch
            recovery = false;		// Normal Mode
            break;

        case ELAN_GEN8_I2CHID_NORMAL_MODE_HELLO_PACKET:
            gen8_touch = true;		// Gen8 Touch
            recovery = false;		// Normal Mode
            break;

        case ELAN_I2CHID_RECOVERY_MODE_HELLO_PACKET:
            // Special Case: First BC of EM32F901 / EM32F902
            if((HIGH_BYTE(bc_bc_version) == BC_VER_H_BYTE_FOR_EM32F901_I2CHID) /* EM32F901 */ ||
               (HIGH_BYTE(bc_bc_version) == BC_VER_H_BYTE_FOR_EM32F902_I2CHID) /* FM32F902 */)
                gen8_touch = true;	// Gen8 Touch
            else
                gen8_touch = false;	// Gen5/6/7 Touch
            recovery = true;		// Recovery Mode
            break;

        case ELAN_GEN8_I2CHID_RECOVERY_MODE_HELLO_PACKET:
            gen8_touch = true;		// Gen8 Touch
            recovery = true;		// Recovery Mode
            break;

        default:
            ERROR_PRINTF("%s: Unknown Hello Packet! (0x%02x) \r\n", __func__, hello_packet);
            err = TP_UNKNOWN_DEVICE_TYPE;
            goto EXIT2;
    }

    // Check if Recovery Mode
    if(recovery == true)
    {
        if(g_silent_mode == false) // Not in Silent Mode
            printf("In Recovery Mode.\r\n");
    }

    /* Get System Info. */
    err = get_system_info(hid_dev_info, sizeof(hid_dev_info), \
                          &edid_manufacturer_code, &edid_product_code, &edid_info_found, \
                          g_fwid_mapping_file_path, sizeof(g_fwid_mapping_file_path), \
                          lcm_panel_info, sizeof(lcm_panel_info));
    if (err != TP_SUCCESS)
    {
        ERROR_PRINTF("Fail to Get System Info.! err=0x%x.\r\n", err);
        goto EXIT2;
    }

    /* Read Information FWID */
    if(gen8_touch) // Gen8 Touch
        err = gen8_read_info_fwid(&info_fwid, recovery);
    else // Gen5/6/7 Touch
        err = read_info_fwid(&info_fwid, recovery);
    if (err != TP_SUCCESS)
    {
        ERROR_PRINTF("Fail to Read Information FWID! err=0x%x.\r\n", err);
        goto EXIT2;
    }

    /* Show System Information */
    if(g_show_system_info == true)
    {
        err = show_system_info(hid_dev_info, sizeof(hid_dev_info), \
                               edid_info_found, edid_manufacturer_code, edid_product_code, \
                               g_lookup_fwid, lcm_panel_info, sizeof(lcm_panel_info), \
                               info_fwid);
        if (err != TP_SUCCESS)
        {
            ERROR_PRINTF("%s: Fail to Show System Info.! err=0x%x.\r\n", __func__, err);
            goto EXIT2;
        }
    }

    /* Validate Elan Touchsceen Device */
    if(g_validate_dev == true)
    {
        err = validate_elan_hid_device(hid_dev_info, sizeof(hid_dev_info), g_pid, g_silent_mode);
        if (err != TP_SUCCESS)
        {
            ERROR_PRINTF("%s: Fail to validate elan HID Device (PID: %04x)! err=0x%x.\r\n", __func__, g_pid, err);
            goto EXIT2;
        }
    }

    if(g_lookup_fwid == true) // Lookup FWID has been Requested
    {
        if(edid_info_found == true) // EDID Info. Found
        {
            /* Get FWID from EDID */
            err = get_fwid_from_edid(lcm_panel_info, sizeof(lcm_panel_info), edid_manufacturer_code, edid_product_code, g_system_type, &fwid_from_edid);
            if (err == TP_SUCCESS) // FWID from EDID Found
            {
                DEBUG_PRINTF("fwid_from_edid Found: 0x%x.\r\n", fwid_from_edid);
                fwid = fwid_from_edid;
            }
            else // FWID from EDID Not Found
            {
                if (err == TP_ERR_DATA_NOT_FOUND) // Can not find FWID from Mapping Table
                {
                    // Use Information FWID Instead
                    DEBUG_PRINTF("fwid_from_edid Not Found, use info_fwid (%x) instead.\r\n", info_fwid);
                    fwid = info_fwid;
                }
                else // if ((err != TP_SUCCESS) && (err != TP_ERR_DATA_NOT_FOUND))
                {
                    ERROR_PRINTF("%s: Fail to get %s FWID! err=0x%x.\r\n", __func__, (g_system_type == CHROME) ? "chrome" : "windows", err);
                    goto EXIT2;
                }
            }
        }
        else // EDID Info. Not Found
        {
            // Use Information FWID Instead
            DEBUG_PRINTF("EDID read failed, use info_fwid (%x) instead.\r\n", info_fwid);
            fwid = info_fwid;
        }

        /* Show FWID */
        err = show_fwid(g_system_type, fwid, g_silent_mode);
        if (err != TP_SUCCESS)
        {
            ERROR_PRINTF("%s: Fail to show %s FWID! err=0x%x.\r\n", __func__, (g_system_type == CHROME) ? "chrome" : "windows", err);
            goto EXIT2;
        }
    }

    // Success
    err = TP_SUCCESS;

EXIT2:
    /* Close Device */
    close_device();

EXIT1:
    /* Release Resource */
    resource_free();

EXIT:
    /* End of Output Stream */
    printf("\r\n");

    return err;
}
