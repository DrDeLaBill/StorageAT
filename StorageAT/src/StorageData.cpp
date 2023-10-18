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

		memcpy(data, page.page.payload, neededLen);
		readLen += neededLen;

		status = page.loadNext();
		if (status != STORAGE_OK) {
			break;
		}
	} while (readLen < len);

	if (readLen == len) {
		return STORAGE_OK;
	}

	return status;
}

StorageStatus StorageData::save(
	uint8_t  prefix[Page::STORAGE_PAGE_PREFIX_SIZE],
	uint32_t id,
	uint8_t* data,
	uint32_t len
) {
	uint32_t pageAddress = m_startAddress;
	StorageStatus status = this->findStartAddress(&pageAddress);
	if (status == STORAGE_BUSY) {
		return STORAGE_BUSY;
	}
	if (status == STORAGE_OK) {
		return STORAGE_ERROR;
	}

	uint32_t curLen = 0;
	uint32_t curAddr = m_startAddress;
	std::unique_ptr<Page> page = std::make_unique<Page>(curAddr);
	while (curLen < len) {
		uint32_t neededLen = std::min(static_cast<uint32_t>(len - curLen), static_cast<uint32_t>(sizeof(page->page.payload)));

		bool isStart = curLen == 0;
		bool isEnd   = curLen + neededLen >= len;
		if (isStart) {
			page->setPageStatus(Page::PAGE_STATUS_START);
		}
		if (isEnd) {
			page->setPageStatus(Page::PAGE_STATUS_END);
		}
		if (!isStart && !isEnd) {
			page->setPageStatus(Page::PAGE_STATUS_MID);
		}

		uint32_t nextAddress = 0;
		if (!isEnd) {
			status = (std::make_unique<StorageSearchEmpty>(/*startSearchAddress=*/curAddr + Page::STORAGE_PAGE_SIZE))
				->searchPageAddress(
					page->page.header.prefix,
					page->page.header.id,
					&nextAddress
				);
		}
		if (status != STORAGE_OK) {
			return status;
		}
		page->page.header.next_addr = nextAddress;

		memcpy(page->page.header.prefix, prefix, Page::STORAGE_PAGE_PREFIX_SIZE);
		page->page.header.id = id;

		memcpy(page->page.payload, data + curLen, neededLen);

		status = page->save();
		if (status != STORAGE_OK) {
			this->deleteData();
			return status;
		}

		HeaderPage header(curAddr);
		status = header.load();
		if (status != STORAGE_OK) {
			this->deleteData();
			return status;
		}

		uint32_t pageIndex = StorageSector::getPageIndexByAddress(curAddr);
		header.data->pages[pageIndex].id = page->page.header.id;
		header.data->pages[pageIndex].status = page->page.header.status;
		memcpy(header.data->pages[pageIndex].prefix, page->page.header.prefix, Page::STORAGE_PAGE_PREFIX_SIZE);
		status = header.save();
		if (status != STORAGE_OK) {
			this->deleteData();
			return status;
		}

		curAddr = nextAddress;
		curLen += neededLen;
		page    = std::make_unique<Page>(curAddr);
	}

    return STORAGE_ERROR;
}

StorageStatus StorageData::deleteData()
{
	uint32_t address = 0;
	StorageStatus status = this->findStartAddress(&address);
	if (status == STORAGE_BUSY) {
		return STORAGE_BUSY;
	}
	if (status == STORAGE_OK) {
		m_startAddress = address;
	}

	uint32_t curAddress = m_startAddress;
	std::unique_ptr<Page> page;

	do {
		page = std::make_unique<Page>(curAddress);
		curAddress = page->page.header.next_addr;

		status = page->load();
		if (status != STORAGE_OK) {
			return status;
		}

		status = page->deletePage();
		if (status != STORAGE_OK) {
			return status;
		}
	} while (page->validateNextAddress());

	return STORAGE_OK;
}


StorageStatus StorageData::findStartAddress(uint32_t* address)
{
	std::unique_ptr<Page> page = std::make_unique<Page>(*address);
	StorageStatus status = page->load();
	if (status != STORAGE_OK) {
		return status;
	}

	uint32_t tmpAddress = 0;
	status = (std::make_unique<StorageSearchEqual>(/*startSearchAddress=*/0))->searchPageAddress(
		page->page.header.prefix,
		page->page.header.id,
		&tmpAddress
	);

	page = std::make_unique<Page>(tmpAddress);
	status = page->load(/*startPage=*/true);
	if (status != STORAGE_OK) {
		return status;
	}

	*address = tmpAddress;

	return STORAGE_OK;
}
