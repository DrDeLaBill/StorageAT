/* Copyright © 2023 Georgy E. All rights reserved. */

#ifndef STORAGE_TYPE_HPP
#define STORAGE_TYPE_HPP


#include <stdint.h>


typedef enum _StorageStatus {
	STORAGE_OK        = ((uint8_t)0b00000001),
	STORAGE_ERROR     = ((uint8_t)0b00000010),
	STORAGE_BUSY      = ((uint8_t)0b00000100),
	STORAGE_OOM       = ((uint8_t)0b00001000),
	STORAGE_NOT_FOUND = ((uint8_t)0b00010000),
} StorageStatus;


typedef enum _StorageFindMode {
	FIND_MODE_EQUAL = ((uint8_t)0x00),
	FIND_MODE_NEXT  = ((uint8_t)0x01),
	FIND_MODE_MIN   = ((uint8_t)0x02),
	FIND_MODE_MAX   = ((uint8_t)0x03),
	FIND_MODE_EMPTY = ((uint8_t)0x04),
} StorageFindMode;


typedef StorageStatus (*StorageDriverCallback) (uint32_t, uint8_t*, uint32_t);


#endif
