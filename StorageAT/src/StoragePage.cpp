/* Copyright © 2023 Georgy E. All rights reserved. */

#include "StoragePage.h"

#include <memory>
#include <string.h>
#include <stdint.h>

#include "StorageAT.h"
#include "StorageData.h"
#include "StoragePage.h"
#include "StorageMacroblock.h"


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

StorageStatus Page::loadPrev()
{
	if (!this->validatePrevAddress()) {
		return STORAGE_NOT_FOUND;
	}

	Page tmpPage(this->page.header.prev_addr);
	StorageStatus status = tmpPage.load();
	if (status != STORAGE_OK) {
		return status;
	}

	if (memcmp(tmpPage.page.header.prefix, this->page.header.prefix, sizeof(tmpPage.page.header.prefix))) {
		return STORAGE_NOT_FOUND;
	}
	if (tmpPage.page.header.id != this->page.header.id) {
		return STORAGE_NOT_FOUND;
	}

	this->address = this->page.header.prev_addr;
	memcpy(reinterpret_cast<void*>(&this->page), reinterpret_cast<void*>(&tmpPage.page), sizeof(this->page));

	return STORAGE_OK;
}

StorageStatus Page::loadNext() {
	if (!this->validateNextAddress()) {
		return STORAGE_NOT_FOUND;
	}

	Page tmpPage(this->page.header.next_addr);
	StorageStatus status = tmpPage.load();
	if (status != STORAGE_OK) {
		return status;
	}

	if (memcmp(tmpPage.page.header.prefix, this->page.header.prefix, sizeof(tmpPage.page.header.prefix))) {
		return STORAGE_NOT_FOUND;
	}
	if (tmpPage.page.header.id != this->page.header.id) {
		return STORAGE_NOT_FOUND;
	}

	this->address = this->page.header.next_addr;
	memcpy(reinterpret_cast<void*>(&this->page), reinterpret_cast<void*>(&tmpPage.page), sizeof(this->page));

	return STORAGE_OK;
}

StorageStatus Page::load(bool startPage)
{
	if (this->address + sizeof(page) > StorageAT::getStorageSize()) {
		return STORAGE_OOM;
	}

	Page tmpPage(this->address);
	StorageStatus status = AT::driverCallback()->read(address, reinterpret_cast<uint8_t*>(&tmpPage.page), sizeof(tmpPage.page));
	if (status != STORAGE_OK) {
		return status;
	}

	if (!tmpPage.validate()) {
		return STORAGE_ERROR;
	}

	if (startPage && !tmpPage.isStart()) {
		return STORAGE_ERROR;
	}

	memcpy(reinterpret_cast<void*>(&this->page), reinterpret_cast<void*>(&tmpPage.page), sizeof(this->page));

	return STORAGE_OK;
}

StorageStatus Page::save()
{
	if (this->address + sizeof(page) > StorageAT::getStorageSize()) {
		return STORAGE_OOM;
	}
	page.header.magic = STORAGE_MAGIC;
	page.header.version = STORAGE_VERSION;
	page.crc = this->getCRC16(reinterpret_cast<uint8_t*>(&page), sizeof(page) - sizeof(page.crc));

	Page checkPage(this->address);
	StorageStatus status = checkPage.load();
	bool sameDataExists = false;
	if (status == STORAGE_OK && !memcmp(reinterpret_cast<void*>(&(this->page)), reinterpret_cast<void*>(&(checkPage.page)), sizeof(this->page))) {
		sameDataExists = true;
	}

	if (!sameDataExists) {
		status = AT::driverCallback()->write(address, reinterpret_cast<uint8_t*>(&page), sizeof(page));
	}
	if (!sameDataExists && status != STORAGE_OK) {
		return status;
	}

	status = checkPage.load();
	if (status == STORAGE_OK) {
		memcpy(reinterpret_cast<void*>(&(this->page)), reinterpret_cast<void*>(&(checkPage.page)), sizeof(this->page));
	}

	return status;
}

StorageStatus Header::deletePage(uint32_t targetAddress)
{
	if (StorageMacroblock::isMacroblockAddress(targetAddress)) {
		return STORAGE_ERROR;
	}

	Header header(targetAddress);
	StorageStatus status = StorageMacroblock::loadHeader(&header);
	if (status == STORAGE_BUSY || status == STORAGE_OOM) {
		return status;
	}
	if (status == STORAGE_OK) {
		Header::PageHeader* pageHeader = &(header.data->pages[StorageMacroblock::getPageIndexByAddress(this->address)]);
		memset(pageHeader->prefix, 0, Page::PREFIX_SIZE);
		pageHeader->status = Header::PAGE_EMPTY;
		pageHeader->id = 0;

		return header.save();
	}

	Page page(targetAddress);
	status = page.load();
	if (status == STORAGE_BUSY) {
		return status;
	}
	if (status != STORAGE_OK) {
		return STORAGE_OK;
	}

	memset(reinterpret_cast<void*>(&page.page), 0xFF, sizeof(page.page));

	return page.save();
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
	return !this->isStart(); // && (this->isMiddle() || this->isEnd());
}

