#include "StorageSearch.h"

#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "StoragePage.h"
#include "StorageSector.h"


StorageStatus StorageSearchBase::searchPageAddress(
	const uint8_t  prefix[Page::STORAGE_PAGE_PREFIX_SIZE],
	const uint32_t id,
	uint32_t*      resAddress
) {
	uint32_t sectorIndex = StorageSector::getSectorIndex(this->m_startSearchAddress);
	for (; sectorIndex < StorageSector::getSectorsCount(); sectorIndex++) {
		HeaderPage header(StorageSector::getSectorStartAdderss(sectorIndex));

		StorageStatus status = StorageSector::loadHeader(&header);
		if (status == STORAGE_BUSY) {
			return STORAGE_BUSY;
		}
		if (status != STORAGE_OK) {
			continue;
		}

		status = this->searchPageAddressInBox(&header, prefix, id, resAddress);
		if (status == STORAGE_BUSY) {
			return STORAGE_BUSY;
		}
		if (status == STORAGE_OK) {
			continue;
		}
	}

	return STORAGE_NOT_FOUND;
}

StorageStatus StorageSearchBase::searchPageAddressInBox(
	HeaderPage*    header,
	const uint8_t  prefix[Page::STORAGE_PAGE_PREFIX_SIZE],
	const uint32_t id,
	uint32_t*      resAddress
) {
	bool foundOnce  = true;
	uint32_t prevId = getStartCmpId();

	uint32_t pageIndex = StorageSector::getPageIndexByAddress(this->m_startSearchAddress);
	for (; pageIndex < HeaderPage::PAGE_HEADERS_COUNT; pageIndex++) {
		if (!header->isSetHeaderStatus(pageIndex, HeaderPage::PAGE_OK)) {
			continue;
		}

		if (memcmp(header->data->pages[pageIndex].prefix, prefix, Page::STORAGE_PAGE_PREFIX_SIZE)) {
			continue;
		}

		if (!isIdFound(header->data->pages[pageIndex].id, id, prevId)) {
			continue;
		}

		Page page(StorageSector::getPageAddressByIndex(header->sectorIndex, pageIndex));
		if (!page.load(/*startPage=*/true)) {
			continue;
		}

		prevId      = header->data->pages[pageIndex].id;
		*resAddress = StorageSector::getPageAddressByIndex(header->sectorIndex, pageIndex);

		if (foundOnce && isNeededFirstResult()) {
			return STORAGE_OK;
		}
	}

	return foundOnce ? STORAGE_OK : STORAGE_NOT_FOUND;
}

bool StorageSearchEqual::isIdFound(
	const uint32_t headerId,
	const uint32_t pageId,
	const uint32_t prevId
) {
	if (pageId == headerId) {
		return true;
	}
	return false;
}

bool StorageSearchNext::isIdFound(
	const uint32_t headerId,
	const uint32_t pageId,
	const uint32_t prevId
) {
	if (pageId <= headerId) {
		return false;
	}
	if (pageId < prevId) {
		return true;
	}
	return false;
}

bool StorageSearchMin::isIdFound(
	const uint32_t headerId,
	const uint32_t pageId,
	const uint32_t prevId
) {
	return prevId > pageId;
}

bool StorageSearchMax::isIdFound(
	const uint32_t headerId,
	const uint32_t pageId,
	const uint32_t prevId
) {
	return prevId < pageId;
}

StorageStatus StorageSearchEmpty::searchPageAddressInBox(
	HeaderPage*    header,
	const uint8_t  prefix[Page::STORAGE_PAGE_PREFIX_SIZE],
	const uint32_t id,
	uint32_t*      resAddress
) {
	for (uint16_t i = 0; i < HeaderPage::PAGE_HEADERS_COUNT; i++) {
		if (header->isSetHeaderStatus(i, HeaderPage::PAGE_BLOCKED)) {
			continue;
		}

		Page page(StorageSector::getPageAddressByIndex(header->sectorIndex, i));

		StorageStatus status = page.load();
		if (status == STORAGE_BUSY) {
			return STORAGE_BUSY;
		}
		if (status != STORAGE_OK) {
			*resAddress = page.address;
			return STORAGE_OK;
		}
	}

	return STORAGE_ERROR;
}
