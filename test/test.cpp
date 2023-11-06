#include <iostream>
#include <string>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "StorageAT.h"
#include "StorageEmulator.h"


const int SECTORS_COUNT = 20;
const int PAGES_COUNT   = StorageSector::SECTOR_PAGES_COUNT * SECTORS_COUNT;
const int PAGE_LEN      = 256;


StorageEmulator storage(PAGES_COUNT);


typedef struct _RequestsCount {
    unsigned read;
    unsigned write;
} RequestsCount;

RequestsCount requestsCount[PAGES_COUNT] = {};


class StorageDriver: public IStorageDriver
{
public: 
    StorageDriver() {}
	StorageStatus read(uint32_t address, uint8_t* data, uint32_t len) override
    {
        requestsCount[address / PAGE_LEN].read++;
        StorageEmulatorStatus status = storage.readPage(address, data, len);
        if (status == EMULATOR_BUSY) {
            return STORAGE_BUSY;
        }
        if (status == EMULATOR_ERROR) {
            return STORAGE_ERROR;
        }
        return STORAGE_OK;
    }
	StorageStatus write(uint32_t address, uint8_t* data, uint32_t len) override
    {
        requestsCount[address / PAGE_LEN].write++;
        StorageEmulatorStatus status = storage.writePage(address, data, len);
        if (status == EMULATOR_BUSY) {
            return STORAGE_BUSY;
        }
        if (status == EMULATOR_ERROR) {
            return STORAGE_ERROR;
        }
        return STORAGE_OK;
    }
};


class MockStorageDriver: public IStorageDriver
{
public:
    MockStorageDriver() {}

    MOCK_METHOD(
        StorageStatus,
        read,
        (uint32_t, uint8_t*, uint32_t),
        (override)
    );

    MOCK_METHOD(
        StorageStatus,
        write,
        (uint32_t, uint8_t*, uint32_t),
        (override)
    );
};


class StorageFixture: public testing::Test
{
public:
    uint32_t address;
    StorageDriver driver;
    std::unique_ptr<StorageAT> sat;

    void SetUp() override
    {
        storage.clear();
        address = 0;
        sat = std::make_unique<StorageAT>(
            storage.getPagesCount(),
            &driver
        );
    }

    void TearDown() override
    {
        sat.reset();
    }
};


TEST(Page, Struct)
{
    EXPECT_EQ(sizeof(struct Page::_PageMeta), 18);
    EXPECT_EQ(sizeof(struct Page::_PageStruct), PAGE_LEN);
}

TEST(Header, Struct)
{
    EXPECT_EQ(sizeof(struct Header::_PageHeader), 9);
}

TEST(StorageSector, CheckSectorAddresses)
{
    struct SectorAddressIndex {
        uint32_t address;
        uint32_t sectorAddress;
        uint32_t sectorIndex;
        uint32_t pageIndex;
        uint32_t indexAddress;
        bool isSectorAddress;
    };
    SectorAddressIndex addrIndex[] = {
        {0,     0,    0,    0,    2048,  true},
        {1,     0,    0,    0,    2048,  true},
        {255,   0,    0,    0,    2048,  true},
        {256,   0,    0,    0,    2048,  true},
        {2048,  0,    0,    0,    2048,  false},
        {2049,  0,    0,    0,    2048,  false},
        {8703,  0,    0,    25,   8448,  false},
        {8704,  8704, 1,    0,    10752, true},
        {8705,  8704, 1,    0,    10752, true},
        {8959,  8704, 1,    0,    10752, true},
        {11008, 8704, 1,    1,    11008, false},
    };

    for (unsigned i = 0; i < sizeof(addrIndex) / sizeof(*addrIndex); i++) {
        EXPECT_EQ(StorageSector::getSectorAddress(addrIndex[i].sectorIndex), addrIndex[i].sectorAddress);
        EXPECT_EQ(StorageSector::getSectorIndex(addrIndex[i].sectorAddress), addrIndex[i].sectorIndex);
        EXPECT_EQ(StorageSector::getSectorIndex(addrIndex[i].address), addrIndex[i].sectorIndex);
        EXPECT_EQ(StorageSector::getPageAddressByIndex(addrIndex[i].sectorIndex, addrIndex[i].pageIndex), addrIndex[i].indexAddress);
        EXPECT_EQ(StorageSector::getPageIndexByAddress(addrIndex[i].address), addrIndex[i].pageIndex);
        EXPECT_EQ(StorageSector::isSectorAddress(addrIndex[i].address), addrIndex[i].isSectorAddress);
        EXPECT_EQ(Header::getSectorStartAddress(addrIndex[i].address), addrIndex[i].sectorAddress);
    }
}

TEST(StorageDriver, RequestExists)
{
    storage.clear();
    MockStorageDriver mockDriver;
    StorageAT sat(
        storage.getPagesCount(),
        &mockDriver
    );
    uint8_t prefix[] = "tst";
    uint32_t id = 0;
    uint8_t data[] = { 1, 2, 3, 4, 5 };
    uint32_t address = 0;

    EXPECT_CALL(mockDriver, read)
        .Times(::testing::AtLeast(1));

    EXPECT_CALL(mockDriver, write)
        .Times(::testing::AtLeast(1));

    EXPECT_EQ(sat.find(FIND_MODE_EMPTY, &address), STORAGE_NOT_FOUND);
}

