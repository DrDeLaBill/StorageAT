#include "StorageSearch.h"

#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "StoragePage.h"
#include "StorageMacroblock.h"


uint32_t StorageSearchBase::startSearchAddress = 0;
bool     StorageSearchBase::foundOnce          = false;
bool     StorageSearchBase::foundInMacroblock  = false;
uint32_t StorageSearchBase::prevAddress        = 0;
uint32_t StorageSearchBase::prevId             = 0;


StorageSearchBase::StorageSearchBase(uint32_t startSearchAddress) 
{
    StorageSearchBase::startSearchAddress = startSearchAddress;
}

StorageStatus StorageSearchBase::searchPageAddress(
    const uint8_t  prefix[STORAGE_PAGE_PREFIX_SIZE],
    const uint32_t id,
    uint32_t*      resAddress
) {
    uint32_t macroblockIndex = StorageMacroblock::getMacroblockIndex(startSearchAddress);
    
    prevId      = getStartCmpId();
    foundOnce   = false;
    prevAddress = startSearchAddress;

    for (; macroblockIndex < StorageMacroblock::getMacroblocksCount(); macroblockIndex++) {
        Header header(StorageMacroblock::getMacroblockAddress(macroblockIndex));

        StorageStatus status = StorageMacroblock::loadHeader(&header);
        if (status == STORAGE_BUSY || status == STORAGE_OOM) {
            return status;
        }

        status = this->searchPageAddressInMacroblock(&header, prefix, id);
        if (status == STORAGE_BUSY) {
            return STORAGE_BUSY;
        }
        if (status != STORAGE_OK) {
            continue;
        }

        if (isNeededFirstResult()) {
            break;
        }
    }

    if (foundOnce) {
        *resAddress = prevAddress;
        return STORAGE_OK;
    }

    return STORAGE_NOT_FOUND;
}

StorageStatus StorageSearchBase::searchPageAddressInMacroblock(
    Header*        header,
    const uint8_t  prefix[STORAGE_PAGE_PREFIX_SIZE],
    const uint32_t id
) {
    uint32_t pageIndex = StorageMacroblock::getPageIndexByAddress(startSearchAddress);
    foundInMacroblock = false;

    Header::MetaUnit *metUnitPtr = header->data->metaUnits;
    for (; pageIndex < Header::PAGES_COUNT; pageIndex++, metUnitPtr++) {
        if (header->isAddressBlocked((pageIndex + StorageMacroblock::RESERVED_PAGES_COUNT) * STORAGE_PAGE_SIZE)) {
            continue;
        }

        if (
            strlen(reinterpret_cast<const char*>(prefix)) &&
            memcmp((*metUnitPtr).prefix, prefix, STORAGE_PAGE_PREFIX_SIZE)
        ) {
            continue;
        }

        if (!isIdFound((*metUnitPtr).id, id)) {
            continue;
        }

        Page page(StorageMacroblock::getPageAddressByIndex(header->getMacroblockIndex(), pageIndex));
        StorageStatus status = page.load(/*startPage=*/true);
        if (status != STORAGE_OK) {
            continue;
        }

        foundOnce         = true;
        foundInMacroblock = true;
        prevId            = (*metUnitPtr).id;
        prevAddress       = StorageMacroblock::getPageAddressByIndex(header->getMacroblockIndex(), pageIndex);

        if (isNeededFirstResult()) {
            break;
        }
    }

    return foundInMacroblock ? STORAGE_OK : STORAGE_NOT_FOUND;
}

bool StorageSearchBase::found()
{
    return foundOnce;
}

uint32_t StorageSearchBase::getAddress()
{
    return prevAddress;
}

bool StorageSearchEqual::isIdFound(
    const uint32_t headerId,
    const uint32_t targetId
) {
    return targetId == headerId;
}

bool StorageSearchNext::isIdFound(
    const uint32_t headerId,
    const uint32_t targetId
) {
    if (targetId >= headerId) {
        return false;
    }
    return targetId < prevId && headerId < prevId;
}

bool StorageSearchMin::isIdFound(
    const uint32_t headerId,
    const uint32_t
) {
    return prevId > headerId;
}

bool StorageSearchMax::isIdFound(
    const uint32_t headerId,
    const uint32_t
) {
    return prevId < headerId;
}

StorageStatus StorageSearchEmpty::searchPageAddressInMacroblock(
    Header*        header,
    const uint8_t[STORAGE_PAGE_PREFIX_SIZE],
    const uint32_t
) {
    foundInMacroblock = false;
    uint32_t pageIndex = StorageMacroblock::getPageIndexByAddress(startSearchAddress);
    Header::MetaUnit* metaUnitPtr = &(header->data->metaUnits[pageIndex]);
    for (; pageIndex < Header::PAGES_COUNT; pageIndex++, metaUnitPtr++) {
        if (header->isAddressBlocked((pageIndex + StorageMacroblock::RESERVED_PAGES_COUNT) * STORAGE_PAGE_SIZE)) {
            continue;
        }

        uint32_t address = StorageMacroblock::getPageAddressByIndex(header->getMacroblockIndex(), pageIndex);


        if (header->isAddressEmpty((pageIndex + StorageMacroblock::RESERVED_PAGES_COUNT) * STORAGE_PAGE_SIZE)) {
            foundOnce     = true;
            foundInMacroblock = true;
            prevAddress   = address;
            break;
        }
    }

    return foundInMacroblock ? STORAGE_OK : STORAGE_NOT_FOUND;
}
