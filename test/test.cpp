#include <gtest/gtest.h>
#include <string>
// #include "gmock/gmock.h"

#include "../StorageAT/include/StorageAT.h"
#include "StorageEmulator.h"


const int PAGES_COUNT = 200; // bytes
const int PAGE_LEN    = 256;

StorageEmulator storage(PAGES_COUNT);

StorageStatus read_driver(uint32_t address, uint8_t* data, uint32_t len) 
{
    StorageEmulatorStatus status = storage.readPage(address, data, len);
    if (status == EMULATOR_BUSY) {
        return STORAGE_BUSY;
    }
    if (status == EMULATOR_ERROR) {
        STORAGE_ERROR;
    }
    return STORAGE_OK;
};

StorageStatus write_driver(uint32_t address, uint8_t* data, uint32_t len) 
{
    StorageEmulatorStatus status = storage.writePage(address, data, len);
    if (status == EMULATOR_BUSY) {
        return STORAGE_BUSY;
    }
    if (status == EMULATOR_ERROR) {
        STORAGE_ERROR;
    }
    return STORAGE_OK;
};


TEST(Core, primitive)
{
    StorageAT sat(
        storage.getPagesCount(),
        read_driver,
        write_driver
    );

    uint8_t emptyData256[256] = { 0xFF };

    uint8_t data256[256] = {};
    EXPECT_EQ(sat.load(0, data256, 0), STORAGE_ERROR);
    EXPECT_EQ(sat.load(0, data256, PAGE_LEN), STORAGE_ERROR);
}