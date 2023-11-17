/* Copyright © 2023 Georgy E. All rights reserved. */

#include "StorageEmulator.h"

#include <memory>
#include <iostream>
#include <string.h>
#include <String>

#include "StorageAT.h"


StorageEmulator::StorageEmulator(uint32_t pagesCount): pagesCount(pagesCount), size(pagesCount * Page::PAGE_SIZE)
{
    this->memory = std::make_unique<uint8_t[]>(this->getSize());
    this->blocked = std::make_unique<bool[]>(this->getSize());
    this->requestsCount = std::make_unique<RequestsCount[]>(this->pagesCount);
    this->clear();
    this->isBusy = false;
}

uint32_t StorageEmulator::getSize()
{
    return this->getPagesCount() * Page::PAGE_SIZE;
}

uint32_t  StorageEmulator::getPayloadSize()
{
    uint32_t payloadPages = (getPagesCount() / StorageMacroblock::PAGES_COUNT) * Header::PAGES_COUNT;
    if (getPagesCount() % StorageMacroblock::PAGES_COUNT > StorageMacroblock::RESERVED_PAGES_COUNT) {
        payloadPages += (getPagesCount() % StorageMacroblock::PAGES_COUNT) - StorageMacroblock::RESERVED_PAGES_COUNT;
    }
    return payloadPages * Page::PAYLOAD_SIZE;
}

uint32_t StorageEmulator::getPagesCount()
{
    return  this->pagesCount;
}

void StorageEmulator::setBusy(bool busy)
{
    this->isBusy = busy;
}

void StorageEmulator::setBlocked(uint32_t idx, bool blocked)
{
    if (idx > StorageEmulator::getSize()) {
        return;
    }
    this->blocked[idx] = blocked;
}

StorageEmulatorStatus StorageEmulator::readPage(uint32_t address, uint8_t* data, uint32_t len)
{
    if (address + len > this->size) {
        return EMULATOR_OOM;
    }
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
    if (address + len > this->size) {
        return EMULATOR_OOM;
    }
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

    for (unsigned i = 0; i < len; i++) {
        if (this->blocked[address + i]) {
            continue;
        }
        this->memory[address + i] = data[i];
    }

    return EMULATOR_OK;
}

void StorageEmulator::clear()
{
    memset(this->memory.get(), 0xFF, this->getSize());
    for (unsigned i = 0; i < this->getSize(); i++) {
        this->blocked[i] = false;
    }
}

void StorageEmulator::showReadWrite()
{
    std::cout << "Requests to memory count (Pages count: " << this->pagesCount << ")." << std::endl;
    std::cout << "Payload size: " << (getPayloadSize() * 100 / getSize()) << "% (" << this->getPayloadSize() << " byte(s) of " << this->getSize() << " byte(s))" << std::endl;
    for (unsigned i = 0; i < this->pagesCount; i++) {
        unsigned sectorIndex = i / StorageMacroblock::PAGES_COUNT;
        if (i % StorageMacroblock::PAGES_COUNT == 0) {
            std::cout << "|============ " << sectorIndex << " ";
            for (unsigned j = 0; j < 17 - std::to_string(sectorIndex).length(); j++)
                std::cout << "=";
            std::cout << "|" << std::endl;
        } else if (i % StorageMacroblock::PAGES_COUNT == StorageMacroblock::RESERVED_PAGES_COUNT) {
            std::cout << "|-------------------------------|" << std::endl;
        }
        std::cout << "| " << (i % StorageMacroblock::PAGES_COUNT) << "\tpage:\tr-" << requestsCount[i].read << "\tw-" << requestsCount[i].write << "\t|" <<std::endl;
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