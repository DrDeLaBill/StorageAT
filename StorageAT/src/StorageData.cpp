/* Copyright © 2023 Georgy E. All rights reserved. */

#include "StorageData.h"

#include <cstring>
#include <algorithm>

#include "StoragePage.h"
#include "StorageType.h"
#include "StorageSearch.h"
#include "StorageMacroblock.h"


StorageData::StorageData(uint32_t startAddress): m_startAddress(startAddress) {}

StorageStatus StorageData::load(uint8_t* data, uint32_t len)
{
    Page page(m_startAddress);

    if (StorageMacroblock::isMacroblockAddress(m_startAddress)) {
        return STORAGE_ERROR;
    }

    if (!len) {
        return STORAGE_ERROR;
    }

    StorageStatus status = page.load(/*startPage=*/true);
    if (status != STORAGE_OK) {
        return status;
    }

    uint32_t readLen = 0;
    do {
        uint32_t neededLen = std::min(static_cast<uint32_t>(len - readLen), static_cast<uint32_t>(sizeof(page.page.payload)));

        memcpy(&data[readLen], page.page.payload, neededLen);
        readLen += neededLen;

        status = page.loadNext();
        if (status != STORAGE_OK) {
            break;
        }
    } while (readLen < len);

    if (readLen == len) {
        return STORAGE_OK;
    }

    if (status != STORAGE_OK) {
        this->deleteData(page.page.header.prefix, page.page.header.id);
    }

    return status;
}

StorageStatus StorageData::save(
    uint8_t  prefix[Page::PREFIX_SIZE],
    uint32_t id,
    uint8_t* data,
    uint32_t len
) {
    uint32_t pageAddress = m_startAddress;

    if (StorageMacroblock::isMacroblockAddress(pageAddress)) {
        return STORAGE_ERROR;
    }

    uint32_t checkAddress = pageAddress;
    Header checkHeader(checkAddress);
    StorageStatus status = StorageMacroblock::loadHeader(&checkHeader);
    if (status == STORAGE_BUSY || status == STORAGE_OOM) {
        return status;
    }
    if (!checkHeader.isAddressEmpty(checkAddress) &&
        !checkHeader.isSameMeta(StorageMacroblock::getPageIndexByAddress(checkAddress), prefix, id)
    ) {
        return STORAGE_DATA_EXISTS;
    }
    if (checkHeader.isSameMeta(StorageMacroblock::getPageIndexByAddress(checkAddress), prefix, id)) {
        status = StorageData::findStartAddress(&checkAddress); // TODO: check
    }


    status = this->rewrite(prefix, id, data, len);
    if (status != STORAGE_OK) {
        this->deleteData(prefix, id);
    }
    return status;
}

StorageStatus StorageData::rewrite(
    uint8_t  prefix[Page::PREFIX_SIZE],
    uint32_t id,
    uint8_t* data,
    uint32_t len
) {
    uint32_t pageAddress = m_startAddress;

    if (StorageMacroblock::isMacroblockAddress(pageAddress)) {
        return STORAGE_ERROR;
    }

    StorageStatus status = deleteData(prefix, id);
    if (status != STORAGE_OK) {
    	return status;
    }
    m_startAddress = pageAddress;

    Header header(pageAddress);
    status = StorageMacroblock::loadHeader(&header);
    if (status == STORAGE_BUSY || status == STORAGE_OOM) {
        return status;
    }

    uint32_t curLen = 0;
    uint32_t curAddr = pageAddress;
    uint32_t prevAddr = pageAddress;
    uint32_t macroblockAddress = Page::PAGE_SIZE + 1;
    bool headerLoaded = false;
    // TODO: remove heap variables
    // TODO: previously erase needed memory length
    while (curLen < len) {
        if (curAddr - 1 + Page::PAGE_SIZE > StorageAT::getStorageSize()) {
            return STORAGE_OOM;
        }

        // Initialize page
        Page page(curAddr);
        uint32_t neededLen = std::min(static_cast<uint32_t>(len - curLen), static_cast<uint32_t>(sizeof(page.page.payload)));
        bool isStart = curLen == 0;
        bool isEnd   = curLen + neededLen >= len;

        // Search
        uint32_t nextAddr = 0;
        status = StorageSearchEmpty(/*startSearchAddress=*/curAddr + Page::PAGE_SIZE).searchPageAddress(prefix, id, &nextAddr);
        if (status != STORAGE_OK) {
            nextAddr = curAddr + Page::PAGE_SIZE;
        }
        if (!isEnd && status != STORAGE_OK) {
            break;
        }
        status = STORAGE_OK;

        // Set prev and next pages
        page.setPrevAddress(prevAddr);
        page.setNextAddress(nextAddr);
        if (isStart) {
            page.setPrevAddress(curAddr);
        }
        if (isEnd) {
            page.setNextAddress(curAddr);
        }


        // Check header (and save)
        uint32_t curMacroblockAddress = Header::getMacroblockStartAddress(curAddr);
        if (headerLoaded && macroblockAddress != curMacroblockAddress) {
            status = header.save();
        }
        if (status == STORAGE_BUSY) {
            break;
        }
        if (macroblockAddress != curMacroblockAddress) {
            header = Header(curAddr);
            status = StorageMacroblock::loadHeader(&header);
            macroblockAddress = curMacroblockAddress;
        }
        if (status == STORAGE_BUSY || status == STORAGE_OOM) {
            break;
        }
        if (status == STORAGE_OK) {
        	headerLoaded = true;
        }

        // Save page
        if (status == STORAGE_OK) {
            memcpy(page.page.header.prefix, prefix, Page::PREFIX_SIZE);
            page.page.header.id = id;
            memcpy(page.page.payload, data + curLen, neededLen);
            status = page.save();
        }
        if (status == STORAGE_BUSY) {
            break;
        }

        uint32_t pageIndex = StorageMacroblock::getPageIndexByAddress(curAddr);
        Header::MetaUnit* metaUnitPtr = &(header.data->metaUnits[pageIndex]);
        if (status != STORAGE_OK) {
            header.setAddressBlocked(curAddr);
            curAddr = nextAddr;
            continue;
        }


        // Registrate page in header
        memcpy((*metaUnitPtr).prefix, prefix, Page::PREFIX_SIZE);
        (*metaUnitPtr).id = id;
        header.setPageStatus(pageIndex, Header::PAGE_OK);


        // Update current values
        prevAddr = curAddr;
        curAddr  = nextAddr;
        curLen  += neededLen;
    }

    if (status != STORAGE_OK) {
        return status;
    }

    // If header has not been saved, the data will be available
    header.save();

    return STORAGE_OK;
}


