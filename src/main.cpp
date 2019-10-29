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
#include <errno.h>			/* errno */
#include <getopt.h>
#include <time.h>       	/* time_t, struct tm, time, localtime, asctime */
#include <dirent.h>         /* opendir, readdir, closedir */
#include <fcntl.h>      	/* open */
#include <unistd.h>     	/* close */
#include <sys/ioctl.h>  	/* ioctl */
#include <sys/stat.h>
#include <sys/types.h>
#include <linux/input.h>	/* BUS_TYPE */
#include <linux/hidraw.h>	/* hidraw */
#include "ErrCode.h"
#include "I2CHIDLinuxGet.h"
#include "ElanTsFuncUtility.h"

/***************************************************
 * Definitions
 ***************************************************/

// SW Version
#ifndef ELAN_TOOL_SW_VERSION
#define	ELAN_TOOL_SW_VERSION	"0.5"
#endif //ELAN_TOOL_SW_VERSION

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

// Device Information Set
#ifndef DEV_INFO_SET_MAX
#define DEV_INFO_SET_MAX		100
#endif //DEV_INFO_SET_MAX

// Error Retry Count
#ifndef ERROR_RETRY_COUNT
#define ERROR_RETRY_COUNT	3
#endif //ERROR_RETRY_COUNT

// ELAN Default VID
#ifndef ELAN_USB_VID
#define ELAN_USB_VID					0x04F3
#endif //ELAN_USB_VID

// ELAN Recovery PID
#ifndef ELAN_USB_RECOVERY_PID
#define ELAN_USB_RECOVERY_PID			0x0732
#endif //ELAN_USB_RECOVERY_PID

// Path of modetest command
#ifndef MODETEST_CMD
#define MODETEST_CMD	"/usr/bin/modetest"
#endif //MODETEST_CMD

// EDID Header
#ifndef STANDARD_EDID_HEADER
#define STANDARD_EDID_HEADER		"00ffffffffffff00"
#endif //STANDARD_EDID_HEADER

#ifndef AUO_EDID_HEADER
#define AUO_EDID_HEADER             "b36f0200b004ec04"
#endif //AUO_EDID_HEADER

#ifndef BOE_EDID_HEADER
#define BOE_EDID_HEADER             "59700200b0042205"
#endif //BOE_EDID_HEADER

// System Name Length
#ifndef SYSTEM_NAME_LENGTH
#define SYSTEM_NAME_LENGTH	16
#endif //SYSTEM_NAME_LENGTH

/***************************************************
 * Data Structure Declaration
 ***************************************************/

// HID Device Info.
struct hidraw_devinfo   g_hid_dev_info[DEV_INFO_SET_MAX];
struct hidraw_devinfo *gp_hid_dev_info = NULL;

// LCM Device Info.
typedef struct lcm_dev_info
{
    char panel_info[PANEL_INFO_LENGTH_MAX];
	unsigned short chrome_fwid;
	unsigned short windows_fwid;
} DEV_INFO, *PDEV_INFO;

struct lcm_dev_info   g_lcm_dev_info[DEV_INFO_SET_MAX];
struct lcm_dev_info *gp_lcm_dev_info = NULL;

// System Type
enum system_type 
{
	UNKNOWN = 0,
	CHROME,
	WINDOWS
};

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
FILE *g_fd_fwid_mapping_file = NULL;

// Device Inforamtion
bool g_dev_info = false;

// EDID Information
unsigned short g_manufacturer_code = 0,
			   g_product_code = 0;

// System Type
system_type g_system_type = UNKNOWN;

// Silent Mode (Quiet)
bool g_quiet = false;

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

// HID Device Info.
const char *bus_str(int bus);
int get_hid_dev_info(struct hidraw_devinfo *p_hid_dev_info, size_t dev_info_size);

// Validate Elan Device
int validate_elan_hid_device(struct hidraw_devinfo *p_hid_dev_info, size_t dev_info_size, int pid, bool quiet);

