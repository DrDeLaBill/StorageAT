#include <gtest/gtest.h>
#include <string>
// #include "gmock/gmock.h"

#include "../StorageAT/include/StorageAT.h"
#include "../StorageAT/include/StoragePage.h"
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
    storage.clear();
    StorageAT sat(
        storage.getPagesCount(),
        read_driver,
        write_driver
    );

    // Packet structures tests
    EXPECT_EQ(sizeof(struct Page::_PageMeta), 18);
    EXPECT_EQ(sizeof(struct Page::_PageStruct), PAGE_LEN);
    EXPECT_EQ(sizeof(struct HeaderPage::_PageHeader), 9);

    uint32_t address = 0;
    EXPECT_EQ(sat.find(static_cast<StorageFindMode>(0), &address, nullptr, 0), STORAGE_ERROR);
    EXPECT_EQ(sat.find(FIND_MODE_EQUAL, &address, nullptr, 0), STORAGE_NOT_FOUND);
    EXPECT_EQ(sat.find(FIND_MODE_MAX, &address, nullptr, 0), STORAGE_NOT_FOUND);
    EXPECT_EQ(sat.find(FIND_MODE_MIN, &address, nullptr, 0), STORAGE_NOT_FOUND);
    EXPECT_EQ(sat.find(FIND_MODE_NEXT, &address, nullptr, 0), STORAGE_NOT_FOUND);
    EXPECT_EQ(sat.save(address, reinterpret_cast<uint8_t*>(""), 0, nullptr, 0), STORAGE_ERROR);
    EXPECT_EQ(sat.save(address, reinterpret_cast<uint8_t*>(""), 0, nullptr, 1000), STORAGE_ERROR);
    EXPECT_EQ(sat.save(address, reinterpret_cast<uint8_t*>(""), 0, nullptr, 1000000), STORAGE_ERROR);
    EXPECT_EQ(sat.save(address, nullptr, 0, nullptr, 0), STORAGE_ERROR);
    EXPECT_EQ(sat.save(address, nullptr, 0, nullptr, 1000), STORAGE_ERROR);
    EXPECT_EQ(sat.save(address, nullptr, 0, nullptr, 1000000), STORAGE_ERROR);
    EXPECT_EQ(sat.load(address, nullptr, 0), STORAGE_ERROR);
    EXPECT_EQ(sat.load(address, nullptr, 1000), STORAGE_ERROR);
    EXPECT_EQ(sat.load(address, nullptr, 1000000), STORAGE_ERROR);
}

TEST(Core, object)
{
    storage.clear();
    StorageAT sat(
        storage.getPagesCount(),
        read_driver,
        write_driver
    );

    uint8_t emptyData256[256] = { 0xFF };
    uint8_t data256[256] = {};

    EXPECT_EQ(sat.load(0, data256, 0), STORAGE_ERROR);
    EXPECT_EQ(sat.load(0, data256, PAGE_LEN), STORAGE_ERROR);


    const uint8_t testPrefix[Page::STORAGE_PAGE_PREFIX_SIZE] = "tst";

    struct ReadData100 {
        uint8_t data[100];
    };
    // Test save without find empty address
    ReadData100 saveData100 = { 1, 2, 3, 4, 5 };
    EXPECT_EQ(sat.save(0, const_cast<uint8_t*>(testPrefix), 1, reinterpret_cast<uint8_t*>(&saveData100), sizeof(ReadData100)), STORAGE_ERROR);
    ReadData100 readData100 = {};
    EXPECT_EQ(sat.load(0, reinterpret_cast<uint8_t*>(&readData100), sizeof(ReadData100)), STORAGE_ERROR);
    // Test save with find empty address
    uint32_t emptyAddress = 0;
    EXPECT_EQ(sat.find(FIND_MODE_EMPTY, &emptyAddress), STORAGE_OK);
    EXPECT_EQ(sat.save(emptyAddress, const_cast<uint8_t*>(testPrefix), 1, reinterpret_cast<uint8_t*>(&saveData100), sizeof(ReadData100)), STORAGE_OK);
    EXPECT_EQ(sat.load(emptyAddress, reinterpret_cast<uint8_t*>(&readData100), sizeof(ReadData100)), STORAGE_OK);
    EXPECT_FALSE(memcmp(reinterpret_cast<void*>(&saveData100), reinterpret_cast<void*>(&readData100), sizeof(saveData100)));
}