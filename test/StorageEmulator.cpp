#include "StorageEmulator.h"

#include <memory>
#include <string.h>


StorageEmulator::StorageEmulator(uint32_t pagesCount): pagesCount(pagesCount) 
{
    this->memory = std::make_unique<uint8_t[]>(this->getSize());
    this->clear();
    this->isBusy = false;
}

uint32_t StorageEmulator::getSize()
{
    return this->getPagesCount() * 256;;
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