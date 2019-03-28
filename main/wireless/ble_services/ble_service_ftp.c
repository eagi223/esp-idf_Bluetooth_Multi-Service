/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/*
BLE-FTP Service

Author - Paul Abbott - 2019
*/

#include "ble_service_ftp.h"
#include "esp_gatts_api.h"

#include "../ble_common.h"
#include "../../main.h"

#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include <sys/errno.h>
#include "esp_err.h"
#include "esp_log.h"

#define TAG "ble ftp"

#define FILE_CHUNK_LEN    (512) // must be less than ESP_GATT_MAX_ATTR_LEN=600
#define MAX_FILENAME_LENGTH (127) // for VFS, not ftp Gatt protocol

//========================================================================
//		ftp GATT Service
//========================================================================

const uint16_t FTP_SERV_uuid 			  = 0xFFFA;
static const uint16_t FTP_DATA_uuid       = 0xFFFB;

#define FTP_ATTR_DESCR_LEN	(20)
static const uint8_t FTP_DATA_descr[] = "FTP Data";

static uint8_t FTP_DATA_config_ccc[2]  = {0x00,0x00};

#define BLE_SINGLE_MAXLENGTH	(20) // BLE command is 20 bytes long
#define BLE_LONG_MAXLENGTH		ESP_GATT_MAX_ATTR_LEN // max length using long read/write

#ifndef BLEFTP_BASE_PATH
#define BLEFTP_BASE_PATH	"/spiflash";
#endif

/* OPCODE: first byte declares the type of packet */
#define FTP_OPCODE_BYTES	(1)
#define FTP_OPCODE_POS		(0)
#define FTP_OPCODE_READ		(0x10)	// Read Request
#define FTP_OPCODE_WRITE	(0x20)	// Write Request
#define FTP_OPCODE_DATAC	(0x01)  // Continuing data
#define FTP_OPCODE_DATAX	(0x03)  // Final data

// XID: Transfer ID
#define FTP_XID_BYTES		(1)
#define FTP_XID_POS			(1)

// FNAME: Filename
#define FTP_FNAME_BYTES		(18) // per the ftp Gatt protocol
#define FTP_FNAME_POS		(2)

// DATA: the data!
#define FTP_DATA_POS		(2)
#define FTP_HEADER_SIZE		(FTP_OPCODE_BYTES + FTP_XID_BYTES)
// size of data can be 0 ~ MTU-3

typedef struct {
	uint8_t                 opcode; // current operation (read or write)
	int                     offset; // read or write offset
	uint8_t                 xid;  // current transfer ID
	char                    fname[MAX_FILENAME_LENGTH]; // filename
	FILE *                  file; // file handle
} ftp_state_t;

static ftp_state_t ftp_state;

//static uint8_t FTP_DATA_testing_file[45] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18};

const esp_gatts_attr_db_t ftp_serv_gatt_db[FTP_SERV_NUM_ATTR] =
{
	[FTP_SERV] = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *) &primary_service_uuid, ESP_GATT_PERM_READ, sizeof(uint16_t), sizeof(FTP_SERV_uuid), (uint8_t *)&FTP_SERV_uuid}},

	[FTP_DATA_CHAR] = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *) &character_declaration_uuid, ESP_GATT_PERM_READ, CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_write_notify}},
    [FTP_DATA_VAL] = {{ESP_GATT_RSP_BY_APP}, {ESP_UUID_LEN_16, (uint8_t *) &FTP_DATA_uuid, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE, BLE_LONG_MAXLENGTH, 0, NULL}},
    [FTP_DATA_DESCR] = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *) &character_description, ESP_GATT_PERM_READ, FTP_ATTR_DESCR_LEN, sizeof(FTP_DATA_descr), (uint8_t *)FTP_DATA_descr}},
    [FTP_DATA_CFG] = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *) &character_client_config_uuid, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE, sizeof(uint16_t), sizeof(FTP_DATA_config_ccc), (uint8_t *)FTP_DATA_config_ccc}},

};

