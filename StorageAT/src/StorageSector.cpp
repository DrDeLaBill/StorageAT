/* Copyright © 2023 Georgy E. All rights reserved. */

#include "StorageSector.h"

#include <string.h>
#include <stdint.h>

#include "StorageAT.h"
#include "StoragePage.h"
#include "StorageType.h"
#include "StorageSearch.h"


typedef StorageAT AT;


uint32_t StorageSector::getSectorAddress(uint32_t sectorIndex)
{
	return PAGES_COUNT * sectorIndex * Page::PAGE_SIZE;
}

uint32_t StorageSector::getSectorIndex(uint32_t address)
{
	return address / Page::PAGE_SIZE / PAGES_COUNT;
}

uint32_t StorageSector::getSectorsCount()
{
	return AT::getStoragePagesCount() / PAGES_COUNT;
}

uint32_t StorageSector::getPageAddressByIndex(uint32_t sectorIndex, uint32_t pageIndex)
{
	return getSectorAddress(sectorIndex) + (RESERVED_PAGES_COUNT + pageIndex) * Page::PAGE_SIZE;
}

uint32_t StorageSector::getPageIndexByAddress(uint32_t address)
{
	if (StorageSector::isSectorAddress(address)) {
		return 0;
	}
    return ((address / Page::PAGE_SIZE) % (PAGES_COUNT)) - RESERVED_PAGES_COUNT;
}

bool StorageSector::isSectorAddress(uint32_t address)
{
	return ((address / Page::PAGE_SIZE) % (PAGES_COUNT)) < RESERVED_PAGES_COUNT;
}

StorageStatus StorageSector::formatSector(uint32_t sectorIndex)
{
	Header header(StorageSector::getSectorAddress(sectorIndex));
	StorageStatus status = StorageSector::loadHeader(&header);
	if (status != STORAGE_OK) {
		return status;
	}

	for (unsigned i = 0; i < Header::PAGES_COUNT; i++) {
		memset(reinterpret_cast<void*>(header.data->pages[i].prefix), 0, Page::PREFIX_SIZE);
		header.data->pages[i].id = 0;
		if (!header.isSetHeaderStatus(i, Header::PAGE_BLOCKED)) {
			header.setHeaderStatus(i, Header::PAGE_EMPTY);
		}
	}
	return header.save();
}

StorageStatus StorageSector::loadHeader(Header *header)
{
	if (header->getAddress() > AT::getStorageSize()) {
		return STORAGE_OOM;
	}

	StorageStatus status = header->load();
	if (status == STORAGE_BUSY || status == STORAGE_OOM) {
		return status;
	}
	if (status == STORAGE_OK) {
		return STORAGE_OK;
	}

	status = header->create();
	if (status == STORAGE_BUSY || status == STORAGE_OOM) {
		return status;
	}
	if (status == STORAGE_OK) {
		return STORAGE_OK;
	}

	return status;
}