StorageStatus StorageData::deleteData(const uint8_t prefix[Header::PREFIX_SIZE], const uint32_t index)
{
    StorageStatus status = STORAGE_OK;

	for (uint32_t macroblockIndex = 0; macroblockIndex < StorageMacroblock::getMacroblocksCount(); macroblockIndex++) {
		Header header(StorageMacroblock::getMacroblockAddress(macroblockIndex));

		status = StorageMacroblock::loadHeader(&header);
		if (status == STORAGE_BUSY || status == STORAGE_OOM) {
			return status;
		}

	    Header::MetaUnit *metUnitPtr = header.data->metaUnits;
		for (uint32_t pageIndex = 0; pageIndex < Header::PAGES_COUNT; pageIndex++, metUnitPtr++) {
			if (memcmp((*metUnitPtr).prefix, prefix, Page::PREFIX_SIZE) ||
				(*metUnitPtr).id != index
			) {
				continue;
			}

	        Header::MetaUnit* metaUnitPtr = &(header.data->metaUnits[pageIndex]);
	        memset((*metaUnitPtr).prefix, 0, Page::PREFIX_SIZE);
	        (*metaUnitPtr).id = 0;
	        header.setPageStatus(pageIndex, Header::PAGE_EMPTY);
		}

		status = header.save();
		if (status != STORAGE_OK) {
            for (uint32_t pageIndex = 0; pageIndex < Header::PAGES_COUNT; pageIndex++, metUnitPtr++) {
                Page page(StorageMacroblock::getPageAddressByIndex(macroblockIndex, pageIndex));
                if (page.load() != STORAGE_OK) {
                    continue;
                }
                if (memcmp(page.page.header.prefix, prefix, Page::PREFIX_SIZE) ||
                    page.page.header.id != index
                ) {
                    continue;
                }
                this->erasePage(page.getAddress());
            }
		}
	}

    return status;
}

StorageStatus StorageData::clearAddress(const uint32_t address)
{
	uint32_t cur_address = address;

    if (StorageMacroblock::isMacroblockAddress(cur_address)) {
    	return STORAGE_ERROR;
    }

	StorageStatus status = StorageData::findStartAddress(&cur_address);
	if (status == STORAGE_BUSY || status == STORAGE_OOM) {
		return status;
	}
	if (status == STORAGE_OK) {
		m_startAddress = cur_address;
	}

	uint32_t curAddress = m_startAddress;
	Page page(curAddress);
	status = page.load();
	if (status != STORAGE_OK) {
		return status;
	}

	return this->deleteData(page.page.header.prefix, page.page.header.id);
}

StorageStatus StorageData::findStartAddress(uint32_t* address)
{
    Page page(*address);
    StorageStatus status = page.load();
    if (status != STORAGE_OK) {
        return status;
    }

    while (!page.isStart()) {
        status = page.loadPrev();
        if (status != STORAGE_OK) {
            break;
        }
    }

    if (status != STORAGE_OK) {
        return status;
    }

    status = page.load(/*startPage=*/true);
    if (status != STORAGE_OK) {
        return status;
    }

    if (!page.isStart()) {
        return STORAGE_NOT_FOUND;
    }

    *address = page.getAddress();

    return STORAGE_OK;
}

StorageStatus StorageData::findEndAddress(uint32_t* address)
{
    Page page(*address);
    StorageStatus status = page.load();
    if (status != STORAGE_OK) {
        return status;
    }

    while (!page.isEnd()) {
        status = page.loadNext();
        if (status != STORAGE_OK) {
            break;
        }
    }

    if (status != STORAGE_OK) {
        return status;
    }

    status = page.load(/*startPage=*/true);
    if (status != STORAGE_OK) {
        return status;
    }

    *address = page.getAddress();

    return STORAGE_OK;
}

StorageStatus StorageData::isEmptyAddress(uint32_t address)
{
    Header header(address);
    StorageStatus status = StorageMacroblock::loadHeader(&header);
    if (status == STORAGE_BUSY || status == STORAGE_OOM) {
        return status;
    }
    if (status == STORAGE_OK && header.isAddressEmpty(address)) {
        return STORAGE_OK;
    }
    Page page(address);
    status = page.load();
    if (status == STORAGE_BUSY) {
        return STORAGE_BUSY;
    }
    if (status == STORAGE_OOM) {
        return STORAGE_DATA_EXISTS;
    }
    if (status != STORAGE_OK) {
        return STORAGE_OK;
    }
    return STORAGE_DATA_EXISTS;
}

StorageStatus StorageData::erasePage(const uint32_t address)
{
    if (StorageMacroblock::isMacroblockAddress(address)) {
        return STORAGE_ERROR;
    }

    Page page(address);
    memset(reinterpret_cast<void*>(&page.page), 0xFF, sizeof(page.page));

    return page.save();
}
