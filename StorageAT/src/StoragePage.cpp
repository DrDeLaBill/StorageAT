/* Copyright © 2023 Georgy E. All rights reserved. */

#include "StoragePage.h"

#include <memory>
#include <string.h>
#include <stdint.h>

#include "StorageAT.h"
#include "StorageData.h"
#include "StoragePage.h"
#include "StorageSector.h"


typedef StorageAT AT;


Page::Page(uint32_t address): address(address)
{
	memset(reinterpret_cast<void*>(&page), 0, sizeof(page));
	page.header.magic     = STORAGE_MAGIC;
	page.header.version   = STORAGE_VERSION;
	page.header.prev_addr = this->address;
	page.header.next_addr = this->address;
}

Page& Page::operator=(Page* other)
{
	this->address = other->address;
	memcpy(reinterpret_cast<void*>(&this->page), reinterpret_cast<void*>(&other->page), sizeof(PageStruct));
	return *this;
}

StorageStatus Page::load(bool startPage)
{
	StorageStatus status = AT::driverCallback()->read(address, reinterpret_cast<uint8_t*>(&page), sizeof(page));
	if (status != STORAGE_OK) {
		return status;
	}

	if (!this->validate()) {
		return STORAGE_ERROR;
	}

	if (startPage && !this->isStart()) {
		return STORAGE_ERROR;
	}

	return STORAGE_OK;
}

StorageStatus Page::loadPrev()
{
	if (!this->validatePrevAddress()) {
		return STORAGE_NOT_FOUND;
	}

	this->address = this->page.header.prev_addr;

	uint8_t prefix[STORAGE_PAGE_PREFIX_SIZE] = {};
	memcpy(prefix, this->page.header.prefix, sizeof(prefix));
	uint32_t id = this->page.header.id;

	StorageStatus status = this->load();
	if (status != STORAGE_OK) {
		return status;
	}

	if (memcmp(prefix, this->page.header.prefix, sizeof(prefix))) {
		return STORAGE_NOT_FOUND;
	}
	if (id != this->page.header.id) {
		return STORAGE_NOT_FOUND;
	}

	return STORAGE_OK;
}

StorageStatus Page::loadNext() {
	if (!this->validateNextAddress()) {
		return STORAGE_NOT_FOUND;
	}

	this->address = this->page.header.next_addr;

	uint8_t prefix[STORAGE_PAGE_PREFIX_SIZE] = {};
	memcpy(prefix, this->page.header.prefix, sizeof(prefix));
	uint32_t id = this->page.header.id;

	StorageStatus status = this->load();
	if (status != STORAGE_OK) {
		return status;
	}

	if (memcmp(prefix, this->page.header.prefix, sizeof(prefix))) {
		return STORAGE_NOT_FOUND;
	}
	if (id != this->page.header.id) {
		return STORAGE_NOT_FOUND;
	}

	return STORAGE_OK;
}

StorageStatus Page::save()
{
	page.crc = this->getCRC16(reinterpret_cast<uint8_t*>(&page), sizeof(page) - sizeof(page.crc));

	StorageStatus status = AT::driverCallback()->write(address, reinterpret_cast<uint8_t*>(&page), sizeof(page));
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
	pageHeader->status = Header::PAGE_EMPTY;

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

bool Page::isStart()
{
	return this->address == this->page.header.prev_addr;
}

bool Page::isMiddle()
{
	return !this->isStart() && !this->isEnd();
}

bool Page::isEnd()
{
	return this->address == this->page.header.next_addr;
}

bool Page::validatePrevAddress()
{
	return !this->isMiddle() && this->page.header.prev_addr > 0;
}

bool Page::validateNextAddress()
{
	return !this->isEnd() && this->page.header.next_addr > 0;
}

uint32_t Page::getAddress()
{
	return this->address;
}

void Page::setPrevAddress(uint32_t prevAddress)
{
	this->page.header.prev_addr = prevAddress;
}

void Page::setNextAddress(uint32_t nextAddress)
{
	this->page.header.next_addr = nextAddress;
}

Header::Header(uint32_t address): Page(address)
{
	this->address       = Header::getSectorStartAddress(address);
	this->data          = reinterpret_cast<HeaderPageStruct*>(page.payload);
	this->m_sectorIndex = StorageSector::getSectorIndex(address);

	this->page.header.prev_addr = 0;
	this->page.header.next_addr = 0;
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

bool Header::isAddressEmpty(uint32_t targetAddress)
{
	return this->isSetHeaderStatus(StorageSector::getPageIndexByAddress(targetAddress), Header::PAGE_EMPTY);
}

uint32_t Header::getSectorStartAddress(uint32_t address)
{
	return (address / (StorageSector::SECTOR_PAGES_COUNT * Page::STORAGE_PAGE_SIZE)) * (StorageSector::SECTOR_PAGES_COUNT * Page::STORAGE_PAGE_SIZE);
}

StorageStatus Header::create()
{
	Header dumpHeader(this->address);
	for (uint16_t i = 0; i < PAGE_HEADERS_COUNT; i++) {
		Page tmpPage(StorageSector::getPageAddressByIndex(this->m_sectorIndex, i));

		StorageStatus status = tmpPage.load();
		if (status == STORAGE_BUSY) {
			return STORAGE_BUSY;
		}

		dumpHeader.data->pages[i].status = Header::PAGE_EMPTY;

		uint32_t tmpAddress = 0;
		status = StorageData::findStartAddress(&tmpAddress);
		if (status == STORAGE_BUSY) {
			return STORAGE_BUSY;
		}
		if (status != STORAGE_OK) {
			continue;
		}
		status = StorageData::findEndAddress(&tmpAddress);
		if (status == STORAGE_BUSY) {
			return STORAGE_BUSY;
		}
		if (status != STORAGE_OK) {
			continue;
		}

		if (status == STORAGE_OK) {
			memcpy(dumpHeader.data->pages[i].prefix, tmpPage.page.header.prefix, sizeof(tmpPage.page.header.prefix));
			dumpHeader.data->pages[i].id     = tmpPage.page.header.id;
			dumpHeader.data->pages[i].status = Header::PAGE_OK;
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

bool Header::validate()
{
	if (!Page::validate()) {
		return false;
	}

	for (unsigned i = 0; i < PAGE_HEADERS_COUNT; i++) {
		if (!data->pages[i].status) {
			return false;
		}
	}

	return true;
}
