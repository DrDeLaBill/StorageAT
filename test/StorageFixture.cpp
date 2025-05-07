/* Copyright © 2025 Georgy E. All rights reserved. */

#include "StorageFixture.hpp"


uint32_t StorageFixture::MIN_MEMORY_ERASE_SIZE = STORAGE_PAGE_SIZE;
uint32_t StorageFixture::address = 0;
StorageStatus StorageFixture::status = STORAGE_OK; 
StorageDriver StorageFixture::driver;
std::unique_ptr<StorageAT> StorageFixture::sat;
StorageEmulator StorageFixture::storage(StorageFixture::PAGES_COUNT);

utl::Timer StorageFixtureAsync::timer(0);
bool StorageFixtureAsync::asyncReady = false;


void StorageFixture::SetUp()
{
    storage.clear();
    storage.setBusy(false);
    address = 0;
    sat = std::make_unique<StorageAT>(
        storage.getPagesCount(),
        &driver,
        MIN_MEMORY_ERASE_SIZE
    );
}

void StorageFixture::TearDown()
{
    sat.reset();
}

void StorageFixtureAsync::asyncCallback(StorageStatus _status)
{
    StorageFixture::status = _status;
    asyncReady = true;
}

void StorageFixtureAsync::SetUp()
{
    asyncReady = false;
    timer.changeDelay(10000);
    StorageFixture::SetUp();
}

StorageStatus StorageFixtureAsync::asyncFind(
    StorageFindMode mode,
    uint32_t*       address,
    const char*     prefix,
    uint32_t        id
) {
    asyncReady = false;
    StorageStatus status = sat->asyncFind(mode, address, asyncCallback, prefix, id);
    if (status != STORAGE_OK) {
        return status;
    }
    // timer.start();
    // while (timer.wait() && !asyncReady) {
    while (!asyncReady) {
        sat->tick();
    }
    // if (!timer.wait()) {
    //     return STORAGE_ERROR;
    // }
    return StorageFixtureAsync::status;
}

StorageStatus StorageFixtureAsync::asyncLoad(uint32_t address, uint8_t* data, uint32_t len)
{
    asyncReady = false;
    StorageStatus status = sat->asyncLoad(address, data, len, asyncCallback);
    if (status != STORAGE_OK) {
        return status;
    }
    // timer.start();
    // while (timer.wait() && !asyncReady) {
    while (!asyncReady) {
        sat->tick();
    }
    // if (!timer.wait()) {
    //     return STORAGE_ERROR;
    // }
    return StorageFixtureAsync::status;
}

StorageStatus StorageFixtureAsync::asyncSave(
    uint32_t    address,
    const char* prefix,
    uint32_t    id,
    uint8_t*    data,
    uint32_t    len
) {
    asyncReady = false;
    StorageStatus status = sat->asyncSave(address, prefix, id, data, len, asyncCallback);
    if (status != STORAGE_OK) {
        return status;
    }
    // timer.start();
    // while (timer.wait() && !asyncReady) {
    while (!asyncReady) {
        sat->tick();
    }
    // if (!timer.wait()) {
    //     return STORAGE_ERROR;
    // }
    return StorageFixtureAsync::status;
}

StorageStatus StorageFixtureAsync::asyncRewrite(
    uint32_t    address,
    const char* prefix,
    uint32_t    id,
    uint8_t*    data,
    uint32_t    len
) {
    asyncReady = false;
    StorageStatus status = sat->asyncRewrite(address, prefix, id, data, len, asyncCallback);
    if (status != STORAGE_OK) {
        return status;
    }
    // timer.start();
    // while (timer.wait() && !asyncReady) {
    while (!asyncReady) {
        sat->tick();
    }
    // if (!timer.wait()) {
    //     return STORAGE_ERROR;
    // }
    return StorageFixtureAsync::status;
}