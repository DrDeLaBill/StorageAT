/* Copyright © 2023 Georgy E. All rights reserved. */

#ifndef STORAGE_SEARCH_HPP
#define STORAGE_SEARCH_HPP


#include <stdint.h>
#include <stdbool.h>

#include "StorageAT.hpp"
#include "StoragePage.hpp"
#include "StorageType.hpp"


class StorageSearchBase
{
public:
	StorageSearchBase(uint32_t startSearchAddress): m_startSearchAddress(startSearchAddress) {}
	virtual ~StorageSearchBase() {}

	virtual StorageStatus searchPageAddress(
		const uint8_t  prefix[Page::STORAGE_PAGE_PREFIX_SIZE],
		const uint32_t id,
		uint32_t*      resAddress
	);

protected:
	uint32_t m_startSearchAddress;

	virtual uint32_t getStartCmpId() { return 0; }
	virtual bool isNeededFirstResult() { return false; }

	virtual StorageStatus searchPageAddressInBox(
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
	StorageStatus searchPageAddressInBox(
		HeaderPage* header,
		const uint8_t prefix[Page::STORAGE_PAGE_PREFIX_SIZE],
		const uint32_t id,
		uint32_t* resAddress
	) override;
};


#endif
