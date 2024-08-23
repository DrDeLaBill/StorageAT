/* Copyright © 2023 Georgy E. All rights reserved. */

#ifndef _STORAGE_TYPE_H_
#define _STORAGE_TYPE_H_


#ifdef __cplusplus
extern "C" {
#endif


#include <stdint.h>


#ifndef STORAGE_PACK
#   ifdef __GNUC__
#       define STORAGE_PACK( __Type__, __Declaration__ )  __Type__ __attribute__((__packed__)) __Declaration__
#   elif _MSC_VER
#       define STORAGE_PACK( __Type__, __Declaration__ ) __pragma(pack(push, 1) ) __Type__ __Declaration__ __pragma(pack(pop))
#   endif
#endif


/* 
 * StorageAT method exit codes 
 */
typedef enum _StorageStatus {
	STORAGE_OK          = (0x00), // Successful exit code
	STORAGE_ERROR       = (0x01), // Internal error
	STORAGE_BUSY        = (0x02), // Physical drive is busy
	STORAGE_OOM         = (0x03), // Out of memory
	STORAGE_NOT_FOUND   = (0x04), // Data was not found on physical drive
	STORAGE_DATA_EXISTS = (0x05), // Data already exists on current address
} StorageStatus;


/*
 * StorageAT find modes
 */
typedef enum _StorageFindMode {
	FIND_MODE_EQUAL   = (0x01), // Search equal prefix and id
	FIND_MODE_NEXT    = (0x02), // Search equal prefix and next id
	FIND_MODE_MIN     = (0x03), // Search equal prefix and min id
	FIND_MODE_MAX     = (0x04), // Search equal prefix and max id
	FIND_MODE_EMPTY   = (0x05), // Search empty page
} StorageFindMode;


/* Data storage page size in bytes */
#define STORAGE_PAGE_SIZE              (256)

/* Page structure validator */
#define STORAGE_MAGIC                  (0xBEDAC0DE)

/* Current page structure version */
#define STORAGE_VERSION                (0x06)

/* Available page title bytes in block header */
#define STORAGE_PAGE_PREFIX_SIZE       (3)

/* Storage AT default minimal erase size of the memory sector */
#define STORAGE_DEFAULT_MIN_ERASE_SIZE (4096)


/* Packed page header meta data structure */
STORAGE_PACK(typedef struct, _PageMeta {
	 // Special code
    uint32_t magic;
    // StorageAT library version
    uint8_t  version;
    // Previously data address
    uint32_t prev_addr;
    // Next data address
    uint32_t next_addr;
    // String page prefix for searching
    uint8_t  prefix[STORAGE_PAGE_PREFIX_SIZE];
    // ID for searching
    uint32_t id;
} PageMeta);


/* Available payload bytes in page structure */
#define STORAGE_PAGE_PAYLOAD_SIZE (STORAGE_PAGE_SIZE - sizeof(struct _PageMeta) - sizeof(uint16_t))

/* Page structure */
STORAGE_PACK(typedef struct, _PageStruct {
	// Page meta data
    PageMeta header;
    // User payload data
    uint8_t  payload[STORAGE_PAGE_PAYLOAD_SIZE];
    // Page CRC16
    uint16_t crc;
} PageStruct);


#ifdef __cplusplus
}
#endif


#endif
