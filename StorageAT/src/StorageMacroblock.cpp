/* Copyright © 2023 Georgy E. All rights reserved. */

#include "StorageMacroblock.h"

#include <string.h>
#include <stdint.h>

#include "StorageAT.h"
#include "StoragePage.h"
#include "StorageType.h"
#include "StorageSearch.h"


typedef StorageAT AT;


uint32_t StorageMacroblock::getMacroblockAddress(uint32_t macroblockIndex)
{
	return macroblockIndex * Page::PAGE_SIZE * StorageMacroblock::PAGES_COUNT;
}

uint32_t StorageMacroblock::getMacroblockIndex(uint32_t macroblockAddress)
{
	return macroblockAddress / Page::PAGE_SIZE / StorageMacroblock::PAGES_COUNT;
}

uint32_t StorageMacroblock::getMacroblocksCount()
{
	return AT::getStoragePagesCount() / PAGES_COUNT;
}

uint32_t StorageMacroblock::getPageAddressByIndex(uint32_t macroblockIndex, uint32_t pageIndex)
{
	return getMacroblockAddress(macroblockIndex) + (RESERVED_PAGES_COUNT + pageIndex) * Page::PAGE_SIZE;
}

uint32_t StorageMacroblock::getPageIndexByAddress(uint32_t address)
{
	if (StorageMacroblock::isMacroblockAddress(address)) {
		return 0;
	}
    return ((address / Page::PAGE_SIZE) % (PAGES_COUNT)) - RESERVED_PAGES_COUNT;
}

bool StorageMacroblock::isMacroblockAddress(uint32_t address)
{
	return ((address / Page::PAGE_SIZE) % (PAGES_COUNT)) < RESERVED_PAGES_COUNT;
}

StorageStatus StorageMacroblock::formatMacroblock(uint32_t macroblockIndex)
{
	Header header(StorageMacroblock::getMacroblockAddress(macroblockIndex));
	StorageStatus status = StorageMacroblock::loadHeader(&header);
	if (status != STORAGE_OK) {
		return status;
	}

	Header::PageHeader* headerPtr = header.data->pages;
	Header::PageHeader* headerEndPtr = &(header.data->pages[Header::PAGES_COUNT-1]);
	for (; headerPtr < headerEndPtr; headerPtr++) {
		memset((*headerPtr).prefix, 0, Page::PREFIX_SIZE);
		(*headerPtr).id = 0;
		if (!header.isSetHeaderStatus(headerPtr, Header::PAGE_BLOCKED)) {
			header.setHeaderStatus(headerPtr, Header::PAGE_EMPTY);
		}
	}
	return header.save();
}

StorageStatus StorageMacroblock::loadHeader(Header *header)
{
	if (header->getAddress() > AT::getStorageSize()) {
		return STORAGE_OOM;
	}

	StorageStatus status = header->load();
	if (status == STORAGE_BUSY || status == STORAGE_OOM) {
		return status;
	}
	if (status == STORAGE_OK) {
		return STORAGE_OK;
	}

	status = header->create();
	if (status == STORAGE_BUSY || status == STORAGE_OOM) {
		return status;
	}
	if (status == STORAGE_OK) {
		return STORAGE_OK;
	}

	return status;
}
