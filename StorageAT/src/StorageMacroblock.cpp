/* Copyright © 2023 Georgy E. All rights reserved. */

#include "StorageMacroblock.h"

#include <string.h>
#include <stdint.h>

#include "StorageAT.h"
#include "StoragePage.h"
#include "StorageType.h"
#include "StorageSearch.h"


typedef StorageAT AT;


uint32_t StorageMacroblock::getMacroblockAddress(uint32_t macroblockIndex)
{
    return macroblockIndex * STORAGE_PAGE_SIZE * StorageMacroblock::PAGES_COUNT;
}

uint32_t StorageMacroblock::getMacroblockIndex(uint32_t macroblockAddress)
{
    return macroblockAddress / STORAGE_PAGE_SIZE / StorageMacroblock::PAGES_COUNT;
}

uint32_t StorageMacroblock::getMacroblocksCount()
{
    return AT::getStoragePagesCount() / PAGES_COUNT;
}

uint32_t StorageMacroblock::getPageAddressByIndex(uint32_t macroblockIndex, uint32_t pageIndex)
{
    return getMacroblockAddress(macroblockIndex) + (RESERVED_PAGES_COUNT + pageIndex) * STORAGE_PAGE_SIZE;
}

uint32_t StorageMacroblock::getPageIndexByAddress(uint32_t address)
{
    if (StorageMacroblock::isMacroblockAddress(address)) {
        return 0;
    }
    return ((address / STORAGE_PAGE_SIZE) % (PAGES_COUNT)) - RESERVED_PAGES_COUNT;
}

bool StorageMacroblock::isMacroblockAddress(uint32_t address)
{
    return ((address / STORAGE_PAGE_SIZE) % (PAGES_COUNT)) < RESERVED_PAGES_COUNT;
}

StorageStatus StorageMacroblock::formatMacroblock(uint32_t macroblockIndex)
{
    Header header(StorageMacroblock::getMacroblockAddress(macroblockIndex));
    StorageStatus status = StorageMacroblock::loadHeader(&header);
    if (!storage_at_data_success(status)) {
        return status;
    }

    Header::MetaUnit* metaUnitPtr = header.data->metaUnits;
    for (uint32_t pageIndex = 0; pageIndex < Header::PAGES_COUNT; pageIndex++, metaUnitPtr++) {
        memset((*metaUnitPtr).prefix, 0, STORAGE_PAGE_PREFIX_SIZE);
        (*metaUnitPtr).id = 0;
        if (!header.isPageStatus(pageIndex, Header::PAGE_BLOCKED)) {
            header.setPageStatus(pageIndex, Header::PAGE_EMPTY);
        }
    }
    status = header.save();
    if (status == STORAGE_HEADER_ERROR) {
		unsigned count = 0;
		uint32_t addresess[Header::PAGES_COUNT] = {};
        for (uint32_t i = 0; i < Header::PAGES_COUNT; i++) {
            addresess[count++] = StorageMacroblock::getPageAddressByIndex(macroblockIndex, i);
        }
		status = StorageAT::driverCallback()->erase(addresess, count);
		if (storage_at_data_success(status)) {
			status = STORAGE_OK;
		}
    }
    if (storage_at_data_success(status)) {
    	return STORAGE_OK;
    }
    return status;
}

StorageStatus StorageMacroblock::loadHeader(Header* header)
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

    return header->create();
}
