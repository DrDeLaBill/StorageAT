/* Copyright © 2023 Georgy E. All rights reserved. */

#include "StorageAT.hpp"

#include <memory>
#include <string.h>
#include <stdint.h>
#include <stddef.h>

#include "StorageData.hpp"
#include "StorageSearch.hpp"
#include "StorageSector.hpp"


using namespace _SAT;


uint32_t StorageAT::m_pagesCount = 0;
StorgeDriverCallback StorageAT::m_readDriver  = NULL;
StorgeDriverCallback StorageAT::m_writeDriver = NULL;


StorageAT::StorageAT(
	uint32_t pagesCount,
	StorgeDriverCallback read_driver,
	StorgeDriverCallback write_driver
) {
	StorageAT::m_pagesCount   = pagesCount;
	StorageAT::m_readDriver  = read_driver;
	StorageAT::m_writeDriver = write_driver;
}

StorageStatus StorageAT::find(
	uint8_t         prefix[Page::STORAGE_PAGE_PREFIX_SIZE],
	uint32_t        id,
	StorageFindMode mode,
	uint32_t*       address
) {
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
	StorageData storageData(address);
	return storageData.save(prefix, id, data, len);
}

StorageStatus StorageAT::deleteData(uint32_t address)
{
	StorageData storageData(address);
	return storageData.deleteData();
}

uint32_t StorageAT::getPagesCount()
{
	return m_pagesCount;
}

uint32_t StorageAT::getBytesSize()
{
	return StorageAT::getPagesCount() * Page::STORAGE_PAGE_SIZE;
}

StorgeDriverCallback StorageAT::readCallback()
{
	return StorageAT::m_readDriver;
}

StorgeDriverCallback StorageAT::writeCallback()
{
	return StorageAT::m_writeDriver;
}
