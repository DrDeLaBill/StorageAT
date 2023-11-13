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


typedef enum _StorageStatus {
	STORAGE_OK          = static_cast<uint8_t>(0x00),
	STORAGE_ERROR       = static_cast<uint8_t>(0x01),
	STORAGE_BUSY        = static_cast<uint8_t>(0x02),
	STORAGE_OOM         = static_cast<uint8_t>(0x03),
	STORAGE_NOT_FOUND   = static_cast<uint8_t>(0x04),
	STORAGE_DATA_EXISTS = static_cast<uint8_t>(0x05),
} StorageStatus;


typedef enum _StorageFindMode {
	FIND_MODE_EQUAL   = static_cast<uint8_t>(0x01),
	FIND_MODE_NEXT    = static_cast<uint8_t>(0x02),
	FIND_MODE_MIN     = static_cast<uint8_t>(0x03),
	FIND_MODE_MAX     = static_cast<uint8_t>(0x04),
	FIND_MODE_EMPTY   = static_cast<uint8_t>(0x05),
} StorageFindMode;


typedef StorageStatus (*StorageDriverCallback) (uint32_t, uint8_t*, uint32_t);
