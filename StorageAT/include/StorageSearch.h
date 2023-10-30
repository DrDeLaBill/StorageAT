/* Copyright © 2023 Georgy E. All rights reserved. */

#ifndef STORAGE_SEARCH_HPP
#define STORAGE_SEARCH_HPP


#include <stdint.h>
#include <stdbool.h>

#include "StorageAT.h"
#include "StoragePage.h"
#include "StorageType.h"


class StorageSearchBase
{
public:
	StorageSearchBase(uint32_t startSearchAddress): startSearchAddress(startSearchAddress) {}
	virtual ~StorageSearchBase() { foundOnce = false; }

	virtual StorageStatus searchPageAddress(
		const uint8_t  prefix[Page::STORAGE_PAGE_PREFIX_SIZE],
		const uint32_t id,
		uint32_t*      resAddress
	);

protected:
	uint32_t startSearchAddress;
	bool     foundOnce;

	virtual uint32_t getStartCmpId() { return 0; }
	virtual bool isNeededFirstResult() { return false; }

	virtual StorageStatus searchPageAddressInSector(
		HeaderPage*    header,
		const uint8_t  prefix[Page::STORAGE_PAGE_PREFIX_SIZE],
		const uint32_t id,
		uint32_t*      resAddress
	);

	virtual bool isIdFound(
		const uint32_t headerId,
		const uint32_t pageId,
		const uint32_t prevId
	) { return false; };
};


class StorageSearchEqual: public StorageSearchBase
{
public:
	StorageSearchEqual(uint32_t startSearchAddress): StorageSearchBase(startSearchAddress) {}

protected:
	bool isNeededFirstResult() override { return true; }

	bool isIdFound(
		const uint32_t headerId,
		const uint32_t pageId,
		const uint32_t prevId
	) override;
};

class StorageSearchNext: public StorageSearchBase
{
public:
	StorageSearchNext(uint32_t startSearchAddress): StorageSearchBase(startSearchAddress) {}

protected:
	virtual uint32_t getStartCmpId() { return StorageAT::STORAGE_MAX_ADDRESS; }

	bool isIdFound(
		const uint32_t headerId,
		const uint32_t pageId,
		const uint32_t prevId
	) override;
};

class StorageSearchMin: public StorageSearchBase
{
public:
	StorageSearchMin(uint32_t startSearchAddress): StorageSearchBase(startSearchAddress) {}

protected:
	virtual uint32_t getStartCmpId() { return StorageAT::STORAGE_MAX_ADDRESS; }

	bool isIdFound(
		const uint32_t headerId,
		const uint32_t pageId,
		const uint32_t prevId
	) override;
};

class StorageSearchMax: public StorageSearchBase
{
public:
	StorageSearchMax(uint32_t startSearchAddress): StorageSearchBase(startSearchAddress) {}

protected:
	bool isIdFound(
		const uint32_t headerId,
		const uint32_t pageId,
		const uint32_t prevId
	) override;
};

class StorageSearchEmpty: public StorageSearchBase
{
public:
	StorageSearchEmpty(uint32_t startSearchAddress): StorageSearchBase(startSearchAddress) {}

protected:
	bool isNeededFirstResult() override { return true; }
	
	StorageStatus searchPageAddressInSector(
		HeaderPage* header,
		const uint8_t prefix[Page::STORAGE_PAGE_PREFIX_SIZE],
		const uint32_t id,
		uint32_t* resAddress
	) override;
};


#endif
