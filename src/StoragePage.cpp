/* Copyright © 2023 Georgy E. All rights reserved. */

#include "StoragePage.hpp"
#include "StoragePage.hpp"

#include <memory>
#include <string.h>
#include <stdint.h>

#include "StorageAT.hpp"
#include "StorageSector.hpp"


typedef StorageAT FS;

Page::Page(uint32_t address): address(address)
{
	memset((void*)&page, 0, sizeof(page));
	page.header.magic   = STORAGE_MAGIC;
	page.header.version = STORAGE_VERSION;
	page.header.status  = PAGE_STATUS_EMPTY;
}

void Page::setPageStatus(uint8_t status)
{
	page.header.status |= status;
}

bool Page::isSetPageStatus(uint8_t status)
{
	return (bool)(page.header.status & status);
}

StorageStatus Page::load(bool startPage)
{
	StorageStatus status = (FS::readCallback())(address, (uint8_t*)&page, sizeof(page));
	if (status != STORAGE_OK) {
		return status;
	}

	if (!this->validate()) {
		return STORAGE_ERROR;
	}

	if (startPage && !this->isSetPageStatus(PAGE_STATUS_START)) {
		return STORAGE_ERROR;
	}

	return STORAGE_OK;
}

StorageStatus Page::loadNext() {
	if (!this->validateNextAddress()) {
		return STORAGE_NOT_FOUND;
	}

	this->address = this->page.header.next_addr;

	return this->load();
}

StorageStatus Page::save()
{
	StorageStatus status = (FS::writeCallback())(address, (uint8_t*)&page, sizeof(page));
	if (status != STORAGE_OK) {
		return status;
	}

	status = this->load();
	if (status != STORAGE_OK) {
		return status;
	}

	return STORAGE_OK;
}

StorageStatus Page::deletePage()
{
	HeaderPage header(this->address);
	StorageStatus status = StorageSector::loadHeader(&header);
	if (status != STORAGE_OK) {
		return status;
	}

	HeaderPage::PageHeader* pageHeader = &(header.data->pages[StorageSector::getPageIndexByAddress(this->address)]);
	memset(pageHeader->prefix, 0, Page::STORAGE_PAGE_PREFIX_SIZE);
	pageHeader->id = 0;
	pageHeader->status = Page::PAGE_STATUS_EMPTY;

	status = header.save();
	if (status != STORAGE_OK) {
		return status;
	}

	return STORAGE_OK;
}

bool Page::isEmpty()
{
	return !this->validate();
}

bool Page::validate()
{
	if (page.header.magic != STORAGE_MAGIC) {
		return false;
	}

	if (page.header.version != STORAGE_VERSION) {
		return false;
	}

	uint16_t crc = this->getCRC16((uint8_t*)&page, sizeof(page) - sizeof(page.crc));
	if (crc != page.crc) {
		return false;
	}

	return true;
}

uint16_t Page::getCRC16(uint8_t* buf, uint16_t len) {
    uint16_t crc = 0;
    for (uint16_t i = 1; i < len; i++) {
        crc  = (uint8_t)(crc >> 8) | (crc << 8);
        crc ^= buf[i];
        crc ^= (uint8_t)(crc & 0xFF) >> 4;
        crc ^= (crc << 8) << 4;
        crc ^= ((crc & 0xff) << 4) << 1;
    }
    return crc;
}

bool Page::validateNextAddress()
{
	return !this->isSetPageStatus(PAGE_STATUS_END) && this->page.header.next_addr > 0;
}

HeaderPage::HeaderPage(uint32_t address): Page(address)
{
	this->address     = address - (address % StorageSector::SECTOR_PAGES_COUNT);
	this->data        = (HeaderPageStruct*)(uint8_t*)page.payload;
	this->sectorIndex = StorageSector::getSectorIndex(address);
}

void HeaderPage::setHeaderStatus(uint32_t pageIndex, uint8_t status)
{
	data->pages[pageIndex].status = status;
}

bool HeaderPage::isSetHeaderStatus(uint32_t pageIndex, uint8_t status)
{
	return (bool)(data->pages[pageIndex].status & status);
}

StorageStatus HeaderPage::createHeader()
{
	HeaderPage dumpHeader(this->address);
	for (uint16_t i = 0; i < PAGE_HEADERS_COUNT; i++) {
		Page page(StorageSector::getPageAddressByIndex(this->sectorIndex, i));

		StorageStatus status = page.load();
		if (status == STORAGE_BUSY) {
			return STORAGE_BUSY;
		}
		if (status == STORAGE_OK) {
			memcpy(dumpHeader.data->pages[i].prefix, page.page.header.prefix, sizeof(page.page.header.prefix));
			dumpHeader.data->pages[i].id     = page.page.header.id;
			dumpHeader.data->pages[i].status = HeaderPage::PAGE_OK;
		} else {
			dumpHeader.data->pages[i].status = HeaderPage::PAGE_EMPTY;
		}
	}

	return this->save();
}

StorageStatus HeaderPage::load()
{
	uint32_t startAddress = StorageSector::getSectorStartAdderss(this->sectorIndex);

	StorageStatus status = STORAGE_ERROR;
	for (uint8_t i = 0; i < StorageSector::SECTOR_RESERVED_PAGES_COUNT; i++) {
		this->address = startAddress + i * Page::STORAGE_PAGE_SIZE;

		status = Page::load();
		if (status == STORAGE_BUSY) {
			return STORAGE_BUSY;
		}
		if (status == STORAGE_OK) {
			return STORAGE_OK;
		}
	}
	return status;
}

StorageStatus HeaderPage::save()
{
	uint32_t startAddress = StorageSector::getSectorStartAdderss(this->sectorIndex);

	StorageStatus status = STORAGE_ERROR;
	for (uint8_t i = 0; i < StorageSector::SECTOR_RESERVED_PAGES_COUNT; i++) {
		this->address = startAddress + i * Page::STORAGE_PAGE_SIZE;

		status = Page::save();
		if (status == STORAGE_BUSY) {
			return STORAGE_BUSY;
		}
		if (status == STORAGE_OK) {
			return STORAGE_OK;
		}
	}

	return status;
}
