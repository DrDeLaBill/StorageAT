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
	
	if (StorageSector::isSectorAddress(m_startAddress)) {
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

	// TODO: if data has broken -> remove from headers

	return status;
}

StorageStatus StorageData::save(
	uint8_t  prefix[Page::PREFIX_SIZE],
	uint32_t id,
	uint8_t* data,
	uint32_t len
) {
	uint32_t pageAddress = m_startAddress;
	
	if (StorageSector::isSectorAddress(pageAddress)) {
		return STORAGE_ERROR;
	}

	uint32_t checkAddress = pageAddress;
	Header checkHeader(checkAddress);
	StorageStatus status = StorageSector::loadHeader(&checkHeader);
	if (status != STORAGE_OK) {
		return status;
	}
	if (!checkHeader.isAddressEmpty(checkAddress) && !checkHeader.isSameMeta(StorageSector::getPageIndexByAddress(checkAddress), prefix, id)) {
		return STORAGE_DATA_EXISTS;
	}
	if (checkHeader.isSameMeta(StorageSector::getPageIndexByAddress(checkAddress), prefix, id)) {
		status = StorageData::findStartAddress(&checkAddress);
	}
	if (status == STORAGE_OK) {
		pageAddress = checkAddress;
	}

	uint32_t curLen = 0;
	uint32_t curAddr = pageAddress;
	uint32_t prevAddr = pageAddress;
	uint32_t sectorAddress = Page::PAGE_SIZE + 1;
	std::unique_ptr<Header> header;
	std::unique_ptr<Page> page;
	while (curLen < len) {
		// Initialize page
		page = std::make_unique<Page>(curAddr);
		uint32_t neededLen = std::min(static_cast<uint32_t>(len - curLen), static_cast<uint32_t>(sizeof(page->page.payload)));
		bool isStart = curLen == 0;
		bool isEnd   = curLen + neededLen >= len;

		// Search
		uint32_t nextAddr = 0;
		status = (std::make_unique<StorageSearchEmpty>(/*startSearchAddress=*/curAddr + Page::PAGE_SIZE))
			->searchPageAddress(prefix, id, &nextAddr);
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
		uint32_t curSectorAddress = Header::getSectorStartAddress(curAddr);
		if (header && sectorAddress != curSectorAddress) {
			status = header->save();
		}
		if (status != STORAGE_OK) {
			break;
		}
		if (sectorAddress != curSectorAddress) {
			header = std::make_unique<Header>(curAddr);
			status = StorageSector::loadHeader(header.get());
			sectorAddress = curSectorAddress;
		}
		if (status == STORAGE_BUSY) {
			break;
		}
		if (status != STORAGE_OK) {
			curAddr = nextAddr;
			continue;
		}


		// Save page
		memcpy(page->page.header.prefix, prefix, Page::PREFIX_SIZE);
		page->page.header.id = id;
		memcpy(page->page.payload, data + curLen, neededLen);
		status = page->save();
		if (status == STORAGE_BUSY) {
			break;
		}
		if (status != STORAGE_OK) {
			header->setPageBlocked(StorageSector::getPageIndexByAddress(page->getAddress()));
			continue;
		}


		// Registrate page in header
		uint32_t pageIndex = StorageSector::getPageIndexByAddress(curAddr);
		header->data->pages[pageIndex].id     = id;
		header->data->pages[pageIndex].status = Header::PAGE_OK;
		memcpy(header->data->pages[pageIndex].prefix, prefix, Page::PREFIX_SIZE);


		// Update current values
		prevAddr = curAddr;
		curAddr = nextAddr;
		curLen += neededLen;
	} 

	if (status != STORAGE_OK) {
		// TODO: remove data
		this->deleteData();
		return status;
	}
	
	// Save last header
	return header->save();
}

StorageStatus StorageData::deleteData() // TODO: удалять записи из header
{
	uint32_t address = this->m_startAddress;

	if (StorageSector::isSectorAddress(address)) {
		return STORAGE_ERROR;
	}

	StorageStatus status = StorageData::findStartAddress(&address);
	if (status == STORAGE_BUSY) {
		return status;
	}
	if (status == STORAGE_OK) {
		m_startAddress = address;
	}

	uint32_t curAddress = m_startAddress;
	uint32_t sectorAddress = Page::PAGE_SIZE + 1;
	std::unique_ptr<Page> page = std::make_unique<Page>(curAddress);
	std::unique_ptr<Header> header;

	// Load first page
	status = page->load();
	if (status != STORAGE_OK) {
		return status;
	}

	do {
		// Check header (and save)
		uint32_t curSectorAddress = Header::getSectorStartAddress(curAddress);
		if (header && sectorAddress != curSectorAddress) {
			status = header->save();
		}
		if (status != STORAGE_OK) {
			continue;
		}
		if (sectorAddress != curSectorAddress) {
			header = std::make_unique<Header>(curSectorAddress);
			status = StorageSector::loadHeader(header.get());
			sectorAddress = curSectorAddress;
		}
		if (status != STORAGE_OK) {
			continue;
		}

		// Delete page from header
		uint32_t pageIndex = StorageSector::getPageIndexByAddress(page->getAddress());
		header->data->pages[pageIndex].id     = 0;
		header->data->pages[pageIndex].status = Header::PAGE_EMPTY;
		memset(header->data->pages[pageIndex].prefix, 0, Page::PREFIX_SIZE);

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
	StorageStatus status = StorageSector::loadHeader(&header);
	if (status != STORAGE_OK) {
		return status;
	}
	if (header.isAddressEmpty(address)) {
		return STORAGE_OK;
	}
	return STORAGE_ERROR;
}
