/* Copyright © 2023 Georgy E. All rights reserved. */

#include "StorageAT.h"

#include <memory>
#include <string.h>
#include <stdint.h>
#include <stddef.h>

#include "StorageData.h"
#include "StorageType.h"
#include "StorageSearch.h"
#include "StorageSector.h"


uint32_t StorageAT::m_pagesCount = 0;
IStorageDriver* StorageAT::m_driver = nullptr;


StorageAT::StorageAT(
	uint32_t pagesCount,
	IStorageDriver* driver
) {
	StorageAT::m_pagesCount = pagesCount;
	StorageAT::m_driver = driver;
}

StorageStatus StorageAT::find(
	StorageFindMode mode,
	uint32_t*       address,
	uint8_t         prefix[Page::STORAGE_PAGE_PREFIX_SIZE],
	uint32_t        id
) {
	if (!address) {
		return STORAGE_ERROR;
	}

	if (mode != FIND_MODE_EMPTY && !prefix) {
		return STORAGE_ERROR;
	}

	std::unique_ptr<StorageSearchBase> search;

	switch (mode) {
	case FIND_MODE_EQUAL:
		search = std::make_unique<StorageSearchEqual>(/*startSearchAddress=*/0);
		break;
	case FIND_MODE_NEXT:
		search = std::make_unique<StorageSearchNext>(/*startSearchAddress=*/0);
		break;
	case FIND_MODE_MIN:
		search = std::make_unique<StorageSearchMin>(/*startSearchAddress=*/0);
		break;
	case FIND_MODE_MAX:
		search = std::make_unique<StorageSearchMax>(/*startSearchAddress=*/0);
		break;
	case FIND_MODE_EMPTY:
		search = std::make_unique<StorageSearchEmpty>(/*startSearchAddress=*/0);
		break;
	default:
		return STORAGE_ERROR;
	}

	return search->searchPageAddress(prefix, id, address);
}

StorageStatus StorageAT::load(uint32_t address, uint8_t* data, uint32_t len)
{
	if (address % Page::STORAGE_PAGE_SIZE > 0) {
		return STORAGE_ERROR;
	}
	if (!data) {
		return STORAGE_ERROR;
	}
	if (address + len > getBytesSize()) {
		return STORAGE_OOM;
	}
	StorageData storageData(address);
	return storageData.load(data, len);
}

StorageStatus StorageAT::save(
	uint32_t address,
	uint8_t  prefix[Page::STORAGE_PAGE_PREFIX_SIZE],
	uint32_t id,
	uint8_t* data,
	uint32_t len
) {
	if (address % Page::STORAGE_PAGE_SIZE > 0) {
		return STORAGE_ERROR;
	}
	if (!data) {
		return STORAGE_ERROR;
	}
	if (!prefix) {
		return STORAGE_ERROR;
	}
	if (address + len > getBytesSize()) {
		return STORAGE_OOM;
	}
	StorageData storageData(address);
	return storageData.save(prefix, id, data, len);
}

StorageStatus StorageAT::format()
{
	for (unsigned i = 0; i < StorageSector::getSectorsCount(); i++) {
		StorageStatus status = StorageSector::formatSector(i);
		if (status == STORAGE_BUSY) {
			return STORAGE_BUSY;
		}
	}
	return STORAGE_OK;
}

StorageStatus StorageAT::deleteData(uint32_t address)
{
	if (address > getBytesSize()) {
		return STORAGE_OOM;
	}
	StorageData storageData(address);
	return storageData.deleteData();
}

uint32_t StorageAT::getPagesCount()
{
	uint32_t sectorsCount = m_pagesCount / StorageSector::SECTOR_PAGES_COUNT;
	uint32_t pagesCount = sectorsCount * StorageSector::SECTOR_PAGES_COUNT;
	if (m_pagesCount % StorageSector::SECTOR_PAGES_COUNT > StorageSector::SECTOR_RESERVED_PAGES_COUNT) {
		pagesCount += (StorageSector::SECTOR_RESERVED_PAGES_COUNT - (m_pagesCount % StorageSector::SECTOR_PAGES_COUNT));
	}
	return pagesCount;
}

uint32_t StorageAT::getBytesSize()
{
	return StorageAT::getPagesCount() * Page::STORAGE_PAGE_SIZE;
}

IStorageDriver* StorageAT::driverCallback()
{
	return StorageAT::m_driver;
}
