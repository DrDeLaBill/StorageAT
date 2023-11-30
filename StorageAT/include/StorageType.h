/* Copyright © 2023 Georgy E. All rights reserved. */

#pragma once


#include <stdint.h>


#ifndef STORAGE_PACK
#ifdef __GNUC__
#define STORAGE_PACK( __Type__, __Declaration__ )  __Type__ __attribute__((__packed__)) __Declaration__
#elif _MSC_VER
#define STORAGE_PACK( __Type__, __Declaration__ ) __pragma(pack(push, 1) ) __Type__ __Declaration__ __pragma(pack(pop))
#endif
#endif


/* 
 * StorageAT method exit codes 
 */
typedef enum _StorageStatus {
	STORAGE_OK          = static_cast<uint8_t>(0x00), // Successful exit code
	STORAGE_ERROR       = static_cast<uint8_t>(0x01), // Internal error
	STORAGE_BUSY        = static_cast<uint8_t>(0x02), // Physical drive is busy
	STORAGE_OOM         = static_cast<uint8_t>(0x03), // Out of memory
	STORAGE_NOT_FOUND   = static_cast<uint8_t>(0x04), // Data was not found on physical drive
	STORAGE_DATA_EXISTS = static_cast<uint8_t>(0x05), // Data already exists on current address
} StorageStatus;


/*
 * StorageAT find modes
 */
typedef enum _StorageFindMode {
	FIND_MODE_EQUAL   = static_cast<uint8_t>(0x01), // Search equal prefix and id
	FIND_MODE_NEXT    = static_cast<uint8_t>(0x02), // Search equal prefix and next id
	FIND_MODE_MIN     = static_cast<uint8_t>(0x03), // Search equal prefix and min id
	FIND_MODE_MAX     = static_cast<uint8_t>(0x04), // Search equal prefix and max id
	FIND_MODE_EMPTY   = static_cast<uint8_t>(0x05), // Search empty page
} StorageFindMode;
