#include "StorageEmulator.h"

#include <memory>
#include <iostream>
#include <string.h>

#include "StorageAT.h"


StorageEmulator::StorageEmulator(uint32_t pagesCount): pagesCount(pagesCount) 
{
    this->memory = std::make_unique<uint8_t[]>(this->getSize());
    this->requestsCount = std::make_unique<RequestsCount[]>(this->pagesCount);
    this->clear();
    this->isBusy = false;
}

uint32_t StorageEmulator::getSize()
{
    return this->getPagesCount() * Page::PAGE_SIZE;
}

uint32_t StorageEmulator::getPagesCount()
{
    return  this->pagesCount;
}

void StorageEmulator::setBusy(bool busy)
{
    this->isBusy = busy;
}

StorageEmulatorStatus StorageEmulator::readPage(uint32_t address, uint8_t* data, uint32_t len)
{
    requestsCount[address / Page::PAGE_SIZE].read++;
    if (this->isBusy) {
        return EMULATOR_BUSY;
    }

    if (len > 256) {
        return EMULATOR_ERROR;
    }

    if (address + len > this->getSize()) {
        return EMULATOR_ERROR;
    }

    memcpy(data, this->memory.get() + address, len);

    return EMULATOR_OK;
}

StorageEmulatorStatus StorageEmulator::writePage(uint32_t address, uint8_t* data, uint32_t len)
{
    requestsCount[address / Page::PAGE_SIZE].write++;
    if (this->isBusy) {
        return EMULATOR_BUSY;
    }

    if (len > 256) {
        return EMULATOR_ERROR;
    }

    if (address + len > this->getSize()) {
        return EMULATOR_ERROR;
    }

    memcpy(this->memory.get() + address, data, len);

    return EMULATOR_OK;
}

void StorageEmulator::clear()
{
    memset(this->memory.get(), 0xFF, this->getSize());
}

void StorageEmulator::showReadWrite()
{
    std::cout << "Requests to memory count (Pages count: " << this->pagesCount << ")." << std::endl;
    for (unsigned i = 0; i < this->pagesCount; i++) {
        if (i % StorageSector::PAGES_COUNT == 0) {
            std::cout << "|===============================|" << std::endl;
        } else if (i % StorageSector::PAGES_COUNT == StorageSector::RESERVED_PAGES_COUNT) {
            std::cout << "|-------------------------------|" << std::endl;
        }
        std::cout << "| " << i << "\tpage:\tr-" << requestsCount[i].read << "\tw-" << requestsCount[i].write << "\t|" <<std::endl;
    }
    std::cout << "|===============================|" << std::endl;
}

void StorageEmulator::showPage(uint32_t address)
{
    const uint8_t cols_count = 16;
    uint32_t i = 0;
    uint32_t start_counter = address;
    do {
        printf("%08X: ", (unsigned int)(start_counter + i));
        for (uint32_t j = 0; j < cols_count; j++) {
            if (i * cols_count + j > Page::PAGE_SIZE) {
            	printf("   ");
            } else {
            	printf("%02X ", this->memory[i * cols_count + j]);
            }
            if ((j + 1) % 8 == 0) {
            	printf("| ");
            }
        }
        for (uint32_t j = 0; j < cols_count; j++) {
            if (i * cols_count + j > Page::PAGE_SIZE) {
                break;
            }
            char c = this->memory[i * cols_count + j];
            if (c > 31 && c != 0xFF) {
                printf("%c", (char)c);
            } else {
                printf(".");
            }
        }
        printf("\n");
        i++;
    } while (i * cols_count < Page::PAGE_SIZE);
}