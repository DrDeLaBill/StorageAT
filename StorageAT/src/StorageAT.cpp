/* Copyright © 2023 Georgy E. All rights reserved. */

#include "StorageAT.h"

#include <memory>
#include <utility>
#include <string.h>
#include <stdint.h>
#include <stddef.h>

#include "StorageData.h"
#include "StorageType.h"
#include "StorageSearch.h"
#include "StorageMacroblock.h"


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
    const char*     prefix,
    uint32_t        id
) {
    if (!address) {
        return STORAGE_ERROR;
    }

    if (mode != FIND_MODE_EMPTY && !prefix) {
        return STORAGE_ERROR;
    }

    uint8_t tmpPrefix[STORAGE_PAGE_PREFIX_SIZE + 1] = { 0 };
    memcpy(tmpPrefix, prefix, std::min(static_cast<size_t>(STORAGE_PAGE_PREFIX_SIZE), strlen(prefix)));

    switch (mode) {
    case FIND_MODE_EQUAL:
        return (StorageSearchEqual(/*startSearchAddress=*/0)).searchPageAddress(tmpPrefix, id, address);
    case FIND_MODE_NEXT:
        return (StorageSearchNext(/*startSearchAddress=*/0)).searchPageAddress(tmpPrefix, id, address);
    case FIND_MODE_MIN:
        return (StorageSearchMin(/*startSearchAddress=*/0)).searchPageAddress(tmpPrefix, id, address);
    case FIND_MODE_MAX:
        return (StorageSearchMax(/*startSearchAddress=*/0)).searchPageAddress(tmpPrefix, id, address);
    case FIND_MODE_EMPTY:
        return (StorageSearchEmpty(/*startSearchAddress=*/0)).searchPageAddress(tmpPrefix, id, address);
    default:
        return STORAGE_ERROR;
    }
}

StorageStatus StorageAT::load(uint32_t address, uint8_t* data, uint32_t len)
{
    if (address % STORAGE_PAGE_SIZE > 0) {
        return STORAGE_ERROR;
    }
    if (!data) {
        return STORAGE_ERROR;
    }
    if (address + len >= StorageAT::getStorageSize()) {
        return STORAGE_OOM;
    }

    StorageData storageData(address);
    return storageData.load(data, len);
}

StorageStatus StorageAT::save(
    uint32_t address,
    const char* prefix,
    uint32_t id,
    uint8_t* data,
    uint32_t len
) {
    if (address % STORAGE_PAGE_SIZE > 0) {
        return STORAGE_ERROR;
    }
    if (!data) {
        return STORAGE_ERROR;
    }
    if (!prefix) {
        return STORAGE_ERROR;
    }
    if (address + len >= StorageAT::getStorageSize()) {
        return STORAGE_OOM;
    }

    uint8_t tmpPrefix[STORAGE_PAGE_PREFIX_SIZE] = {};
    memcpy(tmpPrefix, prefix, std::min(static_cast<size_t>(STORAGE_PAGE_PREFIX_SIZE), strlen(prefix)));

    StorageData storageData(address);
    return storageData.save(tmpPrefix, id, data, len);
}



StorageStatus StorageAT::rewrite(
    uint32_t address,
    const char* prefix,
    uint32_t id,
    uint8_t* data,
    uint32_t len
) {
    if (address % STORAGE_PAGE_SIZE > 0) {
        return STORAGE_ERROR;
    }
    if (!data) {
        return STORAGE_ERROR;
    }
    if (!prefix) {
        return STORAGE_ERROR;
    }
    if (address + len >= StorageAT::getStorageSize()) {
        return STORAGE_OOM;
    }

    uint8_t tmpPrefix[STORAGE_PAGE_PREFIX_SIZE + 1] = {};
    memcpy(tmpPrefix, prefix, std::min(static_cast<size_t>(STORAGE_PAGE_PREFIX_SIZE), strlen(prefix)));

    StorageData storageData(address);
    return storageData.rewrite(tmpPrefix, id, data, len);
}

StorageStatus StorageAT::format()
{
    for (unsigned i = 0; i < StorageMacroblock::getMacroblocksCount(); i++) {
        StorageStatus status = StorageMacroblock::formatMacroblock(i);
        if (status == STORAGE_BUSY) {
            return STORAGE_BUSY;
        }
    }
    return STORAGE_OK;
}

StorageStatus StorageAT::deleteData(const char* prefix, const uint32_t index)
{
    uint8_t tmpPrefix[STORAGE_PAGE_PREFIX_SIZE + 1] = {};
    memcpy(tmpPrefix, prefix, std::min(static_cast<size_t>(STORAGE_PAGE_PREFIX_SIZE), strlen(prefix)));

    return StorageData(0).deleteData(tmpPrefix, index);
}

StorageStatus StorageAT::clearAddress(const uint32_t address)
{
	return StorageData(0).clearAddress(address);
}

uint32_t StorageAT::getStoragePagesCount()
{
    return m_pagesCount;
}

uint32_t StorageAT::getPayloadPagesCount()
{
    uint32_t pagesCount = (getStoragePagesCount() / StorageMacroblock::PAGES_COUNT) * Header::PAGES_COUNT;
    uint32_t lastPagesCount = getStoragePagesCount() % StorageMacroblock::PAGES_COUNT;
    if (lastPagesCount > StorageMacroblock::RESERVED_PAGES_COUNT) {
        pagesCount += (lastPagesCount - StorageMacroblock::RESERVED_PAGES_COUNT);
    }
    return pagesCount;
}

uint32_t StorageAT::getStorageSize()
{
    return StorageAT::getStoragePagesCount() * STORAGE_PAGE_SIZE;
}

uint32_t StorageAT::getPayloadSize()
{
    return StorageAT::getPayloadPagesCount() * STORAGE_PAGE_PAYLOAD_SIZE;
}

IStorageDriver* StorageAT::driverCallback()
{
    return StorageAT::m_driver;
}