static void stuff_network_uint32(uint8_t * array, uint32_t value, int offset, int bytes)
{
	int from = 0;
	int to = offset+bytes-1;  // work backwards (LSB to MSB)
	for (from = 0; from < bytes; from++)
	{
		array[to] = (value >> (from*8)) & 0xFF;
		to--;
	}
}

// Network byte order is Big-Endian
static uint32_t parse_network_uint32(const uint8_t * array, int offset, int bytes)
{
	uint32_t res = 0;
	int from = offset+bytes-1; // work backwards (LSB to MSB)
	int to = 0;
	for (to = 0; to < bytes; to++)
	{
		res |= (uint32_t)(array[from] & 0xFF) << (to*8);
		from--;
	}
	return res;
}

static int32_t parse_network_int32(const uint8_t * array, int offset, int bytes)
{
	if (bytes == 1) {
		return (int32_t)((uint8_t)parse_network_uint32(array, offset, bytes)); // sign-extend
	}
	if (bytes == 2) {
		return (int32_t)((uint16_t)parse_network_uint32(array, offset, bytes));
	}
	// not support 3-bytes
	if (bytes == 4) {
		return (int32_t)((uint32_t)parse_network_uint32(array, offset, bytes));
	}
	return 0;
}

static esp_err_t createMissingDirectories(char *fullpath) {
	char *string, *tofree;
	int err = ESP_OK;
	tofree = string = strndup(fullpath, MAX_FILENAME_LENGTH); // make a copy because modifies input
	assert(string != NULL);

	int i;
	for(i=0; string[i] != '\0'; i++) // search over all chars in fullpath
	{
		if (i>0 && (string[i] == '\\' || string[i] == '/')) // stop when reached a slash
		{
			struct stat sb;
			char slash = string[i];
			string[i] = '\0';  // replace slash with null terminator temporarily
			if (stat(string, &sb) != 0) { // stat() will tell us if it is a file or directory or neither.
				//printf("stat failed.\n");
				if (errno == ENOENT)  /* No such file or directory */
				{
					// Create directory
					int e = mkdir(string, S_IRWXU);
					if (e != 0)
					{
						// if command not supported, it probably means you are using SPIFFS without directory support, that's OK, proceed.
						if (errno == ENOTSUP) break; /* Command not supported? */
						ESP_LOGE(__func__, "mkdir failed; errno=%d",errno);
						err = ESP_FAIL;
						break;
					}
					else
					{
						ESP_LOGI(__func__, "created the directory %s",string);
					}
				}
		   }
		   string[i] = slash; // replace the slash and keep searching fullpath
	   }
	}
	free(tofree);  // don't skip this or memory-leak!
	return err;
}

static size_t getFilesize(FILE *fd)
{
	size_t orig = ftell(fd); // save original file pointer
	fseek(fd, 0, SEEK_END); // seek to end of file
	size_t size = ftell(fd); // get current file pointer
	fseek(fd, orig, SEEK_SET); // seek back to original location
	return size;
}

