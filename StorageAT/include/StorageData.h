/* Copyright © 2023 Georgy E. All rights reserved. */

#pragma once


#include "StoragePage.h"
#include "StorageType.h"


class StorageData
{
private:
	uint32_t m_startAddress;

	StorageStatus isEmptyAddress(uint32_t address);

public:
	StorageData(uint32_t startAddress);

	StorageStatus load(uint8_t* data, uint32_t len);
	StorageStatus save(
		uint8_t  prefix[Page::PREFIX_SIZE],
		uint32_t id,
		uint8_t* data,
		uint32_t len
	);
	StorageStatus rewrite(
		uint8_t  prefix[Page::PREFIX_SIZE],
		uint32_t id,
		uint8_t* data,
		uint32_t len
	);
	StorageStatus deleteData();
	
	static StorageStatus findStartAddress(uint32_t* address);
	static StorageStatus findEndAddress(uint32_t* address);
};