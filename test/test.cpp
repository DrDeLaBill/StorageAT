#include <gtest/gtest.h>
#include <string>
// #include "gmock/gmock.h"

#include "../StorageAT/include/StorageAT.h"
#include "StorageEmulator.h"


const int SECTORS_COUNT = 20;
const int PAGES_COUNT   = StorageSector::SECTOR_PAGES_COUNT * SECTORS_COUNT;
const int PAGE_LEN      = 256;

StorageEmulator storage(PAGES_COUNT);

StorageStatus read_driver(uint32_t address, uint8_t* data, uint32_t len) 
{
    StorageEmulatorStatus status = storage.readPage(address, data, len);
    if (status == EMULATOR_BUSY) {
        return STORAGE_BUSY;
    }
    if (status == EMULATOR_ERROR) {
        return STORAGE_ERROR;
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
        return STORAGE_ERROR;
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

    // Page tests
    



    // Trash requests
    uint32_t address = 0;
    EXPECT_EQ(sat.find(static_cast<StorageFindMode>(0), &address, nullptr, 0), STORAGE_ERROR);
    EXPECT_EQ(sat.find(FIND_MODE_EMPTY, &address), STORAGE_OK);
    EXPECT_EQ(sat.find(FIND_MODE_EQUAL, &address, nullptr, 0), STORAGE_ERROR);
    EXPECT_EQ(sat.find(FIND_MODE_MAX, &address, nullptr, 0), STORAGE_ERROR);
    EXPECT_EQ(sat.find(FIND_MODE_MIN, &address, nullptr, 0), STORAGE_ERROR);
    EXPECT_EQ(sat.find(FIND_MODE_NEXT, &address, nullptr, 0), STORAGE_ERROR);
    EXPECT_EQ(sat.find(FIND_MODE_EMPTY, nullptr), STORAGE_ERROR);
    EXPECT_EQ(sat.find(FIND_MODE_EQUAL, nullptr, nullptr, 0), STORAGE_ERROR);
    EXPECT_EQ(sat.find(FIND_MODE_MAX, nullptr, nullptr, 0), STORAGE_ERROR);
    EXPECT_EQ(sat.find(FIND_MODE_MIN, nullptr, nullptr, 0), STORAGE_ERROR);
    EXPECT_EQ(sat.find(FIND_MODE_NEXT, nullptr, nullptr, 0), STORAGE_ERROR);
    EXPECT_EQ(sat.find(FIND_MODE_EQUAL, nullptr, const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>("nullptr")), 0), STORAGE_ERROR);
    EXPECT_EQ(sat.find(FIND_MODE_MAX, nullptr, const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>("nullptr")), 0), STORAGE_ERROR);
    EXPECT_EQ(sat.find(FIND_MODE_MIN, nullptr, const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>("nullptr")), 0), STORAGE_ERROR);
    EXPECT_EQ(sat.find(FIND_MODE_NEXT, nullptr, const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>("nullptr")), 0), STORAGE_ERROR);
    EXPECT_EQ(sat.find(FIND_MODE_EQUAL, nullptr, const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>("tst")), 0), STORAGE_ERROR);
    EXPECT_EQ(sat.find(FIND_MODE_MAX, nullptr, const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>("tst")), 0), STORAGE_ERROR);
    EXPECT_EQ(sat.find(FIND_MODE_MIN, nullptr, const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>("tst")), 0), STORAGE_ERROR);
    EXPECT_EQ(sat.find(FIND_MODE_NEXT, nullptr, const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>("tst")), 0), STORAGE_ERROR);
    EXPECT_EQ(sat.save(address, const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>("")), 0, nullptr, 0), STORAGE_ERROR);
    EXPECT_EQ(sat.save(address, const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>("")), 0, nullptr, 1000), STORAGE_ERROR);
    EXPECT_EQ(sat.save(address, const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>("")), 0, nullptr, 1000000), STORAGE_ERROR);
    EXPECT_EQ(sat.save(address, nullptr, 0, nullptr, 0), STORAGE_ERROR);
    EXPECT_EQ(sat.save(address, nullptr, 0, nullptr, 1000), STORAGE_ERROR);
    EXPECT_EQ(sat.save(address, nullptr, 0, nullptr, 1000000), STORAGE_ERROR);
    EXPECT_EQ(sat.load(address, nullptr, 0), STORAGE_ERROR);
    EXPECT_EQ(sat.load(address, nullptr, 1000), STORAGE_ERROR);
    EXPECT_EQ(sat.load(address, nullptr, 1000000), STORAGE_ERROR);

    // Test busy
    storage.setBusy(true);
    EXPECT_EQ(sat.find(FIND_MODE_EQUAL, &address, const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>("")), 0), STORAGE_BUSY);
    EXPECT_EQ(sat.find(FIND_MODE_MAX, &address, const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>("")), 0), STORAGE_BUSY);
    EXPECT_EQ(sat.find(FIND_MODE_MIN, &address, const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>("")), 0), STORAGE_BUSY);
    EXPECT_EQ(sat.find(FIND_MODE_NEXT, &address, const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>("")), 0), STORAGE_BUSY);
    storage.setBusy(false);

    // Test write all bytes
    EXPECT_EQ(sat.find(FIND_MODE_EMPTY, &address), STORAGE_OK);
    EXPECT_EQ(sat.save(address, const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>("")), 0, (new uint8_t[PAGES_COUNT * PAGE_LEN + 1]), PAGES_COUNT * PAGE_LEN + 1), STORAGE_OOM);


    // Test read and write unacceptable address
    StorageAT sat2(
        storage.getPagesCount() * 2,
        read_driver,
        write_driver
    );
    uint32_t sat2Address = storage.getPagesCount() * PAGE_LEN * 2 - 1024;
    EXPECT_EQ(sat2.load(sat2Address, (new uint8_t[PAGE_LEN]), PAGE_LEN), STORAGE_ERROR);
    EXPECT_EQ(sat2.save(sat2Address, const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>("tst")), 1, (new uint8_t[PAGE_LEN]), PAGE_LEN), STORAGE_NOT_FOUND);

    // Test write all
    storage.clear();
    EXPECT_EQ(sat2.find(FIND_MODE_EMPTY, &sat2Address), STORAGE_OK);
    uint32_t maxEmptyMemory = SECTORS_COUNT * HeaderPage::PAGE_HEADERS_COUNT * Page::STORAGE_PAGE_PAYLOAD_SIZE;
    EXPECT_EQ(sat2.save(sat2Address, const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>("tst")), 1, (new uint8_t[maxEmptyMemory]), maxEmptyMemory), STORAGE_OK);

    // TODO: проверка записи в последний сектор, если он не выровненный по концу памяти
}

