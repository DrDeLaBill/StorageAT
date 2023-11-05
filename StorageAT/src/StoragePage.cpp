/* Copyright © 2023 Georgy E. All rights reserved. */

#include "StoragePage.h"

#include <memory>
#include <string.h>
#include <stdint.h>

#include "StorageAT.h"
#include "StoragePage.h"
#include "StorageSector.h"


typedef StorageAT FS;


Page::Page(uint32_t address): address(address)
{
	memset(reinterpret_cast<void*>(&page), 0, sizeof(page));
	page.header.magic   = STORAGE_MAGIC;
	page.header.version = STORAGE_VERSION;
	page.header.status  = PAGE_STATUS_EMPTY;
}

Page& Page::operator=(Page* other)
{
	this->address = other->address;
	memcpy(reinterpret_cast<void*>(&this->page), reinterpret_cast<void*>(&other->page), sizeof(PageStruct));
	return *this;
}

void Page::setPageStatus(uint8_t status)
{
	page.header.status |= status;
}

bool Page::isSetPageStatus(uint8_t status)
{
	return static_cast<bool>(page.header.status & status);
}

StorageStatus Page::load(bool startPage)
{
	StorageStatus status = (FS::readCallback())(address, reinterpret_cast<uint8_t*>(&page), sizeof(page));
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
	page.crc = this->getCRC16(reinterpret_cast<uint8_t*>(&page), sizeof(page) - sizeof(page.crc));

	StorageStatus status = (FS::writeCallback())(address, reinterpret_cast<uint8_t*>(&page), sizeof(page));
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
	if (StorageSector::isSectorAddress(this->address)) {
		return STORAGE_ERROR;
	}

	Header header(this->address);
	StorageStatus status = StorageSector::loadHeader(&header);
	if (status != STORAGE_OK) {
		return status;
	}

	Header::PageHeader* pageHeader = &(header.data->pages[StorageSector::getPageIndexByAddress(this->address)]);
	memset(pageHeader->prefix, 0, Page::STORAGE_PAGE_PREFIX_SIZE);
	pageHeader->id = 0;
	pageHeader->status = Page::PAGE_STATUS_EMPTY;

	return header.save();
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

	uint16_t crc = this->getCRC16(reinterpret_cast<uint8_t*>(&page), sizeof(page) - sizeof(page.crc));
	if (crc != page.crc) {
		return false;
	}

	return true;
}

uint16_t Page::getCRC16(uint8_t* buf, uint16_t len) {
    uint16_t crc = 0;
    for (uint16_t i = 1; i < len; i++) {
        crc  = static_cast<uint16_t>((crc >> 8) | (crc << 8));
        crc ^= static_cast<uint16_t>(buf[i]);
        crc ^= static_cast<uint16_t>((crc & 0xFF) >> 4);
        crc ^= static_cast<uint16_t>((crc << 8) << 4);
        crc ^= static_cast<uint16_t>(((crc & 0xff) << 4) << 1);
    }
    return crc;
}

bool Page::validateNextAddress()
{
	return !this->isSetPageStatus(PAGE_STATUS_END) && this->page.header.next_addr > 0;
}

uint32_t Page::getAddress()
{
	return this->address;
}

Header::Header(uint32_t address): Page(address)
{
	this->address       = Header::getSectorStartAddress(address);
	this->data          = reinterpret_cast<HeaderPageStruct*>(page.payload);
	this->m_sectorIndex = StorageSector::getSectorIndex(address);
}

Header& Header::operator=(Header* other)
{
	Page::operator=(other);
	this->m_sectorIndex = other->m_sectorIndex;
 	return *this;
}

void Header::setHeaderStatus(uint32_t pageIndex, uint8_t status)
{
	data->pages[pageIndex].status = status;
}

bool Header::isSetHeaderStatus(uint32_t pageIndex, uint8_t status)
{
	return static_cast<bool>(data->pages[pageIndex].status & status);
}

void Header::setPageBlocked(uint32_t pageIndex)
{
	this->setHeaderStatus(pageIndex, Header::PAGE_BLOCKED);
}

uint32_t Header::getSectorIndex()
{
	return this->m_sectorIndex;
}

uint32_t Header::getSectorStartAddress(uint32_t address)
{
	return (address / (StorageSector::SECTOR_PAGES_COUNT * Page::STORAGE_PAGE_SIZE)) * Page::STORAGE_PAGE_SIZE;
}

StorageStatus Header::create()
{
	Header dumpHeader(this->address);
	for (uint16_t i = 0; i < PAGE_HEADERS_COUNT; i++) {
		Page page(StorageSector::getPageAddressByIndex(this->m_sectorIndex, i));

		StorageStatus status = page.load();
		if (status == STORAGE_BUSY) {
			return STORAGE_BUSY;
		}
		if (status == STORAGE_OK) {
			memcpy(dumpHeader.data->pages[i].prefix, page.page.header.prefix, sizeof(page.page.header.prefix));
			dumpHeader.data->pages[i].id     = page.page.header.id;
			dumpHeader.data->pages[i].status = Header::PAGE_OK;
		} else {
			dumpHeader.data->pages[i].status = Header::PAGE_EMPTY;
		}
	}

	StorageStatus status = dumpHeader.save();
	if (status != STORAGE_OK) {
		return status;
	}

	return this->load();
}

StorageStatus Header::load()
{
	uint32_t startAddress = StorageSector::getSectorAddress(this->m_sectorIndex);

	StorageStatus status = STORAGE_ERROR;
	for (uint8_t i = 0; i < StorageSector::SECTOR_RESERVED_PAGES_COUNT; i++) {
		this->address = startAddress + static_cast<uint32_t>(i * Page::STORAGE_PAGE_SIZE);

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

StorageStatus Header::save()
{
	uint32_t startAddress = StorageSector::getSectorAddress(this->m_sectorIndex);

	StorageStatus status = STORAGE_ERROR;
	for (uint8_t i = 0; i < StorageSector::SECTOR_RESERVED_PAGES_COUNT; i++) {
		this->address = startAddress + static_cast<uint32_t>(i * Page::STORAGE_PAGE_SIZE);

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
