#include "StorageSearch.h"

#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "StoragePage.h"
#include "StorageSector.h"


StorageStatus StorageSearchBase::searchPageAddress(
	const uint8_t  prefix[Page::PREFIX_SIZE],
	const uint32_t id,
	uint32_t*      resAddress
) {
	uint32_t sectorIndex = StorageSector::getSectorIndex(this->startSearchAddress);
	this->prevId = getStartCmpId();
	this->foundOnce = false;

	for (; sectorIndex < StorageSector::getSectorsCount(); sectorIndex++) {
		Header header(StorageSector::getSectorAddress(sectorIndex));

		StorageStatus status = StorageSector::loadHeader(&header);
		if (status == STORAGE_BUSY) {
			return STORAGE_BUSY;
		}
		if (status != STORAGE_OK) {
			continue;
		}

		status = this->searchPageAddressInSector(&header, prefix, id);
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

StorageStatus StorageSearchBase::searchPageAddressInSector(
	Header*        header,
	const uint8_t  prefix[Page::PREFIX_SIZE],
	const uint32_t id
) {
	uint32_t pageIndex = StorageSector::getPageIndexByAddress(this->startSearchAddress);
	this->foundInSector = false;

	for (; pageIndex < Header::PAGES_COUNT; pageIndex++) {
		if (!header->isSetHeaderStatus(pageIndex, Header::PAGE_OK)) {
			continue;
		}

		if (header->isSetHeaderStatus(pageIndex, Header::PAGE_EMPTY)) {
			break;
		}

		if (memcmp(header->data->pages[pageIndex].prefix, prefix, Page::PREFIX_SIZE)) {
			continue;
		}

		if (!isIdFound(header->data->pages[pageIndex].id, id)) {
			continue;
		}

		Page page(StorageSector::getPageAddressByIndex(header->getSectorIndex(), pageIndex));
		StorageStatus status = page.load(/*startPage=*/true);
		if (status != STORAGE_OK) {
			continue;
		}

		this->foundOnce     = true;
		this->foundInSector = true;
		this->prevId        = header->data->pages[pageIndex].id;
		this->prevAddress   = StorageSector::getPageAddressByIndex(header->getSectorIndex(), pageIndex);

		if (isNeededFirstResult()) {
			break;
		}
	}

	return this->foundInSector ? STORAGE_OK : STORAGE_NOT_FOUND;
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

StorageStatus StorageSearchEmpty::searchPageAddressInSector(
	Header*        header,
	const uint8_t  prefix[Page::PREFIX_SIZE],
	const uint32_t id
) {
	uint32_t pageIndex = StorageSector::getPageIndexByAddress(this->startSearchAddress);
	this->foundInSector = false;
	for (; pageIndex < Header::PAGES_COUNT; pageIndex++) {
		if (header->isSetHeaderStatus(pageIndex, Header::PAGE_BLOCKED)) {
			continue;
		}

		uint32_t address = StorageSector::getPageAddressByIndex(header->getSectorIndex(), pageIndex);

		if (header->isSetHeaderStatus(pageIndex, Header::PAGE_EMPTY)) {
			this->foundOnce     = true;
			this->foundInSector = true;
			this->prevAddress   = address;
			break;
		}

		Page page(address);

		StorageStatus status = page.load();
		if (status == STORAGE_BUSY) {
			return STORAGE_BUSY;
		}
		if (status != STORAGE_OK) {
			this->foundOnce     = true;
			this->foundInSector = true;
			this->prevAddress   = page.getAddress();
			return STORAGE_OK;
		}
	}

	return this->foundInSector ? STORAGE_OK : STORAGE_NOT_FOUND;
}
