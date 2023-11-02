/* Copyright © 2023 Georgy E. All rights reserved. */

#include "StorageSector.h"

#include <string.h>
#include <stdint.h>

#include "StorageAT.h"
#include "StoragePage.h"
#include "StorageType.h"
#include "StorageSearch.h"


typedef StorageAT FS;


uint32_t StorageSector::getSectorStartAdderss(uint32_t sectorIndex)
{
	return SECTOR_PAGES_COUNT * sectorIndex * Page::STORAGE_PAGE_SIZE;
}

uint32_t StorageSector::getSectorIndex(uint32_t sectorAddress)
{
	return sectorAddress / Page::STORAGE_PAGE_SIZE / SECTOR_PAGES_COUNT;
}

uint32_t StorageSector::getSectorsCount()
{
	return FS::getPagesCount() / SECTOR_PAGES_COUNT;
}

uint32_t StorageSector::getPageAddressByIndex(uint32_t sectorIndex, uint32_t pageIndex)
{
	return getSectorStartAdderss(sectorIndex) + (SECTOR_RESERVED_PAGES_COUNT + pageIndex) * Page::STORAGE_PAGE_SIZE;
}

uint32_t StorageSector::getPageIndexByAddress(uint32_t address)
{
	if (StorageSector::isSectorAddress(address)) {
		return 0;
	}
    return ((address / Page::STORAGE_PAGE_SIZE) % (SECTOR_PAGES_COUNT)) - SECTOR_RESERVED_PAGES_COUNT;
}

bool StorageSector::isSectorAddress(uint32_t address)
{
	return ((address / Page::STORAGE_PAGE_SIZE) % (SECTOR_PAGES_COUNT)) < SECTOR_RESERVED_PAGES_COUNT;
}

StorageStatus StorageSector::formatSector(uint32_t sectorIndex)
{
	Header header(StorageSector::getSectorStartAdderss(sectorIndex));
	StorageStatus status = StorageSector::loadHeader(&header);
	if (status != STORAGE_OK) {
		return status;
	}

	memset(reinterpret_cast<void*>(header.data), 0, Page::STORAGE_PAGE_PAYLOAD_SIZE);
	status = header.save();
	return status;
}

StorageStatus StorageSector::loadHeader(Header *header)
{
	if (header->address > FS::getBytesSize()) {
		return STORAGE_OOM;
	}

	StorageStatus status = header->load();
	if (status == STORAGE_BUSY) {
		return STORAGE_BUSY;
	}
	if (status == STORAGE_OK) {
		return STORAGE_OK;
	}

	status = header->createHeader();
	if (status == STORAGE_BUSY) {
		return STORAGE_BUSY;
	}
	if (status == STORAGE_OK) {
		return STORAGE_OK;
	}

	return STORAGE_ERROR;
}