TEST_F(StorageFixture, BadFindRequest)
{
    uint8_t shortStr[] = "tst";
    uint8_t longStr[] = "longstringparameter";

    EXPECT_EQ(sat->find(static_cast<StorageFindMode>(0), &address, nullptr, 0), STORAGE_ERROR);
    EXPECT_EQ(sat->find(FIND_MODE_EMPTY, &address), STORAGE_OK);
    EXPECT_EQ(sat->find(FIND_MODE_EQUAL, &address, nullptr, 0), STORAGE_ERROR);
    EXPECT_EQ(sat->find(FIND_MODE_MAX, &address, nullptr, 0), STORAGE_ERROR);
    EXPECT_EQ(sat->find(FIND_MODE_MIN, &address, nullptr, 0), STORAGE_ERROR);
    EXPECT_EQ(sat->find(FIND_MODE_NEXT, &address, nullptr, 0), STORAGE_ERROR);
    EXPECT_EQ(sat->find(FIND_MODE_EMPTY, nullptr), STORAGE_ERROR);
    EXPECT_EQ(sat->find(FIND_MODE_EQUAL, nullptr, nullptr, 0), STORAGE_ERROR);
    EXPECT_EQ(sat->find(FIND_MODE_MAX, nullptr, nullptr, 0), STORAGE_ERROR);
    EXPECT_EQ(sat->find(FIND_MODE_MIN, nullptr, nullptr, 0), STORAGE_ERROR);
    EXPECT_EQ(sat->find(FIND_MODE_NEXT, nullptr, nullptr, 0), STORAGE_ERROR);
    EXPECT_EQ(sat->find(FIND_MODE_EQUAL, nullptr, longStr, 0), STORAGE_ERROR);
    EXPECT_EQ(sat->find(FIND_MODE_MAX, nullptr, longStr, 0), STORAGE_ERROR);
    EXPECT_EQ(sat->find(FIND_MODE_MIN, nullptr, longStr, 0), STORAGE_ERROR);
    EXPECT_EQ(sat->find(FIND_MODE_NEXT, nullptr, longStr, 0), STORAGE_ERROR);
    EXPECT_EQ(sat->find(FIND_MODE_EQUAL, nullptr, shortStr, 0), STORAGE_ERROR);
    EXPECT_EQ(sat->find(FIND_MODE_MAX, nullptr, shortStr, 0), STORAGE_ERROR);
    EXPECT_EQ(sat->find(FIND_MODE_MIN, nullptr, shortStr, 0), STORAGE_ERROR);
    EXPECT_EQ(sat->find(FIND_MODE_NEXT, nullptr, shortStr, 0), STORAGE_ERROR);
}

TEST_F(StorageFixture, BadSaveRequest)
{
    uint8_t emptyStr[] = "";

    EXPECT_EQ(sat->save(address, emptyStr, 0, nullptr, 0), STORAGE_ERROR);
    EXPECT_EQ(sat->save(address, emptyStr, 0, nullptr, 1000), STORAGE_ERROR);
    EXPECT_EQ(sat->save(address, emptyStr, 0, nullptr, 1000000), STORAGE_ERROR);
    EXPECT_EQ(sat->save(address, nullptr, 0, nullptr, 0), STORAGE_ERROR);
    EXPECT_EQ(sat->save(address, nullptr, 0, nullptr, 1000), STORAGE_ERROR);
    EXPECT_EQ(sat->save(address, nullptr, 0, nullptr, 1000000), STORAGE_ERROR);
}

TEST_F(StorageFixture, BadLoadRequest)
{
    EXPECT_EQ(sat->load(address, nullptr, 0), STORAGE_ERROR);
    EXPECT_EQ(sat->load(address, nullptr, 1000), STORAGE_ERROR);
    EXPECT_EQ(sat->load(address, nullptr, 1000000), STORAGE_ERROR);
}

TEST(Core, primitive)
{
    StorageDriver driver;
    storage.clear();
    StorageAT sat(
        storage.getPagesCount(),
        &driver
    );
    uint32_t address = 0;
    uint8_t shortStr[] = "tst";

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
        &driver
    );
    uint32_t sat2Address = storage.getPagesCount() * PAGE_LEN * 2 - 1024;
    EXPECT_EQ(sat2.load(sat2Address, (new uint8_t[PAGE_LEN]), PAGE_LEN), STORAGE_ERROR);
    EXPECT_EQ(sat2.save(sat2Address, shortStr, 1, (new uint8_t[PAGE_LEN]), PAGE_LEN), STORAGE_NOT_FOUND);

    // Test write all
    storage.clear();
    EXPECT_EQ(sat2.find(FIND_MODE_EMPTY, &sat2Address), STORAGE_OK);
    uint32_t maxEmptyMemory = SECTORS_COUNT * Header::PAGE_HEADERS_COUNT * Page::STORAGE_PAGE_PAYLOAD_SIZE;
    EXPECT_EQ(sat2.save(sat2Address, shortStr, 1, (new uint8_t[maxEmptyMemory]), maxEmptyMemory), STORAGE_OK);

    // TODO: проверка записи в последний сектор, если он не выровненный по концу памяти
}

TEST(Core, object)
{
    StorageDriver driver;
    storage.clear();
    StorageAT sat(
        storage.getPagesCount(),
        &driver
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

int main(int args, char** argv)
{
    testing::InitGoogleTest(&args, argv);
    int result = RUN_ALL_TESTS();

    std::cout << "Requests to memory count (Pages count: " << PAGES_COUNT << ")." << std::endl;
    for (unsigned i = 0; i < PAGES_COUNT; i++) {
        if (i % StorageSector::SECTOR_PAGES_COUNT == 0) {
            std::cout << "|===============================|" << std::endl;
        } else if (i % StorageSector::SECTOR_PAGES_COUNT == StorageSector::SECTOR_RESERVED_PAGES_COUNT) {
            std::cout << "|-------------------------------|" << std::endl;
        }
        std::cout << "| " << i << "\tpage:\tr-" << requestsCount[i].read << "\tw-" << requestsCount[i].write << "\t|" <<std::endl;
    }
    std::cout << "|===============================|" << std::endl;

    return result;
}