TEST(Core, object)
{
    storage.clear();
    StorageAT sat(
        storage.getPagesCount(),
        read_driver,
        write_driver
    );
    const uint8_t emptyPage256[256] = { 0xFF };

    // Try to load empty pages
    uint8_t data256[256] = {};
    EXPECT_EQ(sat.load(0, data256, 0), STORAGE_ERROR);
    EXPECT_EQ(sat.load(0, data256, PAGE_LEN), STORAGE_ERROR);


    const uint8_t testPrefix[Page::STORAGE_PAGE_PREFIX_SIZE] = "tst";
    struct ReadData100 {
        uint8_t data[100];
    };


    // Test save 100 bytes structure without find empty address
    ReadData100 saveData100 = { 1, 2, 3, 4, 5 };
    EXPECT_EQ(sat.save(0, const_cast<uint8_t*>(testPrefix), 1, reinterpret_cast<uint8_t*>(&saveData100), sizeof(saveData100)), STORAGE_ERROR);
    ReadData100 readData100 = {};
    EXPECT_EQ(sat.load(0, reinterpret_cast<uint8_t*>(&readData100), sizeof(readData100)), STORAGE_ERROR);

    // Test save 100 bytes structure with find empty address
    uint32_t emptyAddress = 0;
    EXPECT_EQ(sat.find(FIND_MODE_EMPTY, &emptyAddress), STORAGE_OK);
    EXPECT_EQ(emptyAddress, 2048);
    EXPECT_EQ(sat.save(emptyAddress, const_cast<uint8_t*>(testPrefix), 1, reinterpret_cast<uint8_t*>(&saveData100), sizeof(saveData100)), STORAGE_OK);
    EXPECT_EQ(sat.save(emptyAddress, const_cast<uint8_t*>(testPrefix), 1, reinterpret_cast<uint8_t*>(&data256), sizeof(data256)), STORAGE_ERROR);
    EXPECT_EQ(sat.load(emptyAddress, reinterpret_cast<uint8_t*>(&readData100), sizeof(ReadData100)), STORAGE_OK);
    EXPECT_FALSE(memcmp(reinterpret_cast<void*>(&saveData100), reinterpret_cast<void*>(&readData100), sizeof(saveData100)));


    struct ReadData1000 {
        uint8_t data[1000];
    };
    // Test save 1000 bytes structure without find empty address
    ReadData1000 saveData1000 = { 1, 2, 3, 4, 5 };
    EXPECT_EQ(sat.save(0, const_cast<uint8_t*>(testPrefix), 1, reinterpret_cast<uint8_t*>(&saveData1000), sizeof(saveData1000)), STORAGE_ERROR);
    ReadData1000 readData1000 = { 0 };
    EXPECT_EQ(sat.load(0, reinterpret_cast<uint8_t*>(&readData1000), sizeof(readData1000)), STORAGE_ERROR);

    // Test save 1000 bytes structure with find empty address
    emptyAddress = 0;
    EXPECT_EQ(sat.find(FIND_MODE_EMPTY, &emptyAddress), STORAGE_OK);
    EXPECT_EQ(emptyAddress, 2304);
    EXPECT_EQ(sat.save(emptyAddress, const_cast<uint8_t*>(testPrefix), 2, reinterpret_cast<uint8_t*>(&saveData1000), sizeof(saveData1000)), STORAGE_OK);
    EXPECT_EQ(sat.load(emptyAddress + PAGE_LEN, reinterpret_cast<uint8_t*>(&readData1000), sizeof(readData1000)), STORAGE_ERROR);
    EXPECT_EQ(sat.load(emptyAddress, reinterpret_cast<uint8_t*>(&readData1000), sizeof(readData1000)), STORAGE_OK);
    EXPECT_FALSE(memcmp(reinterpret_cast<void*>(&readData1000), reinterpret_cast<void*>(&saveData1000), sizeof(readData1000)));
    // Try to save new data in busy pages
    EXPECT_EQ(sat.save(emptyAddress, const_cast<uint8_t*>(testPrefix), 2, reinterpret_cast<uint8_t*>(&data256), sizeof(data256)), STORAGE_ERROR);
    EXPECT_EQ(sat.save(emptyAddress + PAGE_LEN, const_cast<uint8_t*>(testPrefix), 2, reinterpret_cast<uint8_t*>(&data256), sizeof(data256)), STORAGE_ERROR);
    EXPECT_EQ(sat.save(3328, const_cast<uint8_t*>(testPrefix), 2, reinterpret_cast<uint8_t*>(&data256), sizeof(data256)), STORAGE_ERROR);
    EXPECT_EQ(sat.load(emptyAddress, reinterpret_cast<uint8_t*>(&readData1000), sizeof(readData1000)), STORAGE_OK);
    EXPECT_FALSE(memcmp(reinterpret_cast<void*>(&readData1000), reinterpret_cast<void*>(&saveData1000), sizeof(readData1000)));
    
    // Test load and save not aligned address
    EXPECT_EQ(sat.save(1000, const_cast<uint8_t*>(testPrefix), 2, reinterpret_cast<uint8_t*>(&saveData1000), sizeof(saveData1000)), STORAGE_ERROR);
    EXPECT_EQ(sat.load(1000, reinterpret_cast<uint8_t*>(&readData1000), sizeof(readData1000)), STORAGE_ERROR);

    // Test find
    uint32_t foundAddress = 0;
    EXPECT_EQ(sat.find(FIND_MODE_EQUAL, &foundAddress, const_cast<uint8_t*>(testPrefix), 2), STORAGE_OK);
    EXPECT_EQ(foundAddress, emptyAddress);
    EXPECT_EQ(sat.load(foundAddress, reinterpret_cast<uint8_t*>(&readData1000), sizeof(readData1000)), STORAGE_OK);
    EXPECT_FALSE(memcmp(reinterpret_cast<void*>(&readData1000), reinterpret_cast<void*>(&saveData1000), sizeof(readData1000)));

    // Test clear storage
    storage.clear();
    EXPECT_EQ(sat.load(emptyAddress, reinterpret_cast<uint8_t*>(&readData1000), sizeof(readData1000)), STORAGE_ERROR);

    // Test partiotioned data
    uint32_t newAddress1 = 0, newAddress2 = 0;
    EXPECT_EQ(sat.find(FIND_MODE_EMPTY, &newAddress1), STORAGE_OK);
    EXPECT_EQ(sat.save(newAddress1 + PAGE_LEN, const_cast<uint8_t*>(testPrefix), 1, reinterpret_cast<uint8_t*>(&saveData100), sizeof(saveData100)), STORAGE_OK);
    EXPECT_EQ(sat.find(FIND_MODE_EMPTY, &newAddress2), STORAGE_OK);
    EXPECT_EQ(sat.save(newAddress2, const_cast<uint8_t*>(testPrefix), 2, reinterpret_cast<uint8_t*>(&saveData1000), sizeof(saveData1000)), STORAGE_OK);
    EXPECT_EQ(newAddress1, newAddress2);
    uint32_t foundNewAddress1 = 0, foundNewAddress2 = 0;
    EXPECT_EQ(sat.find(FIND_MODE_EQUAL, &foundNewAddress1, const_cast<uint8_t*>(testPrefix), 1), STORAGE_OK);
    EXPECT_NE(newAddress1, foundNewAddress1);
    EXPECT_EQ(sat.find(FIND_MODE_EQUAL, &foundNewAddress2, const_cast<uint8_t*>(testPrefix), 2), STORAGE_OK);
    EXPECT_EQ(newAddress2, foundNewAddress2);
    EXPECT_LT(foundNewAddress2, foundNewAddress1);
    EXPECT_EQ(sat.load(foundNewAddress1, reinterpret_cast<uint8_t*>(&readData100), sizeof(readData100)), STORAGE_OK);
    EXPECT_FALSE(memcmp(reinterpret_cast<void*>(&readData100), reinterpret_cast<void*>(&saveData100), sizeof(readData100)));
    EXPECT_EQ(sat.load(foundNewAddress2, reinterpret_cast<uint8_t*>(&readData1000), sizeof(readData1000)), STORAGE_OK);
    EXPECT_FALSE(memcmp(reinterpret_cast<void*>(&readData1000), reinterpret_cast<void*>(&saveData1000), sizeof(readData1000)));

    // TODO: тест с заблокированными страницами в headers, а также недоступными в памяти
}