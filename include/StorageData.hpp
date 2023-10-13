/* Copyright © 2023 Georgy E. All rights reserved. */

#ifndef STORAGE_DATA_HPP
#define STORAGE_DATA_HPP


#include "StoragePage.hpp"
#include "StorageType.hpp"


class StorageData
{
private:
	uint32_t m_startAddress;

	StorageStatus findStartAddress(uint32_t* address);

public:
	StorageData(uint32_t startAddress);

	StorageStatus load(uint8_t* data, uint32_t len);
	StorageStatus save(
		uint8_t  prefix[Page::STORAGE_PAGE_PREFIX_SIZE],
		uint32_t id,
		uint8_t* data,
		uint32_t len
	);
	StorageStatus deleteData();
};


#endif
