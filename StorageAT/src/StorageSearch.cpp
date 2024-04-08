#include "StorageSearch.h"

#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "StoragePage.h"
#include "StorageMacroblock.h"


StorageStatus StorageSearchBase::searchPageAddress(
    const uint8_t  prefix[STORAGE_PAGE_PREFIX_SIZE],
    const uint32_t id,
    uint32_t*      resAddress
) {
    uint32_t macroblockIndex = StorageMacroblock::getMacroblockIndex(this->startSearchAddress);
    this->prevId = getStartCmpId();
    this->foundOnce = false;

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

    if (this->foundOnce) {
        *resAddress = this->prevAddress;
        return STORAGE_OK;
    }

    return STORAGE_NOT_FOUND;
}

StorageStatus StorageSearchBase::searchPageAddressInMacroblock(
    Header*        header,
    const uint8_t  prefix[STORAGE_PAGE_PREFIX_SIZE],
    const uint32_t id
) {
    uint32_t pageIndex = StorageMacroblock::getPageIndexByAddress(this->startSearchAddress);
    this->foundInMacroblock = false;

    Header::MetaUnit *metUnitPtr = header->data->metaUnits;
    for (; pageIndex < Header::PAGES_COUNT; pageIndex++, metUnitPtr++) {
        if (!header->isPageStatus(pageIndex, Header::PAGE_OK)) {
            continue;
        }

        if (header->isPageStatus(pageIndex, Header::PAGE_EMPTY)) {
            break;
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

        this->foundOnce         = true;
        this->foundInMacroblock = true;
        this->prevId            = (*metUnitPtr).id;
        this->prevAddress       = StorageMacroblock::getPageAddressByIndex(header->getMacroblockIndex(), pageIndex);

        if (isNeededFirstResult()) {
            break;
        }
    }

    return this->foundInMacroblock ? STORAGE_OK : STORAGE_NOT_FOUND;
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
    this->foundInMacroblock = false;
    uint32_t pageIndex = StorageMacroblock::getPageIndexByAddress(this->startSearchAddress);
    Header::MetaUnit* metaUnitPtr = &(header->data->metaUnits[pageIndex]);
    for (; pageIndex < Header::PAGES_COUNT; pageIndex++, metaUnitPtr++) {
        if (header->isPageStatus(pageIndex, Header::PAGE_BLOCKED)) {
            continue;
        }

        uint32_t address = StorageMacroblock::getPageAddressByIndex(header->getMacroblockIndex(), pageIndex);

        if (header->isPageStatus(pageIndex, Header::PAGE_EMPTY)) {
            this->foundOnce     = true;
            this->foundInMacroblock = true;
            this->prevAddress   = address;
            break;
        }
    }

    return this->foundInMacroblock ? STORAGE_OK : STORAGE_NOT_FOUND;
}