static FILE * findAndOpenFile(const char *fname_from_ptr, const char *basePath, const char *opentype, char *fullpath_buf, int fullpath_maxlen) {

	// basePath specifies where this function is allowed to read from.  It must be specified and can't be empty.
	if ((basePath == NULL) || (*basePath == 0)) {
		ESP_LOGE(TAG, "basePath not found");
		return NULL;
	}
	char *fname_to_ptr = fullpath_buf;
	char *fname_end_ptr	 = fullpath_buf + fullpath_maxlen -1;
	ftp_state.fname[0] = '\0';
	fname_to_ptr += strlcpy(fname_to_ptr, basePath, fname_end_ptr - fname_to_ptr);

	//Filename to get is basePath + '/' + fname.  (fname has already been stripped of leading /)
	// Ensure basePath has a trailing slash
	if (!(*(fname_to_ptr-1) == '\\' || *(fname_to_ptr-1) == '/')) // check last char in string for a slash
	{
		fname_to_ptr += strlcpy(fname_to_ptr, "/", fname_end_ptr - fname_to_ptr);
	}

	// Ensure fname has no leading slash
	if (*fname_from_ptr == '\\' || *fname_from_ptr == '/') // check first char in string for a slash
	{
		fname_from_ptr++; // remove the leading slash
	}
	fname_to_ptr += strlcpy(fname_to_ptr, fname_from_ptr, fname_end_ptr - fname_to_ptr);
	if (fname_end_ptr == fname_to_ptr) {
		ESP_LOGW(TAG, "filename might be truncated: %s", fullpath_buf );
	}

	if (opentype[0] != 'r') // file should be created if not found (so not "r" or "r+")
	{
		// Create missing directories
		if (createMissingDirectories(ftp_state.fname) != ESP_OK)
		{
			return NULL;
		}
	}

	return fopen(ftp_state.fname, opentype);
}

uint16_t getAttributeIndexByFtpHandle(uint16_t attributeHandle)
{
	// Get the attribute index in the attribute table by the returned handle

    uint16_t attrIndex = FTP_SERV_NUM_ATTR;
    uint16_t index;

    for(index = 0; index < FTP_SERV_NUM_ATTR; ++index)
    {
        if( ftp_handle_table[index] == attributeHandle )
        {
            attrIndex = index;
            break;
        }
    }

    return attrIndex;
}

//Handle any reads of characteristics here
void handleFtpReadEvent(int attrIndex, esp_ble_gatts_cb_param_t* param, esp_gatt_rsp_t* rsp)
{
    switch( attrIndex )
    {

    case FTP_DATA_VAL:
    	/*
    	 *  Handle reads to FTP data characteristic value
    	 */
    	memset(rsp->attr_value.value, 0, sizeof(rsp->attr_value.value));  // clear the rsp value first
    	rsp->attr_value.len = 0;

    	if (ftp_state.opcode == FTP_OPCODE_READ)  // previous command was READ
    	{
    		if (ftp_state.file == NULL) {
				ESP_LOGE(TAG, "file not open");
				return;
    		}
    		uint8_t ret_opcode;

    		int max_write_size = FILE_CHUNK_LEN;

    		int fread_len = fread(rsp->attr_value.value + FTP_DATA_POS, 1, max_write_size, ftp_state.file);
    		ftp_state.offset += fread_len;

    		if (fread_len <= 0) {
    			ESP_LOGE(TAG, "file empty");
    			return;
    		}
    		if (fread_len >= max_write_size) {
    			fread_len = max_write_size;
    			ret_opcode = FTP_OPCODE_DATAC; // data will continue after this
    			ESP_LOGI(TAG, "Reading continuing xid: %u offset: %d", ftp_state.xid, ftp_state.offset);
    		} else {
    			ret_opcode = FTP_OPCODE_DATAX; // no more data after this
    			ESP_LOGI(TAG, "Reading final xid: %u", ftp_state.xid);
    			//Finished. Clean up.
    			if(ftp_state.file != NULL){
    				fclose(ftp_state.file);
    				ftp_state.file = NULL;
    				ESP_LOGI(__func__, "fclose: %s, r", ftp_state.fname);
    			}
    		}

    		stuff_network_uint32(rsp->attr_value.value, ret_opcode, FTP_OPCODE_POS, FTP_OPCODE_BYTES); // stuff return opcode
    		stuff_network_uint32(rsp->attr_value.value, ftp_state.xid, FTP_XID_POS, FTP_XID_BYTES); // stuff transfer ID

    	    rsp->attr_value.len = fread_len + FTP_HEADER_SIZE;
    	}

	    break;
    }
}

