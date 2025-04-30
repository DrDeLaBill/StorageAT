
/* Copyright © 2025 Georgy E. All rights reserved. */

#include "StorageDriver.hpp"

#include "StorageFixture.hpp"


StorageStatus StorageDriver::read(const uint32_t address, uint8_t* data, const uint32_t len)
{
    StorageEmulatorStatus status = StorageFixture::storage.readPage(address, data, len);
    if (status == EMULATOR_BUSY) {
        return STORAGE_BUSY;
    }
    if (status == EMULATOR_OOM) {
        return STORAGE_OOM;
    }
    if (status == EMULATOR_ERROR) {
        return STORAGE_ERROR;
    }
    return STORAGE_OK;
}

StorageStatus StorageDriver::write(const uint32_t address, const uint8_t* data, const uint32_t len)
{
    StorageEmulatorStatus status = StorageFixture::storage.writePage(address, data, len);
    if (status == EMULATOR_BUSY) {
        return STORAGE_BUSY;
    }
    if (status == EMULATOR_OOM) {
        return STORAGE_OOM;
    }
    if (status == EMULATOR_ERROR) {
        return STORAGE_ERROR;
    }
    return STORAGE_OK;
}

StorageStatus StorageDriver::erase(const uint32_t* addresses, const uint32_t count)
{
    StorageEmulatorStatus status = StorageFixture::storage.erase(addresses, count);
    if (status == EMULATOR_BUSY) {
        return STORAGE_BUSY;
    }
    if (status == EMULATOR_OOM) {
        return STORAGE_OOM;
    }
    if (status == EMULATOR_ERROR) {
        return STORAGE_ERROR;
    }
    return STORAGE_OK;
}

StorageStatus StorageDriver::asyncRead(const uint32_t address, uint8_t* data, const uint32_t len)
{ 
    StorageStatus status = read(address, data, len);
    StorageFixture::sat->callback(status);
    return status;
}

StorageStatus StorageDriver::asyncWrite(const uint32_t address, const uint8_t* data, const uint32_t len)
{
    StorageStatus status = write(address, data, len);
    StorageFixture::sat->callback(status);
    return status;
}

StorageStatus StorageDriver::asyncErase(const uint32_t* addresses, const uint32_t count)
{
    StorageStatus status = erase(addresses, count);
    StorageFixture::sat->callback(status);
    return status;
}