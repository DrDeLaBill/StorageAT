/* Copyright © 2025 Georgy E. All rights reserved. */

#include "StoragePage.h"

#include <string.h>
#include <stdint.h>

#include "StorageAT.h"
#include "StorageData.h"
#include "StoragePage.h"
#include "StorageMacroblock.h"


typedef StorageAT AT;


bool storage_at_data_success(StorageStatus status)
{
    return status == STORAGE_OK || status == STORAGE_HEADER_ERROR;
}


Page::Page(uint32_t address): address(address)
{
    memset(reinterpret_cast<void*>(&page), 0, sizeof(page));
    page.header.magic     = STORAGE_MAGIC;
    page.header.version   = STORAGE_VERSION;
    page.header.prev_addr = this->address;
    page.header.next_addr = this->address;
}

Page::Page(const Page& other)
{
    this->address = other.address;

    memcpy(
        reinterpret_cast<void*>(&this->page),
        reinterpret_cast<void*>(const_cast<PageStruct*>(&(other.page))),
        sizeof(PageStruct)
    );
}

Page& Page::operator=(const Page& other)
{
    this->address = other.address;

    memcpy(
        reinterpret_cast<void*>(&this->page),
        reinterpret_cast<void*>(const_cast<PageStruct*>(&(other.page))),
        sizeof(PageStruct)
    );

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
    if (status == STORAGE_BUSY) {
    	return status;
    }
    if (status != STORAGE_OK) {
        return STORAGE_NOT_FOUND;
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
        tmpPage.repair();
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

void Page::prepareSave()
{
    page.header.magic = STORAGE_MAGIC;
    page.header.version = STORAGE_VERSION;
    page.crc = this->getCRC16(reinterpret_cast<uint8_t*>(&page), sizeof(page) - sizeof(page.crc));
}

StorageStatus Page::save()
{
    if (this->address + sizeof(page) > StorageAT::getStorageSize()) {
        return STORAGE_OOM;
    }

    this->prepareSave();

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

bool Page::empty()
{
    for (unsigned i = 0; i < STORAGE_PAGE_SIZE; i++) {
        if (((uint8_t*)&page)[i] != 0xFF) {
            return false;
        }
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

void Page::repair()
{
    if (page.header.magic != STORAGE_MAGIC) {
        return;
    }

    uint16_t crc = this->getCRC16(reinterpret_cast<uint8_t*>(&page), sizeof(page) - sizeof(page.crc));
    if (crc != page.crc) {
        return;
    }

    if (page.header.version == STORAGE_VERSION_V5) {
    	page.header.version = STORAGE_VERSION_V6;
        page.crc = this->getCRC16(reinterpret_cast<uint8_t*>(&page), sizeof(page) - sizeof(page.crc));
        AT::driverCallback()->write(address, reinterpret_cast<uint8_t*>(&page), sizeof(page));
        AT::driverCallback()->read(address, reinterpret_cast<uint8_t*>(&page), sizeof(page));
    }
}

Header::Header(uint32_t address): Page(address)
{
    this->header            = (HeaderStruct*)&page;
    this->address           = Header::getMacroblockStartAddress(address);
    this->data              = (HeaderMeta*)header->payload;
    this->m_macroblockIndex = StorageMacroblock::getMacroblockIndex(address);
}

Header::Header(const Header& other): Page(other)
{
    this->header            = (HeaderStruct*)&page;
    this->address           = other.address;
    this->m_macroblockIndex = other.m_macroblockIndex;

    memcpy(header->payload, other.header->payload, STORAGE_HEADER_PAYLOAD_SIZE);
    this->data = reinterpret_cast<HeaderMeta*>(header->payload);
}

Header::~Header()
{
    Page::~Page();
}

Header& Header::operator=(const Header& other)
{
    Page::operator =(other);

    this->m_macroblockIndex = other.m_macroblockIndex;

    this->header = (HeaderStruct*)&page;
    memcpy(header->payload, other.header->payload, STORAGE_HEADER_PAYLOAD_SIZE);
    this->data = (HeaderMeta*)header->payload;

    return *this;
}

void Header::setAddressBlocked(uint32_t targetAddress)
{
    uint32_t pageIndex = StorageMacroblock::getPageIndexByAddress(targetAddress);
    memcpy(this->data->metaUnits[pageIndex].prefix, BLOCK_PREFIX, sizeof(BLOCK_PREFIX));
}

bool Header::isAddressBlocked(uint32_t targetAddress)
{
    uint32_t pageIndex = StorageMacroblock::getPageIndexByAddress(targetAddress);
    return !memcmp(this->data->metaUnits[pageIndex].prefix, BLOCK_PREFIX, sizeof(BLOCK_PREFIX));
}

void Header::setAddressEmpty(uint32_t targetAddress)
{
    uint32_t pageIndex = StorageMacroblock::getPageIndexByAddress(targetAddress);
    memcpy(this->data->metaUnits[pageIndex].prefix, EMPTY_PREFIX, sizeof(EMPTY_PREFIX));
}

bool Header::isAddressEmpty(uint32_t targetAddress)
{
    uint32_t pageIndex = StorageMacroblock::getPageIndexByAddress(targetAddress);
    return !memcmp(this->data->metaUnits[pageIndex].prefix, EMPTY_PREFIX, sizeof(EMPTY_PREFIX));
}

bool Header::isSameMeta(uint32_t pageIndex, const uint8_t* prefix, uint32_t id)
{
    MetaUnit* metaUnitPtr = &(data->metaUnits[pageIndex]);
    return !memcmp(this->data->metaUnits[pageIndex].prefix, prefix, STORAGE_PAGE_PREFIX_SIZE) && (*metaUnitPtr).id == id;
}

uint32_t Header::getMacroblockIndex()
{
    return this->m_macroblockIndex;
}

uint32_t Header::getMacroblockStartAddress(uint32_t address)
{
    return (address / (StorageMacroblock::PAGES_COUNT * STORAGE_PAGE_SIZE)) * (StorageMacroblock::PAGES_COUNT * STORAGE_PAGE_SIZE);
}

StorageStatus Header::create()
{
    StorageStatus status = STORAGE_OK;
    MetaUnit* metaUnitPtr = this->data->metaUnits;
    for (unsigned  i = 0; i < Header::PAGES_COUNT; i++, metaUnitPtr++) {
        Page tmpPage(StorageMacroblock::getPageAddressByIndex(this->m_macroblockIndex, i));

        status = tmpPage.load();
        if (status == STORAGE_BUSY) {
            return STORAGE_BUSY;
        }

        this->setAddressEmpty(tmpPage.getAddress());
        if (status == STORAGE_OOM) {
            this->setAddressBlocked(tmpPage.getAddress());
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
            HeaderStruct* tmp = (HeaderStruct*)&tmpPage.page;
            memcpy((*metaUnitPtr).prefix, tmpPage.page.header.prefix, sizeof(tmpPage.page.header.prefix));
            (*metaUnitPtr).id = tmpPage.page.header.id;
        }
    }

    status = this->save();
    if (storage_at_data_success(status)) {
        return STORAGE_OK;
    }

    return status;
}

StorageStatus Header::load()
{
    uint32_t startAddress = StorageMacroblock::getMacroblockAddress(this->m_macroblockIndex);

    StorageStatus status = STORAGE_ERROR;
    for (uint8_t i = 0; i < StorageMacroblock::RESERVED_PAGES_COUNT; i++) {
        this->address = startAddress + static_cast<uint32_t>(i * STORAGE_PAGE_SIZE);

        status = Page::load();
        if (status == STORAGE_BUSY) {
            return STORAGE_BUSY;
        }
        if (status == STORAGE_OK) {
            return STORAGE_OK;
        }
    }

    if (!this->validate()) {
        return STORAGE_HEADER_ERROR;
    }

    if (status == STORAGE_ERROR) {
        return STORAGE_HEADER_ERROR;
    }
    
    return status;
}

StorageStatus Header::save()
{
    uint32_t startAddress = StorageMacroblock::getMacroblockAddress(this->m_macroblockIndex);

    StorageStatus status = STORAGE_ERROR;
    for (uint8_t i = 0; i < StorageMacroblock::RESERVED_PAGES_COUNT; i++) {
        this->address = startAddress + static_cast<uint32_t>(i * STORAGE_PAGE_SIZE);

        status = Page::save();
        if (status == STORAGE_BUSY) {
            return STORAGE_BUSY;
        }
        if (status == STORAGE_OK) {
            return STORAGE_OK;
        }
    }

    if (!this->validate()) {
        return STORAGE_HEADER_ERROR;
    }

    if (status == STORAGE_ERROR) {
        return STORAGE_HEADER_ERROR;
    }
    
    return status;
}

bool Header::validate()
{
    return Page::validate();
}

uint32_t Header::getAddress()
{
    return Page::getAddress();
}

void Header::prepareSave()
{
    Page::prepareSave();
}