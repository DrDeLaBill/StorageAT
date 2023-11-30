/* Copyright © 2023 Georgy E. All rights reserved. */

#include "StorageData.h"

#include <memory>
#include <algorithm>
#include <string.h>

#include "StoragePage.h"
#include "StorageType.h"
#include "StorageSearch.h"


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
		this->deleteData();
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
		this->deleteData();
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

	uint32_t checkAddress = pageAddress;
	std::unique_ptr<Header> header = std::make_unique<Header>(checkAddress);
	StorageStatus status = StorageMacroblock::loadHeader(header.get());
	if (status == STORAGE_BUSY || status == STORAGE_OOM) {
		return status;
	}
	if (!header->isAddressEmpty(checkAddress)) {
		status = StorageData::findStartAddress(&checkAddress);
	}
	if (status == STORAGE_OK) {
		pageAddress = checkAddress;
	}

	uint32_t curLen = 0;
	uint32_t curAddr = pageAddress;
	uint32_t prevAddr = pageAddress;
	uint32_t macroblockAddress = Page::PAGE_SIZE + 1;
	std::unique_ptr<Page> page;
	while (curLen < len) {
		if (curAddr - 1 + Page::PAGE_SIZE > StorageAT::getStorageSize()) {
			return STORAGE_OOM;
		}

		// Initialize page
		page = std::make_unique<Page>(curAddr);
		uint32_t neededLen = std::min(static_cast<uint32_t>(len - curLen), static_cast<uint32_t>(sizeof(page->page.payload)));
		bool isStart = curLen == 0;
		bool isEnd   = curLen + neededLen >= len;

		// Search
		uint32_t nextAddr = 0;
		status = (std::make_unique<StorageSearchEmpty>(/*startSearchAddress=*/curAddr + Page::PAGE_SIZE))
			->searchPageAddress(prefix, id, &nextAddr);
		if (status != STORAGE_OK) {
			nextAddr = curAddr + Page::PAGE_SIZE;
		}
		if (!isEnd && status != STORAGE_OK) {
			break;
		}
		status = STORAGE_OK;

		// Set prev and next pages
		page->setPrevAddress(prevAddr);
		page->setNextAddress(nextAddr);
		if (isStart) {
			page->setPrevAddress(curAddr);
		}
		if (isEnd) {
			page->setNextAddress(curAddr);
		}


		// Check header (and save)
		uint32_t curMacroblockAddress = Header::getMacroblockStartAddress(curAddr);
		if (header && macroblockAddress != curMacroblockAddress) {
			status = header->save();
		}
		if (status == STORAGE_BUSY) {
			break;
		}
		if (macroblockAddress != curMacroblockAddress) {
			header = std::make_unique<Header>(curAddr);
			status = StorageMacroblock::loadHeader(header.get());
			macroblockAddress = curMacroblockAddress;
		}
		if (status == STORAGE_BUSY || status == STORAGE_OOM) {
			break;
		}

		// Save page
		memcpy(page->page.header.prefix, prefix, Page::PREFIX_SIZE);
		page->page.header.id = id;
		memcpy(page->page.payload, data + curLen, neededLen);
		status = page->save();
		if (status == STORAGE_BUSY) {
			break;
		}

		uint32_t pageIndex = StorageMacroblock::getPageIndexByAddress(curAddr);
		Header::MetaUnit* metaUnitPtr = &(header->data->metaUnits[pageIndex]);
		if (status != STORAGE_OK) {
			header->setAddressBlocked(curAddr);
			curAddr = nextAddr;
			continue;
		}


		// Registrate page in header
		memcpy((*metaUnitPtr).prefix, prefix, Page::PREFIX_SIZE);
		(*metaUnitPtr).id = id;
		header->setPageStatus(pageIndex, Header::PAGE_OK);


		// Update current values
		prevAddr = curAddr;
		curAddr = nextAddr;
		curLen += neededLen;
	} 

	if (status != STORAGE_OK) {
		return status;
	}

	// Save last header
	header->save();
	return STORAGE_OK;
}


StorageStatus StorageData::deleteData()
{
	uint32_t address = this->m_startAddress;

	if (StorageMacroblock::isMacroblockAddress(address)) {
		return STORAGE_ERROR;
	}

	StorageStatus status = StorageData::findStartAddress(&address);
	if (status == STORAGE_BUSY || status == STORAGE_OOM) {
		return status;
	}
	if (status == STORAGE_OK) {
		m_startAddress = address;
	}

	uint32_t curAddress = m_startAddress;
	uint32_t macroblockAddress = Page::PAGE_SIZE + 1;
	std::unique_ptr<Page> page = std::make_unique<Page>(curAddress);
	std::unique_ptr<Header> header;

	// Load first page
	status = page->load();
	if (status != STORAGE_OK) {
		return status;
	}

	do {
		// Check header (and save)
		uint32_t curMacroblockAddress = Header::getMacroblockStartAddress(curAddress);
		if (header && macroblockAddress != curMacroblockAddress) {
			status = header->save();
		}
		if (status != STORAGE_OK) {
			continue;
		}
		if (macroblockAddress != curMacroblockAddress) {
			header = std::make_unique<Header>(curMacroblockAddress);
			status = StorageMacroblock::loadHeader(header.get());
			macroblockAddress = curMacroblockAddress;
		}
		if (status == STORAGE_BUSY || status == STORAGE_OOM) {
			return status;
		}
		if (status != STORAGE_OK) {
			continue;
		}

		// Delete page from header
		uint32_t pageIndex = StorageMacroblock::getPageIndexByAddress(page->getAddress());
		Header::MetaUnit* metaUnitPtr = &(header->data->metaUnits[pageIndex]);
		memset((*metaUnitPtr).prefix, 0, Page::PREFIX_SIZE);
		(*metaUnitPtr).id = 0;
		header->setPageStatus(pageIndex, Header::PAGE_EMPTY);

		// Load next page
		status = page->loadNext();
		if (status != STORAGE_OK) {
			break;
		}
		curAddress = page->getAddress();
	} while (page->validateNextAddress());
	
	// Save last header
	return header->save();
}


StorageStatus StorageData::findStartAddress(uint32_t* address)
{
	std::unique_ptr<Page> page = std::make_unique<Page>(*address);
	StorageStatus status = page->load();
	if (status != STORAGE_OK) {
		return status;
	}

	while (!page->isStart()) {
		status = page->loadPrev();
		if (status != STORAGE_OK) {
			break;
		}
	}

	if (status != STORAGE_OK) {
		return status;
	}

	status = page->load(/*startPage=*/true);
	if (status != STORAGE_OK) {
		return status;
	}

	if (!page->isStart()) {
		return STORAGE_NOT_FOUND;
	}

	*address = page->getAddress();

	return STORAGE_OK;
}

StorageStatus StorageData::findEndAddress(uint32_t* address)
{
	std::unique_ptr<Page> page = std::make_unique<Page>(*address);
	StorageStatus status = page->load();
	if (status != STORAGE_OK) {
		return status;
	}

	while (!page->isEnd()) {
		status = page->loadNext();
		if (status != STORAGE_OK) {
			break;
		}
	}

	if (status != STORAGE_OK) {
		return status;
	}

	status = page->load(/*startPage=*/true);
	if (status != STORAGE_OK) {
		return status;
	}

	*address = page->getAddress();

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