bool Page::validateNextAddress()
{
	return !this->isEnd();// && (this->isStart() || this->isMiddle());
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
	this->address           = Header::getMacroblockStartAddress(address);
	this->data              = reinterpret_cast<HeaderPageStruct*>(page.payload);
	this->m_macroblockIndex = StorageMacroblock::getMacroblockIndex(address);

	this->page.header.prev_addr = 0;
	this->page.header.next_addr = 0;
}

Header& Header::operator=(Header* other)
{
	Page::operator=(other);
	this->m_macroblockIndex = other->m_macroblockIndex;
 	return *this;
}

void Header::setHeaderStatus(PageHeader* pageHeader, uint8_t status)
{
	(*pageHeader).status = status;
}

bool Header::isSetHeaderStatus(PageHeader* pageHeader, uint8_t status)
{
	return static_cast<bool>((*pageHeader).status & status);
}

void Header::setPageBlocked(PageHeader* pageHeader)
{
	this->setHeaderStatus(pageHeader, Header::PAGE_BLOCKED);
}

uint32_t Header::getMacroblockIndex()
{
	return this->m_macroblockIndex;
}

bool Header::isAddressEmpty(uint32_t targetAddress)
{
	return this->isSetHeaderStatus(&this->data->pages[StorageMacroblock::getPageIndexByAddress(targetAddress)], Header::PAGE_EMPTY);
}

bool Header::isSameMeta(uint32_t pageIndex, const uint8_t* prefix, uint32_t id)
{
	PageHeader* headerPtr = &(data->pages[pageIndex]);
	return !memcmp((*headerPtr).prefix, prefix, Page::PREFIX_SIZE) && (*headerPtr).id == id;
}

uint32_t Header::getMacroblockStartAddress(uint32_t address)
{
	return (address / (StorageMacroblock::PAGES_COUNT * Page::PAGE_SIZE)) * (StorageMacroblock::PAGES_COUNT * Page::PAGE_SIZE);
}

StorageStatus Header::create()
{
	PageHeader* headerPtr = this->data->pages;
	for (unsigned  i = 0; i < Header::PAGES_COUNT; i++, headerPtr++) {
		Page tmpPage(StorageMacroblock::getPageAddressByIndex(this->m_macroblockIndex, i));

		StorageStatus status = tmpPage.load();
		if (status == STORAGE_BUSY) {
			return STORAGE_BUSY;
		}

		memset((*headerPtr).prefix, 0, sizeof(tmpPage.page.header.prefix));
		(*headerPtr).id     = 0;
		(*headerPtr).status = Header::PAGE_EMPTY;
		
		if (status == STORAGE_OOM) {
			(*headerPtr).status = Header::PAGE_BLOCKED;
		}
		if (status != STORAGE_OK) {
			continue;
		}

		uint32_t tmpAddress = tmpPage.getAddress();
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
			memcpy((*headerPtr).prefix, tmpPage.page.header.prefix, sizeof(tmpPage.page.header.prefix));
			(*headerPtr).id     = tmpPage.page.header.id;
			(*headerPtr).status = Header::PAGE_OK;
		}
	}

	return this->save();
}

StorageStatus Header::load()
{
	uint32_t startAddress = StorageMacroblock::getMacroblockAddress(this->m_macroblockIndex);

	StorageStatus status = STORAGE_ERROR;
	for (uint8_t i = 0; i < StorageMacroblock::RESERVED_PAGES_COUNT; i++) {
		this->address = startAddress + static_cast<uint32_t>(i * Page::PAGE_SIZE);

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
	uint32_t startAddress = StorageMacroblock::getMacroblockAddress(this->m_macroblockIndex);

	StorageStatus status = STORAGE_ERROR;
	for (uint8_t i = 0; i < StorageMacroblock::RESERVED_PAGES_COUNT; i++) {
		this->address = startAddress + static_cast<uint32_t>(i * Page::PAGE_SIZE);

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

	PageHeader* headerPtr = data->pages;
	PageHeader* headerEndPtr = &(this->data->pages[Header::PAGES_COUNT-1]);
	for (; headerPtr < headerEndPtr; headerPtr++) {
		if (!(*headerPtr).status) {
			return false;
		}
	}

	return true;
}