//Handle any writes to characteristics here
void handleFtpWriteEvent(int attrIndex, const uint8_t *char_val_p, uint16_t char_val_len)
{
	switch( attrIndex )
	{

	case FTP_DATA_VAL:
	/*
	 *  Handle writes to FTP data characteristic value
	 */
		{
			uint32_t opcode, xid;

			if (char_val_len < FTP_HEADER_SIZE) return;  // must at least contain opcode and XID fields!

			opcode = parse_network_uint32(char_val_p, FTP_OPCODE_POS, FTP_OPCODE_BYTES);
			xid = parse_network_uint32(char_val_p, FTP_XID_POS, FTP_XID_BYTES);

			switch (opcode)
			{
			case FTP_OPCODE_READ: // Read Request
			case FTP_OPCODE_WRITE: // Write Request
			{
    			if(ftp_state.file != NULL){
    				fclose(ftp_state.file);
    				ftp_state.file = NULL;
    				ESP_LOGW(__func__, "file was still open: %s", ftp_state.fname);
    			}

				const char* fname_from_ptr = (const char*)(char_val_p) + FTP_FNAME_POS;
				const char* basePath = BLEFTP_BASE_PATH;
				const char* opentype = (opcode == FTP_OPCODE_WRITE)?("w"):("r");
				// try to open the file for reading or writing
				ftp_state.file = findAndOpenFile(fname_from_ptr, basePath, opentype, ftp_state.fname, MAX_FILENAME_LENGTH);
#ifdef BLEFTP_BASE_PATH2
				if (ftp_state.file == NULL) {
					if (opcode == FTP_OPCODE_READ) {  // only attempt 2nd try for read
						basePath = BLEFTP_BASE_PATH2;
						// try to open the file for read-only
						ftp_state.file = findAndOpenFile(fname_from_ptr, basePath, opentype, ftp_state.fname, MAX_FILENAME_LENGTH);
					}
				}
#endif // BLEFTP_BASE_PATH2
				if (ftp_state.file == NULL) {
					ESP_LOGE(TAG, "file open failed: %s", ftp_state.fname);
					return;
				}
				ftp_state.opcode = opcode;
				ftp_state.xid = xid;
				ftp_state.offset = 0;
				ESP_LOGI(TAG, "%s Request xid: %u fname: %s", opentype, ftp_state.xid, ftp_state.fname);
			}
				break;
			case FTP_OPCODE_DATAC: // Continuing data
			case FTP_OPCODE_DATAX: // Final data
	    		if (ftp_state.file == NULL) {
					ESP_LOGE(TAG, "file not open");
					return;
	    		}
				if (ftp_state.opcode != FTP_OPCODE_WRITE){
					ESP_LOGE(TAG, "ftp_state.opcode != FTP_OPCODE_WRITE");
					return;
				}
				if (ftp_state.xid != xid) {
					ESP_LOGE(TAG, "ftp_state.xid != xid");
					return;
				}

				//ESP_LOGI(TAG, "DATA to write: ");
				//ESP_LOG_BUFFER_HEX_LEVEL(TAG, param->write.value + FTP_DATA_POS, param->write.len - FTP_DATA_POS, ESP_LOG_INFO);

				int count = fwrite(char_val_p + FTP_DATA_POS, 1, char_val_len - FTP_DATA_POS, ftp_state.file);

				ftp_state.offset += count; // update offset (like a write pointer)

				ESP_LOGI(TAG, "Write %s xid: %u offset: %d", (opcode == FTP_OPCODE_DATAC)?("continuing"):("final"), ftp_state.xid, ftp_state.offset);
				if (opcode == FTP_OPCODE_DATAX) {
					//Finished. Clean up.
					if(ftp_state.file != NULL){
						fclose(ftp_state.file);
						ftp_state.file = NULL;
						ESP_LOGI(__func__, "fclose: %s", ftp_state.fname);
					}
				}

				break;


			}
		}

	break;
	}
}