// EDID Info.
int get_edid_manufacturer_product_code(unsigned short *p_manufacturer_code, unsigned short *p_product_code);
int show_edid_manufacturer_product_code(unsigned short manufacturer_code, unsigned short product_code);

// FWID Mapping File & LCM Dev Info.
int parse_fwid_mapping_file(FILE *fd_mapping_file, struct lcm_dev_info *dev_info, size_t dev_info_size);
int show_lcm_dev_info(struct lcm_dev_info *p_dev_info, size_t dev_info_size);

// FWID
int get_fwid_from_edid(struct lcm_dev_info *p_dev_info, size_t dev_info_size, unsigned short manufacturer_code, unsigned short product_code, system_type system, unsigned short *p_fwid);
int show_fwid(system_type system, unsigned short fwid, bool quiet);

int get_fwid_from_rom(unsigned short *p_fwid);
int show_fwid_from_rom(unsigned short fwid);

// Firmware Version
int get_firmware_version(unsigned short *p_fw_version);

// Solution ID
int get_solution_id(unsigned char *p_solution_id);

// ROM Data
int get_rom_data(unsigned short addr, unsigned short *p_data, unsigned char solution_id);

// Bulk ROM Data
int get_bulk_rom_data(unsigned short addr, unsigned short *p_data);

// Hello Packet
int get_hello_packet(unsigned char *data);
int get_hello_packet_with_error_retry(unsigned char *data, int retry_count);

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
 * File I/O
 ******************************************/


/*******************************************
 * Function Implementation
 ******************************************/

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
	printf("HID Device:\r\n");

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

int validate_elan_hid_device(struct hidraw_devinfo *p_hid_dev_info, size_t dev_info_size, int pid, bool quiet)
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

	if(quiet == false) // Normal Mode
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
			if(quiet == false) // Normal Mode
				printf("Device exists! (VID: %04x, PID: %04x)\r\n", p_hid_dev_info[index].vendor, p_hid_dev_info[index].product);
			dev_exist = true;
			break;
		}
	}

	if(dev_exist == false)
	{
		if(quiet == false) // Normal Mode
			printf("Device does not exist! (VID: %04x, PID: %04x)\r\n", p_hid_dev_info[index].vendor, p_hid_dev_info[index].product);
	}

	// Success
	err = TP_SUCCESS;

VALIDATE_ELAN_HID_DEVICE_EXIT:
	return err;
}

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

	/* Read the output a line at a time and parse it */
	while (fgets(buf, sizeof(buf)-1, fd_process_pipe) != NULL) 
	{
		//DEBUG_PRINTF("%s", buf);

		/* Search for EDID Header */

        // Pattern 1: Standard EDID Header
		p_edid_header = strstr(buf, STANDARD_EDID_HEADER);
		if(p_edid_header == NULL)
        {
            // Pattern 2: AUO EDID Header
            p_edid_header = strstr(buf, AUO_EDID_HEADER);
            if(p_edid_header == NULL)
            {
                // Pattern 3: BOE EDID Header
                p_edid_header = strstr(buf, BOE_EDID_HEADER);
                if(p_edid_header == NULL)
                {
                    // Not contain of known EDID Header, try to search next line
                    continue;
                }
            }
        }

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

		// Job done, break the loop
		break;
	}

	/* close */
	pclose(fd_process_pipe);

	// Success
	err = TP_SUCCESS;

GET_EDID_MANUFACTURER_PRODUCT_CODE_EXIT:
	return err;
}

int show_edid_manufacturer_product_code(unsigned short manufacturer_code, unsigned short product_code)
{
	printf("--------------------------------------\r\n");
	printf("EDID Information:\r\n");
	printf("Manufacturer Code: %04x.\r\n", manufacturer_code);
	printf("Product Code: %04x.\r\n", product_code);

	return TP_SUCCESS;
}

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
	printf("LCM Device:\r\n");

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
        ERROR_PRINTF("%s: Invalid Parameter! (p_dev_info=0x%p, dev_info_size=%zd, manufacturer_code=0x%x, product_code=0x%x, system=%d, p_fwid=0x%p)\r\n",
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

		DEBUG_PRINTF("%s: [%d] panel_info: \"%s\", chrome_fwid: %04x, windows_fwid: %04x.\r\n", __func__, index, p_dev_info[index].panel_info, p_dev_info[index].chrome_fwid, p_dev_info[index].windows_fwid);
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

// Firmware Version
int get_firmware_version(unsigned short *p_fw_version)
{
	int err = TP_SUCCESS;
    unsigned short fw_version = 0;

    // Check if Parameter Invalid
    if (p_fw_version == NULL)
    {
        ERROR_PRINTF("%s: Invalid Parameter! (p_fw_version=0x%p)\r\n", __func__, p_fw_version);
        err = TP_ERR_INVALID_PARAM;
        goto GET_FIRMWARE_VERSION_EXIT;
    }

    err = send_fw_version_command();
    if(err != TP_SUCCESS)
    {
        ERROR_PRINTF("%s: Fail to Send FW Version Command! errno=0x%x.\r\n", __func__, err);
        goto GET_FIRMWARE_VERSION_EXIT;
    }

    err = get_fw_version_data(&fw_version);
    if(err != TP_SUCCESS)
    {
        ERROR_PRINTF("%s: Fail to Get FW Version Data! errno=0x%x.\r\n", __func__, err);
        goto GET_FIRMWARE_VERSION_EXIT;
    }

    *p_fw_version = fw_version;
    err = TP_SUCCESS;

GET_FIRMWARE_VERSION_EXIT:
	return err;
}

// Solution ID
int get_solution_id(unsigned char *p_solution_id)
{
	int err = TP_SUCCESS;
    unsigned short fw_version = 0;
    unsigned char  solution_id = 0;

    // Check if Parameter Invalid
    if (p_solution_id == NULL)
    {
        ERROR_PRINTF("%s: Invalid Parameter! (p_solution_id=0x%p)\r\n", __func__, p_solution_id);
        err = TP_ERR_INVALID_PARAM;
        goto GET_SOLUTION_ID_EXIT;
    }

    // Get Firmware Version
    err = get_firmware_version(&fw_version);
    if(err != TP_SUCCESS)
    {
        ERROR_PRINTF("%s: Fail to Get FW Version! errno=0x%x.\r\n", __func__, err);
        goto GET_SOLUTION_ID_EXIT;
    }

    // Get Solution ID
    solution_id = (unsigned char)((fw_version & 0xFF00) >> 8);
    DEBUG_PRINTF("Solution ID: %02x.\r\n", solution_id);

    *p_solution_id = solution_id;
    err = TP_SUCCESS;

GET_SOLUTION_ID_EXIT:
	return err;
}

// ROM Data
int get_rom_data(unsigned short addr, unsigned short *p_data, unsigned char solution_id)
{
	int err = TP_SUCCESS;
    unsigned short data = 0;

	// Check if Parameter Invalid
    if (p_data == NULL)
    {
        ERROR_PRINTF("%s: Invalid Parameter! (p_data=0x%p)\r\n", __func__, p_data);
        err = TP_ERR_INVALID_PARAM;
        goto GET_ROM_DATA_EXIT;
    }

    err = send_read_rom_data_command(addr, solution_id);
    if(err != TP_SUCCESS)
    {
        ERROR_PRINTF("%s: Fail to Send Read ROM Data Command (addr=0x%04x, solution_id=0x%02x)! errno=0x%x.\r\n", __func__, addr, solution_id, err);
        goto GET_ROM_DATA_EXIT;
    }

    err = receive_rom_data(&data);
    if(err != TP_SUCCESS)
    {
        ERROR_PRINTF("%s: Fail to Receive ROM Data! errno=0x%x.\r\n", __func__, err);
        goto GET_ROM_DATA_EXIT;
    }

    *p_data = data;
    err = TP_SUCCESS;

GET_ROM_DATA_EXIT:
	return err; 
}

// Bulk ROM Data
int get_bulk_rom_data(unsigned short addr, unsigned short *p_data)
{
	int err = TP_SUCCESS;
    unsigned short data = 0;

	// Check if Parameter Invalid
    if (p_data == NULL)
    {
        ERROR_PRINTF("%s: Invalid Parameter! (p_data=0x%p)\r\n", __func__, p_data);
        err = TP_ERR_INVALID_PARAM;
        goto GET_BULK_ROM_DATA_EXIT;
    }

    err = send_show_bulk_rom_data_command(addr);
    if(err != TP_SUCCESS)
    {
        ERROR_PRINTF("%s: Fail to Send Show Bulk ROM Data Command (addr=0x%04x)! errno=0x%x.\r\n", __func__, addr, err);
        goto GET_BULK_ROM_DATA_EXIT;
    }

    err = receive_bulk_rom_data(&data);
    if(err != TP_SUCCESS)
    {
        ERROR_PRINTF("%s: Fail to Receive Bulk ROM Data! errno=0x%x.\r\n", __func__, err);
        goto GET_BULK_ROM_DATA_EXIT;
    }

    *p_data = data;
    err = TP_SUCCESS;

GET_BULK_ROM_DATA_EXIT:
	return err; 
}

int get_fwid_from_rom(unsigned short *p_fwid)
{
    int err = TP_SUCCESS;
    bool recovery = false; // Recovery Mode
    unsigned char hello_packet = 0,
                  solution_id = 0;
    unsigned short fw_id = 0;

    // Check if Parameter Invalid
    if (p_fwid == NULL)
    {
        ERROR_PRINTF("%s: Invalid Parameter! (p_fwid=0x%p)\r\n", __func__, p_fwid);
        err = TP_ERR_INVALID_PARAM;
        goto GET_FWID_FROM_ROM_EXIT;
    }

    /* Check for Recovery */
	err = get_hello_packet_with_error_retry(&hello_packet, ERROR_RETRY_COUNT);
	if(err != TP_SUCCESS)
	{
		ERROR_PRINTF("Fail to Get Hello Packet! errno=0x%x.\r\n", err);
		goto GET_FWID_FROM_ROM_EXIT;
	}

    // Check if Recovery Mode
	if(hello_packet == ELAN_I2CHID_RECOVERY_MODE_HELLO_PACKET)
    {
		printf("In Recovery Mode.\r\n");
        recovery = true;
    }

    // Get FWID from ROM 
    if(recovery) // Recovery Mode
	{
        // Read FWID from ROM (with bulk rom data command)
        // [Note] Paul @ 20191025
        // Since Read ROM Command only supported by main code,
        //   we can only use Show Bulk ROM Data Command (0x59) in recovery mode (boot code stage).
        err = get_bulk_rom_data(ELAN_INFO_ROM_FWID_MEMORY_ADDR, &fw_id);
        if(err != TP_SUCCESS)
        {
            ERROR_PRINTF("%s: Fail to Get Bulk ROM Data Command! errno=0x%x.\r\n", __func__, err);
            goto GET_FWID_FROM_ROM_EXIT;
        }
	}
    else // Normal Mode
    {
        // Get Solution ID
        err = get_solution_id(&solution_id);
        if(err != TP_SUCCESS)
        {
            ERROR_PRINTF("%s: Fail to Get Solution ID! errno=0x%x.\r\n", __func__, err);
            goto GET_FWID_FROM_ROM_EXIT;
        }

        /* Read FWID from ROM */
        // [Note] Paul @ 20191025
        // In normal mode, just use Read ROM Command (0x96).
        err = get_rom_data(ELAN_INFO_ROM_FWID_MEMORY_ADDR, &fw_id, solution_id);
        if(err != TP_SUCCESS)
        {
            ERROR_PRINTF("%s: Fail to Get ROM Data! errno=0x%x.\r\n", __func__, err);
            goto GET_FWID_FROM_ROM_EXIT;
        }
    }

    *p_fwid = fw_id;
    err = TP_SUCCESS;

GET_FWID_FROM_ROM_EXIT:
	return err; 
}

int show_fwid_from_rom(unsigned short fwid)
{
    int err = TP_SUCCESS;

	printf("--------------------------------------\r\n");
    printf("Information from ROM:\r\n");
    printf("FWID: %04x.\r\n", fwid);    

	return err;
}

int show_fwid(system_type system, unsigned short fwid, bool quiet)
{
	int err = TP_SUCCESS;

	// Check if Parameter Invalid
    if(system == UNKNOWN)
    {
        ERROR_PRINTF("%s: Unknown System Type (%d)!\r\n", __func__, system);
        err = TP_ERR_INVALID_PARAM;
        goto SHOW_FWID_EXIT;
    }

	if(quiet == true)
	{
		printf("%04x", fwid);
	}
	else //if(quiet == false)
	{
		printf("--------------------------------------\r\n");
		printf("System: %s.\r\n", (system == CHROME) ? "Chrome" : "Windows");
		printf("FWID: %04x.\r\n", fwid);
	}

SHOW_FWID_EXIT:
	return err;
}

// Hello Packet
int get_hello_packet(unsigned char *data)
{
	int err = TP_SUCCESS;
	unsigned char hello_packet[4] = {0};

	// Make Sure Page Data Buffer Valid
	if(data == NULL)
	{
		ERROR_PRINTF("%s: NULL Page Data Buffer!\r\n", __func__);
		err = TP_ERR_INVALID_PARAM;
		goto GET_HELLO_PACKET_EXIT;
	}

	// Send 7-bit I2C Slave Address
	err = send_request_hello_packet_command();
	if(err != TP_SUCCESS)
	{
		ERROR_PRINTF("%s: Fail to Send Request Hello Packet Command! errno=0x%x.\r\n", __func__, err);
		goto GET_HELLO_PACKET_EXIT;
	}

	// Receive Hello Packet
	err = read_data(hello_packet, sizeof(hello_packet), ELAN_READ_DATA_TIMEOUT_MSEC);
	if(err == TP_ERR_TIMEOUT)
	{
		DEBUG_PRINTF("%s: Fail to Receive Hello Packet! Timeout!\r\n", __func__);
		goto GET_HELLO_PACKET_EXIT;
	}
	if(err != TP_SUCCESS)
	{
		ERROR_PRINTF("%s: Fail to Receive Hello Packet! errno=0x%x.\r\n", __func__, err);
		goto GET_HELLO_PACKET_EXIT;
	}
	DEBUG_PRINTF("vendor_cmd_data: %02x %02x %02x %02x.\r\n", hello_packet[0], hello_packet[1], hello_packet[2], hello_packet[3]);

	// Copy first byte of Hello Packet to Input Buffer
	*data = hello_packet[0];

	// Success
	err = TP_SUCCESS;

GET_HELLO_PACKET_EXIT:
	return err;
}

int get_hello_packet_with_error_retry(unsigned char *data, int retry_count)
{
	int err = TP_SUCCESS,
		retry_index = 0;

	// Make Sure Retry Count Positive
	if(retry_count <= 0)
		retry_count = 1;

	for(retry_index = 0; retry_index < retry_count; retry_index++)
	{
		err = get_hello_packet(data);
		if(err == TP_SUCCESS)
		{
			// Without any error => Break retry loop and continue.
			break;
		}

		// With Error => Retry at most 3 times 
		DEBUG_PRINTF("%s: [%d/3] Fail to Get Hello Packet! errno=0x%x.\r\n", __func__, retry_index+1, err);
		if(retry_index == 2)
		{
			// Have retried for 3 times and can't fix it => Stop this function 
			ERROR_PRINTF("%s: Fail to Get Hello Packet! errno=0x%x.\r\n", __func__, err);
			goto GET_HELLO_PACKET_WITH_ERROR_RETRY_EXIT;
		}
		else // retry_index = 0, 1
		{
			// wait 50ms
			usleep(50*1000); 

			continue;
		}		
	}

GET_HELLO_PACKET_WITH_ERROR_RETRY_EXIT:
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
    // Connect to Device
    DEBUG_PRINTF("Get I2C-HID Device Handle (VID=0x%x, PID=0x%x).\r\n", ELAN_USB_VID, g_pid);
    err = g_pIntfGet->GetDeviceHandle(ELAN_USB_VID, g_pid);
    if (err != TP_SUCCESS)
        ERROR_PRINTF("Device can't connected! errno=0x%x.\n", err);
    /*********************************/

    return err;
}

int close_device(void)
{
    int err = 0;

    // close opened i2c device; //pseudo function

    /*** example *********************/
    // Release acquired touch device handler
    g_pIntfGet->Close();
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
    // Initialize HID Device Information
	memset(g_hid_dev_info, 0, sizeof(g_hid_dev_info));
	err = get_hid_dev_info(g_hid_dev_info, sizeof(g_hid_dev_info));
	if(err != TP_SUCCESS)
	{
		ERROR_PRINTF("%s: Get HID Dev Info. Fail! errno=0x%x.\r\n", __func__, err);
		goto RESOURCE_INIT_EXIT;
	}

    // Initialize I2C-HID Interface
    g_pIntfGet = new CI2CHIDLinuxGet();
    DEBUG_PRINTF("g_pIntfGet=%p.\n", g_pIntfGet);
    if (g_pIntfGet == NULL)
    {
        ERROR_PRINTF("Fail to initialize I2C-HID Interface!");
        err = TP_ERR_NO_INTERFACE_CREATE;
		goto RESOURCE_INIT_EXIT;
    }

	// Initialize EDID Manufacturer Code / Product Code
	err = get_edid_manufacturer_product_code(&g_manufacturer_code, &g_product_code);
	if (err != TP_SUCCESS)
	{
		ERROR_PRINTF("%s: Fail to get EDID manufacturer code & product code ! errno=0x%x.\r\n", __func__, err);
		goto RESOURCE_INIT_EXIT;
	}

	// Initialize LCM Device Information & FWID Mapping Table
	memset(g_lcm_dev_info, 0, sizeof(g_lcm_dev_info));
	if(strcmp(g_fwid_mapping_file_path, "") != 0) // File path has been configured
	{
		// Open FWID Mapping File
    	g_fd_fwid_mapping_file = fopen(g_fwid_mapping_file_path, "r");
		DEBUG_PRINTF("%s: g_fd_fwid_mapping_file=0x%p.\r\n", __func__, g_fd_fwid_mapping_file);
    	if (g_fd_fwid_mapping_file == NULL)
    	{
        	ERROR_PRINTF("%s: Fail to open FWID mapping table file \"%s\"!\r\n", __func__, g_fwid_mapping_file_path);
        	err = TP_ERR_FILE_NOT_FOUND;
			goto RESOURCE_INIT_EXIT;
    	}

		/* Parse FWID Mapping File */
		err = parse_fwid_mapping_file(g_fd_fwid_mapping_file, g_lcm_dev_info, sizeof(g_lcm_dev_info));
		if (err != TP_SUCCESS)
		{
			ERROR_PRINTF("%s: Fail to parse FWID mapping file, errno=%d.", __func__, err);
			goto RESOURCE_INIT_EXIT;
		}
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

	// Close FWID Mapping File
    if (g_fd_fwid_mapping_file)
    {
        fclose(g_fd_fwid_mapping_file);
		g_fd_fwid_mapping_file = NULL;
		memset(g_fwid_mapping_file_path, 0, sizeof(g_fwid_mapping_file_path));        
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

			case 'i': /* Device Information */

                // Enable Get Device Information
                g_dev_info = true;
                DEBUG_PRINTF("%s: Get Device Inforamtion: %s.\r\n", __func__, (g_dev_info) ? "Enable" : "Disable");
                break;

			case 'q': /* Silent Mode (Quiet) */

                // Enable Silent Mode (Quiet)
                g_quiet = true;
                DEBUG_PRINTF("%s: Silent Mode: %s.\r\n", __func__, (g_quiet) ? "Enable" : "Disable");
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
    DEBUG_PRINTF("[ELAN] ParserCmd: Exit because of an error occurred, errno=0x%x.\r\n", err);
    return err;
}

/*******************************************
 * Main Function
 ******************************************/

int main(int argc, char **argv)
{
    int err = TP_SUCCESS;
    unsigned short fwid = 0,
                   fwid_from_rom = 0;

	/* Process Parameter */
	err = process_parameter(argc, argv);
    if (err != TP_SUCCESS)
        goto EXIT;

	if(g_quiet == false) // Normal Mode
		printf("i2chid_read_fwid v%s.\r\n", ELAN_TOOL_SW_VERSION);

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
		ERROR_PRINTF("Fail to Open Device! errno=0x%x.\r\n", err);
        goto EXIT2;
	}

    /* Get FWID from ROM */
    err = get_fwid_from_rom(&fwid_from_rom);
    if (err != TP_SUCCESS)
	{
		ERROR_PRINTF("Fail to Get FWID from ROM! errno=0x%x.\r\n", err);
        goto EXIT2;
	}

	if(g_dev_info == true)
	{
		/* Show HID Device Information */
		err = show_hid_dev_info(g_hid_dev_info, sizeof(g_hid_dev_info));
		if (err != TP_SUCCESS)
		{
			ERROR_PRINTF("%s: Fail to show elan HID Device! errno=0x%x.\r\n", __func__, err);
        	goto EXIT1;
		}

		/* Show EDID Information */
		show_edid_manufacturer_product_code(g_manufacturer_code, g_product_code);
		
		if(g_lookup_fwid == true)
		{
			/* Show LCM Device Information */
			err = show_lcm_dev_info(g_lcm_dev_info, sizeof(g_lcm_dev_info));
			if (err != TP_SUCCESS)
			{
				ERROR_PRINTF("%s: Fail to show LCM Device Information! errno=0x%x.\r\n", __func__, err);
        		goto EXIT1;
			}
		}

        /* Show FWID from ROM */
        show_fwid_from_rom(fwid_from_rom);
	}

	if(g_validate_dev == true)
	{
		/* Validate Elan Touchsceen Device */
		err = validate_elan_hid_device(g_hid_dev_info, sizeof(g_hid_dev_info), g_pid, g_quiet);
		if (err != TP_SUCCESS)
		{
			ERROR_PRINTF("%s: Fail to validate elan HID Device (PID: %04x)! errno=0x%x.\r\n", __func__, g_pid, err);
        	goto EXIT1;
		}
	}

	if(g_lookup_fwid == true)
	{
		/* Lookup FWID from EDID */
		err = get_fwid_from_edid(g_lcm_dev_info, sizeof(g_lcm_dev_info), g_manufacturer_code, g_product_code, g_system_type, &fwid);
		if (err != TP_SUCCESS)
		{
            if (err == TP_ERR_DATA_NOT_FOUND)
            {
                DEBUG_PRINTF("fwid_from_edid Not Found, use fwid_from_rom (%x) instead.\r\n", fwid_from_rom);
                fwid = fwid_from_rom;
            }
            else // if ((err != TP_SUCCESS) && (err != TP_ERR_DATA_NOT_FOUND))
            {
			    ERROR_PRINTF("%s: Fail to get %s FWID! errno=0x%x.\r\n", __func__, (g_system_type == CHROME) ? "chrome" : "windows", err);
        	    goto EXIT1;
            }
		}

		/* Show FWID */
		err = show_fwid(g_system_type, fwid, g_quiet);
		if (err != TP_SUCCESS)
		{
			ERROR_PRINTF("%s: Fail to show %s FWID! errno=0x%x.\r\n", __func__, (g_system_type == CHROME) ? "chrome" : "windows", err);
        	goto EXIT1;
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
