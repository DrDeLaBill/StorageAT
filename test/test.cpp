/* Copyright © 2023 Georgy E. All rights reserved. */

#include <iostream>
#include <string>
#include <chrono>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "StorageAT.h"
#include "StorageEmulator.h"


const int SECTORS_COUNT = 20;
const int PAGES_COUNT   = StorageMacroblock::PAGES_COUNT * SECTORS_COUNT;
const int PAGE_LEN      = STORAGE_PAGE_SIZE;

uint32_t minMemoryEraseSize = STORAGE_PAGE_SIZE;

StorageEmulator storage(PAGES_COUNT);


class StorageDriver: public IStorageDriver
{
public: 
    StorageDriver() {}
	StorageStatus read(const uint32_t address, uint8_t* data, const uint32_t len) override
    {
        StorageEmulatorStatus status = storage.readPage(address, data, len);
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
	StorageStatus write(const uint32_t address, const uint8_t* data, const uint32_t len) override
    {
        StorageEmulatorStatus status = storage.writePage(address, data, len);
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
	StorageStatus erase(const uint32_t* addresses, const uint32_t count) override
    {
        StorageEmulatorStatus status = storage.erase(addresses, count);
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
};


class MockStorageDriver: public IStorageDriver
{
public:
    MockStorageDriver() {}

    MOCK_METHOD(
        StorageStatus,
        read,
        (const uint32_t, uint8_t*, const uint32_t),
        (override)
    );

    MOCK_METHOD(
        StorageStatus,
        write,
        (const uint32_t, const uint8_t*, const uint32_t),
        (override)
    );

    MOCK_METHOD(
        StorageStatus,
        erase,
        (const uint32_t*, const uint32_t),
        (override)
    );
};


class StorageFixture: public testing::Test
{
protected:
    static constexpr char shortPrefix[] = "tst";
    static constexpr char longPrefix [] = "longstringprefix";
    static constexpr char brokenPrefix[5] = { 't', 'e', 's', 't', 's' };

public: 
    uint32_t address = 0;
    StorageDriver driver;
    std::unique_ptr<StorageAT> sat;

    void SetUp() override
    {
        storage.clear();
        storage.setBusy(false);
        address = 0;
        sat = std::make_unique<StorageAT>(
            storage.getPagesCount(),
            &driver,
            minMemoryEraseSize
        );
    }

    void TearDown() override
    {
        sat.reset();
    }
};


STORAGE_PACK(typedef struct, _TmpStruct {
    uint8_t value1;
});
TEST(StorageTypeSuite, Pack)
{
    ASSERT_EQ(sizeof(struct _TmpStruct), 1);
}

TEST(PageSuite, Struct)
{
    ASSERT_EQ(sizeof(struct _PageMeta), 20);
    ASSERT_EQ(sizeof(struct _PageStruct), PAGE_LEN);
}

TEST(HeaderSuite, Struct)
{
    ASSERT_EQ(sizeof(struct Header::_MetaUnit), 7);
    ASSERT_EQ(sizeof(struct Header::_MetaStatus), 1);
    ASSERT_EQ(sizeof(struct Header::_HeaderMeta), 232);
}

TEST(StorageMacroblockSuite, Struct)
{
    ASSERT_TRUE(StorageMacroblock::PAGES_COUNT == ((STORAGE_PAGE_PAYLOAD_SIZE * 8) / (sizeof(struct Header::_MetaUnit) * 8 + 2)) + StorageMacroblock::RESERVED_PAGES_COUNT);
}

TEST(StorageMacroblockSuite, CheckSectorAddresses)
{
    struct SectorAddressIndex {
        uint32_t address;
        uint32_t sectorAddress;
        uint32_t sectorIndex;
        uint32_t pageIndex;
        uint32_t pageAddress;
        bool isMacroblockAddress;
    };

    SectorAddressIndex addrIndex[] = {
        {
            /*.address             = */0,
            /*.sectorAddress       = */0,
            /*.sectorIndex         = */0,
            /*.pageIndex           = */0,
            /*.pageAddress         = */STORAGE_PAGE_SIZE * StorageMacroblock::RESERVED_PAGES_COUNT,
            /*.isMacroblockAddress = */true
        },
        {
            /*.address             = */1,
            /*.sectorAddress       = */0,
            /*.sectorIndex         = */0,
            /*.pageIndex           = */0,
            /*.pageAddress         = */STORAGE_PAGE_SIZE * StorageMacroblock::RESERVED_PAGES_COUNT,
            /*.isMacroblockAddress = */true
        },
        {
            /*.address             = */STORAGE_PAGE_SIZE - 1,
            /*.sectorAddress       = */0,
            /*.sectorIndex         = */0,
            /*.pageIndex           = */0,
            /*.pageAddress         = */STORAGE_PAGE_SIZE * StorageMacroblock::RESERVED_PAGES_COUNT,
            /*.isMacroblockAddress = */true
        },
        {
            /*.address             = */STORAGE_PAGE_SIZE,
            /*.sectorAddress       = */0,
            /*.sectorIndex         = */0,
            /*.pageIndex           = */0,
            /*.pageAddress         = */STORAGE_PAGE_SIZE * StorageMacroblock::RESERVED_PAGES_COUNT,
            /*.isMacroblockAddress = */true
        },
        {
            /*.address             = */STORAGE_PAGE_SIZE * StorageMacroblock::RESERVED_PAGES_COUNT,
            /*.sectorAddress       = */0,
            /*.sectorIndex         = */0,
            /*.pageIndex           = */0,
            /*.pageAddress         = */STORAGE_PAGE_SIZE * StorageMacroblock::RESERVED_PAGES_COUNT,
            /*.isMacroblockAddress = */false
        },
        {
            /*.address             = */STORAGE_PAGE_SIZE * StorageMacroblock::RESERVED_PAGES_COUNT + 1,
            /*.sectorAddress       = */0,
            /*.sectorIndex         = */0,
            /*.pageIndex           = */0,
            /*.pageAddress         = */STORAGE_PAGE_SIZE * StorageMacroblock::RESERVED_PAGES_COUNT,
            /*.isMacroblockAddress = */false
        },
        {
            /*.address             = */StorageMacroblock::getMacroblockAddress(1) - 1,
            /*.sectorAddress       = */0,
            /*.sectorIndex         = */0,
            /*.pageIndex           = */Header::PAGES_COUNT - 1,
            /*.pageAddress         = */STORAGE_PAGE_SIZE * StorageMacroblock::PAGES_COUNT - STORAGE_PAGE_SIZE,
            /*.isMacroblockAddress = */false
        },
        {
            /*.address             = */StorageMacroblock::getMacroblockAddress(1),
            /*.sectorAddress       = */StorageMacroblock::getMacroblockAddress(1),
            /*.sectorIndex         = */1,
            /*.pageIndex           = */0,
            /*.pageAddress         = */StorageMacroblock::getMacroblockAddress(1) + STORAGE_PAGE_SIZE * StorageMacroblock::RESERVED_PAGES_COUNT,
            /*.isMacroblockAddress = */true
        },
        {
            /*.address             = */StorageMacroblock::getMacroblockAddress(1),
            /*.sectorAddress       = */StorageMacroblock::getMacroblockAddress(1),
            /*.sectorIndex         = */1,
            /*.pageIndex           = */0,
            /*.pageAddress         = */StorageMacroblock::getMacroblockAddress(1) + STORAGE_PAGE_SIZE * StorageMacroblock::RESERVED_PAGES_COUNT,
            /*.isMacroblockAddress = */true
        },
        {
            /*.address             = */StorageMacroblock::getMacroblockAddress(1) + STORAGE_PAGE_SIZE / 2,
            /*.sectorAddress       = */StorageMacroblock::getMacroblockAddress(1),
            /*.sectorIndex         = */1,
            /*.pageIndex           = */0,
            /*.pageAddress         = */StorageMacroblock::getMacroblockAddress(1) + STORAGE_PAGE_SIZE * StorageMacroblock::RESERVED_PAGES_COUNT,
            /*.isMacroblockAddress = */true
        },
        {
            /*.address             = */StorageMacroblock::getMacroblockAddress(1) + STORAGE_PAGE_SIZE * StorageMacroblock::RESERVED_PAGES_COUNT - 1,
            /*.sectorAddress       = */StorageMacroblock::getMacroblockAddress(1),
            /*.sectorIndex         = */1,
            /*.pageIndex           = */0,
            /*.pageAddress         = */StorageMacroblock::getMacroblockAddress(1) + STORAGE_PAGE_SIZE * StorageMacroblock::RESERVED_PAGES_COUNT,
            /*.isMacroblockAddress = */true
        },
        {
            /*.address             = */StorageMacroblock::getMacroblockAddress(1) + STORAGE_PAGE_SIZE * (StorageMacroblock::RESERVED_PAGES_COUNT + 1),
            /*.sectorAddress       = */StorageMacroblock::getMacroblockAddress(1),
            /*.sectorIndex         = */1,
            /*.pageIndex           = */1,
            /*.pageAddress         = */StorageMacroblock::getMacroblockAddress(1) + STORAGE_PAGE_SIZE * (StorageMacroblock::RESERVED_PAGES_COUNT + 1),
            /*.isMacroblockAddress = */false
        },
    };

    for (unsigned i = 0; i < sizeof(addrIndex) / sizeof(*addrIndex); i++) {
        // std::cout << "addr=" << addrIndex[i].address << ",sectorIdx=" << addrIndex[i].sectorIndex << ",pageIdx=" << addrIndex[i].pageIndex << std::endl;
        ASSERT_EQ(StorageMacroblock::getMacroblockAddress(addrIndex[i].sectorIndex), addrIndex[i].sectorAddress);
        ASSERT_EQ(StorageMacroblock::getMacroblockIndex(addrIndex[i].sectorAddress), addrIndex[i].sectorIndex);
        ASSERT_EQ(StorageMacroblock::getMacroblockIndex(addrIndex[i].address), addrIndex[i].sectorIndex);
        ASSERT_EQ(StorageMacroblock::getPageAddressByIndex(addrIndex[i].sectorIndex, addrIndex[i].pageIndex), addrIndex[i].pageAddress);
        ASSERT_EQ(StorageMacroblock::getPageIndexByAddress(addrIndex[i].address), addrIndex[i].pageIndex);
        ASSERT_EQ(StorageMacroblock::isMacroblockAddress(addrIndex[i].address), addrIndex[i].isMacroblockAddress);
        ASSERT_EQ(Header::getMacroblockStartAddress(addrIndex[i].address), addrIndex[i].sectorAddress);
    }
}

TEST(StorageDriver, RequestExists)
{
    storage.clear();
    MockStorageDriver mockDriver;
    StorageAT sat(
        storage.getPagesCount(),
        &mockDriver,
        minMemoryEraseSize
    );
    uint32_t address = 0;

    EXPECT_CALL(mockDriver, read)
        .Times(::testing::AtLeast(1));

    EXPECT_CALL(mockDriver, write)
        .Times(::testing::AtLeast(1));

    ASSERT_EQ(sat.find(FIND_MODE_EMPTY, &address), STORAGE_OK);
}

TEST_F(StorageFixture, BadFindRequest)
{
    ASSERT_EQ(sat->find(static_cast<StorageFindMode>(0), &address, nullptr, 0), STORAGE_ERROR);
    ASSERT_EQ(sat->find(FIND_MODE_EQUAL, &address, nullptr, 0), STORAGE_ERROR);
    ASSERT_EQ(sat->find(FIND_MODE_MAX, &address, nullptr, 0), STORAGE_ERROR);
    ASSERT_EQ(sat->find(FIND_MODE_MIN, &address, nullptr, 0), STORAGE_ERROR);
    ASSERT_EQ(sat->find(FIND_MODE_NEXT, &address, nullptr, 0), STORAGE_ERROR);
    ASSERT_EQ(sat->find(FIND_MODE_EMPTY, nullptr), STORAGE_ERROR);
    ASSERT_EQ(sat->find(FIND_MODE_EQUAL, nullptr, nullptr, 0), STORAGE_ERROR);
    ASSERT_EQ(sat->find(FIND_MODE_MAX, nullptr, nullptr, 0), STORAGE_ERROR);
    ASSERT_EQ(sat->find(FIND_MODE_MIN, nullptr, nullptr, 0), STORAGE_ERROR);
    ASSERT_EQ(sat->find(FIND_MODE_NEXT, nullptr, nullptr, 0), STORAGE_ERROR);
    ASSERT_EQ(sat->find(FIND_MODE_EQUAL, nullptr, longPrefix, 0), STORAGE_ERROR);
    ASSERT_EQ(sat->find(FIND_MODE_MAX, nullptr, longPrefix, 0), STORAGE_ERROR);
    ASSERT_EQ(sat->find(FIND_MODE_MIN, nullptr, longPrefix, 0), STORAGE_ERROR);
    ASSERT_EQ(sat->find(FIND_MODE_NEXT, nullptr, longPrefix, 0), STORAGE_ERROR);
    ASSERT_EQ(sat->find(FIND_MODE_EQUAL, nullptr, longPrefix, 0), STORAGE_ERROR);
    ASSERT_EQ(sat->find(FIND_MODE_MAX, nullptr, shortPrefix, 0), STORAGE_ERROR);
    ASSERT_EQ(sat->find(FIND_MODE_MIN, nullptr, shortPrefix, 0), STORAGE_ERROR);
    ASSERT_EQ(sat->find(FIND_MODE_NEXT, nullptr, shortPrefix, 0), STORAGE_ERROR);
}

TEST_F(StorageFixture, BadSaveRequest)
{
    char emptyStr[] = "";

    ASSERT_EQ(sat->save(address, emptyStr, 0, nullptr, 0), STORAGE_ERROR);
    ASSERT_EQ(sat->save(address, emptyStr, 0, nullptr, STORAGE_PAGE_SIZE * 4), STORAGE_ERROR);
    ASSERT_EQ(sat->save(address, emptyStr, 0, nullptr, STORAGE_PAGE_SIZE * 4000), STORAGE_ERROR);
    ASSERT_EQ(sat->save(address, nullptr, 0, nullptr, 0), STORAGE_ERROR);
    ASSERT_EQ(sat->save(address, nullptr, 0, nullptr, STORAGE_PAGE_SIZE * 4), STORAGE_ERROR);
    ASSERT_EQ(sat->save(address, nullptr, 0, nullptr, STORAGE_PAGE_SIZE * 4000), STORAGE_ERROR);
}

TEST_F(StorageFixture, BadLoadRequest)
{
    ASSERT_EQ(sat->load(address, nullptr, 0), STORAGE_ERROR);
    ASSERT_EQ(sat->load(address, nullptr, STORAGE_PAGE_SIZE * 4), STORAGE_ERROR);
    ASSERT_EQ(sat->load(address, nullptr, STORAGE_PAGE_SIZE * 4000), STORAGE_ERROR);
}

TEST_F(StorageFixture, UseWrongPrefix)
{
    uint8_t wdata[100] = {};
    uint8_t rdata[100] = {};

    ASSERT_EQ(sat->find(FIND_MODE_EMPTY, &address), STORAGE_OK);
    ASSERT_EQ(sat->save(address, brokenPrefix, 1, wdata, sizeof(wdata)), STORAGE_OK);
    ASSERT_EQ(sat->find(FIND_MODE_EQUAL, &address, brokenPrefix, 1), STORAGE_OK);
    ASSERT_EQ(sat->load(address, rdata, sizeof(rdata)), STORAGE_OK);
    ASSERT_FALSE(memcmp(wdata, rdata, sizeof(wdata)));
}

TEST_F(StorageFixture, StorageBusy)
{
    storage.setBusy(true);

    ASSERT_EQ(sat->find(FIND_MODE_EQUAL, &address, "", 0), STORAGE_BUSY);
    ASSERT_EQ(sat->find(FIND_MODE_MAX, &address, "", 0), STORAGE_BUSY);
    ASSERT_EQ(sat->find(FIND_MODE_MIN, &address, "", 0), STORAGE_BUSY);
    ASSERT_EQ(sat->find(FIND_MODE_NEXT, &address, "", 0), STORAGE_BUSY);

    address = StorageMacroblock::RESERVED_PAGES_COUNT * STORAGE_PAGE_SIZE;
    ASSERT_EQ(sat->load(address, (new uint8_t[PAGE_LEN]), PAGE_LEN), STORAGE_BUSY);
    ASSERT_EQ(sat->save(address, shortPrefix, 1, (new uint8_t[PAGE_LEN]), PAGE_LEN), STORAGE_BUSY);
}


TEST_F(StorageFixture, FindEmptyAddress)
{
    ASSERT_EQ(sat->find(FIND_MODE_EMPTY, &address), STORAGE_OK);
}

TEST_F(StorageFixture, CheckEmptyPageLoad)
{
    Page page(0);

    ASSERT_EQ(page.load(), STORAGE_ERROR);
    ASSERT_EQ(page.load(/*startPage=*/true), STORAGE_ERROR);
}

TEST_F(StorageFixture, CheckSingleStartPageSave)
{
    uint8_t wdata[STORAGE_PAGE_PAYLOAD_SIZE] = { 1, 2, 3, 4, 5 };
    Page page(0);

    memcpy(page.page.payload, wdata, sizeof(page.page.payload));
    page.setPrevAddress(0);
    page.setNextAddress(STORAGE_PAGE_SIZE);

    ASSERT_EQ(page.save(), STORAGE_OK);
    ASSERT_EQ(page.load(/*startPage=*/true), STORAGE_OK);
    ASSERT_EQ(page.load(), STORAGE_OK);
    ASSERT_FALSE(memcmp(page.page.payload, wdata, sizeof(page.page.payload)));
    ASSERT_TRUE(page.isStart());
    ASSERT_FALSE(page.isMiddle());
    ASSERT_FALSE(page.isEnd());
    ASSERT_FALSE(page.validatePrevAddress());
    ASSERT_TRUE(page.validateNextAddress());
    ASSERT_EQ(page.loadPrev(), STORAGE_NOT_FOUND);
    ASSERT_EQ(page.loadNext(), STORAGE_NOT_FOUND);
    Page tmpPage(0);
    ASSERT_EQ(tmpPage.load(), STORAGE_OK);
    ASSERT_EQ(tmpPage.loadNext(), STORAGE_NOT_FOUND);
}

TEST_F(StorageFixture, CheckSingleMiddlePageSave)
{
    uint8_t wdata[STORAGE_PAGE_PAYLOAD_SIZE] = { 1, 2, 3, 4, 5 };
    Page page(STORAGE_PAGE_SIZE);

    memcpy(page.page.payload, wdata, sizeof(page.page.payload));
    page.setPrevAddress(0);
    page.setNextAddress(STORAGE_PAGE_SIZE * 2);

    ASSERT_EQ(page.save(), STORAGE_OK);
    ASSERT_EQ(page.load(), STORAGE_OK);
    ASSERT_EQ(page.load(/*startPage=*/true), STORAGE_ERROR);
    ASSERT_FALSE(memcmp(page.page.payload, wdata, sizeof(page.page.payload)));
    ASSERT_FALSE(page.isStart());
    ASSERT_TRUE(page.isMiddle());
    ASSERT_FALSE(page.isEnd());
    ASSERT_TRUE(page.validatePrevAddress());
    ASSERT_TRUE(page.validateNextAddress());
    ASSERT_EQ(page.loadPrev(), STORAGE_ERROR);
    Page tmpPage(STORAGE_PAGE_SIZE);
    ASSERT_EQ(tmpPage.load(), STORAGE_OK);
    ASSERT_EQ(tmpPage.loadNext(), STORAGE_NOT_FOUND);
}

TEST_F(StorageFixture, CheckSingleEndPageSave)
{
    uint8_t wdata[STORAGE_PAGE_PAYLOAD_SIZE] = { 1, 2, 3, 4, 5 };
    Page page(STORAGE_PAGE_SIZE * 2);

    memcpy(page.page.payload, wdata, sizeof(page.page.payload));
    page.page.header.prev_addr = 0;
    page.page.header.next_addr = STORAGE_PAGE_SIZE * 2;

    ASSERT_EQ(page.save(), STORAGE_OK);
    ASSERT_EQ(page.load(), STORAGE_OK);
    ASSERT_EQ(page.load(/*startPage=*/true), STORAGE_ERROR);
    ASSERT_FALSE(memcmp(page.page.payload, wdata, sizeof(page.page.payload)));
    ASSERT_FALSE(page.isStart());
    ASSERT_FALSE(page.isMiddle());
    ASSERT_TRUE(page.isEnd());
    ASSERT_TRUE(page.validatePrevAddress());
    ASSERT_FALSE(page.validateNextAddress());
    ASSERT_EQ(page.loadPrev(), STORAGE_ERROR);
    Page tmpPage(STORAGE_PAGE_SIZE * 2);
    ASSERT_EQ(tmpPage.load(), STORAGE_OK);
    ASSERT_EQ(tmpPage.loadNext(), STORAGE_NOT_FOUND);
}

TEST_F(StorageFixture, CheckBrokenMagicLoad)
{
    uint8_t wdata[STORAGE_PAGE_PAYLOAD_SIZE] = { 1, 2, 3, 4, 5 };
    Page page(STORAGE_PAGE_SIZE * 2);

    memcpy(page.page.payload, wdata, sizeof(page.page.payload));
    page.page.header.prev_addr = 0;
    page.page.header.next_addr = STORAGE_PAGE_SIZE * 2;
    ASSERT_EQ(page.save(), STORAGE_OK);
    page.page.header.magic = 0;
    storage.writePage(page.getAddress(), reinterpret_cast<uint8_t*>(&page.page), sizeof(page.page));

    ASSERT_EQ(page.load(), STORAGE_ERROR);
    ASSERT_EQ(page.load(/*startPage=*/true), STORAGE_ERROR);
}

TEST_F(StorageFixture, CheckBrokenVersionLoad)
{
    uint8_t wdata[STORAGE_PAGE_PAYLOAD_SIZE] = { 1, 2, 3, 4, 5 };
    Page page(STORAGE_PAGE_SIZE * 2);

    memcpy(page.page.payload, wdata, sizeof(page.page.payload));
    page.page.header.prev_addr = 0;
    page.page.header.next_addr = STORAGE_PAGE_SIZE * 2;
    ASSERT_EQ(page.save(), STORAGE_OK);
    page.page.header.version = STORAGE_VERSION - 1;
    storage.writePage(page.getAddress(), reinterpret_cast<uint8_t*>(&page.page), sizeof(page.page));

    ASSERT_EQ(page.load(), STORAGE_ERROR);
    ASSERT_EQ(page.load(/*startPage=*/true), STORAGE_ERROR);
}

TEST_F(StorageFixture, CheckDataStartPageLoad)
{
    uint8_t wdata[STORAGE_PAGE_PAYLOAD_SIZE * 3] = { 1, 2, 3, 4, 5 };

    ASSERT_EQ(sat->find(FIND_MODE_EMPTY, &address), STORAGE_OK);
    ASSERT_EQ(sat->save(address, shortPrefix, 1, wdata, sizeof(wdata)), STORAGE_OK);
    Page page(address);

    ASSERT_EQ(page.load(/*startPage=*/true), STORAGE_OK);
    ASSERT_EQ(page.load(), STORAGE_OK);
    ASSERT_FALSE(memcmp(page.page.payload, wdata, sizeof(page.page.payload)));
    ASSERT_TRUE(page.isStart());
    ASSERT_FALSE(page.isMiddle());
    ASSERT_FALSE(page.isEnd());
    ASSERT_FALSE(page.validatePrevAddress());
    ASSERT_TRUE(page.validateNextAddress());
    ASSERT_EQ(page.loadPrev(), STORAGE_NOT_FOUND);
    Page tmpPage(address);
    ASSERT_EQ(tmpPage.load(), STORAGE_OK);
    ASSERT_EQ(tmpPage.loadNext(), STORAGE_OK);
}

TEST_F(StorageFixture, CheckDataMiddlePageLoad)
{
    uint8_t wdata[STORAGE_PAGE_PAYLOAD_SIZE * 3] = { 1, 2, 3, 4, 5 };

    ASSERT_EQ(sat->find(FIND_MODE_EMPTY, &address), STORAGE_OK);
    ASSERT_EQ(sat->save(address, shortPrefix, 1, wdata, sizeof(wdata)), STORAGE_OK);
    Page page(address + STORAGE_PAGE_SIZE);

    ASSERT_EQ(page.load(/*startPage=*/true), STORAGE_ERROR);
    ASSERT_EQ(page.load(), STORAGE_OK);
    ASSERT_FALSE(memcmp(page.page.payload, wdata + STORAGE_PAGE_PAYLOAD_SIZE, sizeof(page.page.payload)));
    ASSERT_FALSE(page.isStart());
    ASSERT_TRUE(page.isMiddle());
    ASSERT_FALSE(page.isEnd());
    ASSERT_TRUE(page.validatePrevAddress());
    ASSERT_TRUE(page.validateNextAddress());
    ASSERT_EQ(page.loadPrev(), STORAGE_OK);
    Page tmpPage(address + STORAGE_PAGE_SIZE);
    ASSERT_EQ(tmpPage.load(), STORAGE_OK);
    ASSERT_EQ(tmpPage.loadNext(), STORAGE_OK);
}

TEST_F(StorageFixture, CheckDataEndPageLoad)
{
    uint8_t wdata[STORAGE_PAGE_PAYLOAD_SIZE * 3] = { 1, 2, 3, 4, 5 };

    ASSERT_EQ(sat->find(FIND_MODE_EMPTY, &address), STORAGE_OK);
    ASSERT_EQ(sat->save(address, shortPrefix, 1, wdata, sizeof(wdata)), STORAGE_OK);
    Page page(address + STORAGE_PAGE_SIZE * 2);

    ASSERT_EQ(page.load(/*startPage=*/true), STORAGE_ERROR);
    ASSERT_EQ(page.load(), STORAGE_OK);
    ASSERT_FALSE(memcmp(page.page.payload, wdata + STORAGE_PAGE_PAYLOAD_SIZE * 2, sizeof(page.page.payload)));
    ASSERT_FALSE(page.isStart());
    ASSERT_FALSE(page.isMiddle());
    ASSERT_TRUE(page.isEnd());
    ASSERT_TRUE(page.validatePrevAddress());
    ASSERT_FALSE(page.validateNextAddress());
    ASSERT_EQ(page.loadPrev(), STORAGE_OK);
    Page tmpPage(address + STORAGE_PAGE_SIZE * 2);
    ASSERT_EQ(tmpPage.load(), STORAGE_OK);
    ASSERT_EQ(tmpPage.loadNext(), STORAGE_NOT_FOUND);
}

TEST_F(StorageFixture, CheckDataBrokenPayloadStartPageLoad)
{
    uint8_t wdata[STORAGE_PAGE_PAYLOAD_SIZE * 3] = { 1, 2, 3, 4, 5 };
    uint8_t rdata[STORAGE_PAGE_PAYLOAD_SIZE * 3] = { 0 };

    ASSERT_EQ(sat->find(FIND_MODE_EMPTY, &address), STORAGE_OK);
    ASSERT_EQ(sat->save(address, shortPrefix, 1, wdata, sizeof(wdata)), STORAGE_OK);
    Page page(address);
    ASSERT_EQ(page.load(/*startPage=*/true), STORAGE_OK);
    page.page.header.magic = 0;
    storage.writePage(page.getAddress(), reinterpret_cast<uint8_t*>(&page.page), sizeof(page.page));

    ASSERT_EQ(sat->find(FIND_MODE_EQUAL, &address, shortPrefix, 1), STORAGE_NOT_FOUND);
    ASSERT_EQ(sat->load(address, rdata, sizeof(rdata)), STORAGE_ERROR);
}

TEST_F(StorageFixture, CheckDataBrokenPayloadSinglePageLoad)
{
    uint8_t wdata[STORAGE_PAGE_PAYLOAD_SIZE * 3] = { 1, 2, 3, 4, 5 };
    uint8_t rdata[STORAGE_PAGE_PAYLOAD_SIZE * 3] = { 0 };

    ASSERT_EQ(sat->find(FIND_MODE_EMPTY, &address), STORAGE_OK);
    ASSERT_EQ(sat->save(address, shortPrefix, 1, wdata, sizeof(wdata)), STORAGE_OK);
    Page page(address + STORAGE_PAGE_SIZE);
    ASSERT_EQ(page.load(), STORAGE_OK);
    page.page.header.magic = 0;
    storage.writePage(page.getAddress(), reinterpret_cast<uint8_t*>(&page.page), sizeof(page.page));

    ASSERT_EQ(sat->find(FIND_MODE_EQUAL, &address, shortPrefix, 1), STORAGE_OK);
    ASSERT_EQ(sat->load(address, rdata, sizeof(rdata)), STORAGE_NOT_FOUND);
}

TEST_F(StorageFixture, FindPageAfterFormat)
{
    uint8_t wdata[STORAGE_PAGE_PAYLOAD_SIZE * 3] = { 1, 2, 3, 4, 5 };

    ASSERT_EQ(sat->find(FIND_MODE_EMPTY, &address), STORAGE_OK);
    ASSERT_EQ(sat->save(address, shortPrefix, 1, wdata, sizeof(wdata)), STORAGE_OK);
    ASSERT_EQ(sat->format(), STORAGE_OK);
    ASSERT_EQ(sat->find(FIND_MODE_EQUAL, &address, shortPrefix, 1), STORAGE_NOT_FOUND);
}

TEST_F(StorageFixture, IsSetStatusesInHeader)
{
    Header header(0);

    ASSERT_EQ(sat->format(), STORAGE_OK);
    ASSERT_EQ(header.load(), STORAGE_OK);

    for (unsigned i = 0; i < Header::PAGES_COUNT; i++) {
        uint32_t targetAddress = StorageMacroblock::getPageAddressByIndex(0, i);
        ASSERT_TRUE(header.isPageStatus(i, Header::PAGE_EMPTY));
        ASSERT_TRUE(header.isAddressEmpty(targetAddress));
    }
}

TEST_F(StorageFixture, SetStatusesInHeader)
{
    Header header(0);

    ASSERT_EQ(sat->format(), STORAGE_OK);
    ASSERT_EQ(header.load(), STORAGE_OK);

    for (unsigned i = 0; i < Header::PAGES_COUNT; i++) {
        header.setPageStatus(i, Header::PAGE_OK);
        ASSERT_TRUE(header.isPageStatus(i, Header::PAGE_OK));
    }
}

TEST_F(StorageFixture, SetBlockStatusesInHeader)
{
    Header header(0);

    ASSERT_EQ(sat->format(), STORAGE_OK);
    ASSERT_EQ(header.load(), STORAGE_OK);
    
    for (unsigned i = 0; i < Header::PAGES_COUNT; i++) {
        uint32_t targetAddress = StorageMacroblock::getPageAddressByIndex(0, i);
        header.setAddressBlocked(targetAddress);
        ASSERT_TRUE(header.isPageStatus(i, Header::PAGE_BLOCKED));
    }
}

TEST_F(StorageFixture, AutomaticHeadersCreation)
{
    uint8_t wdata[STORAGE_PAGE_PAYLOAD_SIZE] = { 1, 2, 3, 4, 5 };
    uint32_t pageAddress = StorageMacroblock::RESERVED_PAGES_COUNT * STORAGE_PAGE_SIZE;
    Page page(pageAddress);

    memcpy(reinterpret_cast<void*>(&page.page), wdata, STORAGE_PAGE_PAYLOAD_SIZE);
    memcpy(page.page.header.prefix, shortPrefix, sizeof(page.page.header.prefix));
    page.page.header.id = 1;
    page.page.header.prev_addr = pageAddress;
    page.page.header.next_addr = pageAddress;
    page.save();

    ASSERT_EQ(sat->find(FIND_MODE_EQUAL, &address, shortPrefix, 1), STORAGE_OK);
    ASSERT_EQ(address, StorageMacroblock::RESERVED_PAGES_COUNT * STORAGE_PAGE_SIZE);
}

TEST_F(StorageFixture, HeaderSameMeta)
{
    uint8_t wdata[STORAGE_PAGE_PAYLOAD_SIZE] = { 1, 2, 3, 4, 5 };
    uint32_t pageAddress = StorageMacroblock::RESERVED_PAGES_COUNT * STORAGE_PAGE_SIZE;
    Header header(0);
    Page page(pageAddress);

    memcpy(reinterpret_cast<void*>(&page.page), wdata, STORAGE_PAGE_PAYLOAD_SIZE);
    memcpy(page.page.header.prefix, shortPrefix, sizeof(page.page.header.prefix));
    page.page.header.id = 1;
    page.page.header.prev_addr = pageAddress;
    page.page.header.next_addr = pageAddress;
    page.save();

    ASSERT_EQ(sat->find(FIND_MODE_EQUAL, &address, shortPrefix, 1), STORAGE_OK);
    ASSERT_EQ(header.load(), STORAGE_OK);
    ASSERT_TRUE(header.isSameMeta(StorageMacroblock::getPageIndexByAddress(page.getAddress()), reinterpret_cast<const uint8_t*>(shortPrefix), 1));
}

TEST_F(StorageFixture, RewritePageWithSameData)
{
    uint8_t wdata1[STORAGE_PAGE_PAYLOAD_SIZE] = { 1, 2, 3, 4, 5 };
    uint8_t wdata2[STORAGE_PAGE_PAYLOAD_SIZE] = { 6, 7, 8, 9, 10 };
    uint8_t rdata1[STORAGE_PAGE_PAYLOAD_SIZE] = { 0 };
    uint8_t rdata2[STORAGE_PAGE_PAYLOAD_SIZE] = { 0 };

    ASSERT_EQ(sat->find(FIND_MODE_EMPTY, &address), STORAGE_OK);
    ASSERT_EQ(sat->save(address, shortPrefix, 1, wdata1, sizeof(wdata1)), STORAGE_OK);
    ASSERT_EQ(sat->load(address, rdata1, sizeof(rdata1)), STORAGE_OK);
    ASSERT_FALSE(memcmp(wdata1, rdata1, sizeof(wdata1)));
    ASSERT_EQ(sat->save(address, shortPrefix, 1, wdata2, sizeof(wdata2)), STORAGE_OK);
    ASSERT_EQ(sat->load(address, rdata2, sizeof(rdata2)), STORAGE_OK);
    ASSERT_FALSE(memcmp(wdata2, rdata2, sizeof(wdata2)));
}

TEST_F(StorageFixture, RewriteSaveDataByAnotherData)
{
    uint8_t wdata1[STORAGE_PAGE_PAYLOAD_SIZE] = { 1, 2, 3, 4, 5 };
    uint8_t wdata2[STORAGE_PAGE_PAYLOAD_SIZE] = { 6, 7, 8, 9, 10 };
    uint8_t rdata1[STORAGE_PAGE_PAYLOAD_SIZE] = { 0 };
    uint8_t rdata2[STORAGE_PAGE_PAYLOAD_SIZE] = { 0 };

    ASSERT_EQ(sat->find(FIND_MODE_EMPTY, &address), STORAGE_OK);
    ASSERT_EQ(sat->save(address, shortPrefix, 1, wdata1, sizeof(wdata1)), STORAGE_OK);
    ASSERT_EQ(sat->load(address, rdata1, sizeof(rdata1)), STORAGE_OK);
    ASSERT_FALSE(memcmp(wdata1, rdata1, sizeof(wdata1)));
    ASSERT_EQ(sat->rewrite(address, shortPrefix, 2, wdata2, sizeof(wdata2)), STORAGE_OK);
    ASSERT_EQ(sat->load(address, rdata2, sizeof(rdata2)), STORAGE_OK);
    ASSERT_FALSE(memcmp(wdata2, rdata2, sizeof(wdata2)));
}

TEST_F(StorageFixture, WriteAllStorageBytes)
{
    uint32_t storageSize = StorageAT::getStorageSize();

    ASSERT_EQ(sat->find(FIND_MODE_EMPTY, &address), STORAGE_OK);
    ASSERT_EQ(sat->save(address, shortPrefix, 0, (new uint8_t[storageSize]), storageSize), STORAGE_OOM);
}

TEST_F(StorageFixture, FindAllData)
{
    uint8_t wdata[STORAGE_PAGE_PAYLOAD_SIZE] = { 1, 2, 3, 4, 5 };
    uint8_t rdata[STORAGE_PAGE_PAYLOAD_SIZE] = { 0 };

    StorageStatus status = STORAGE_OK;
    uint32_t pagesCount = 0;
    while (status == STORAGE_OK) {
        status = sat->find(FIND_MODE_EMPTY, &address);
        if (status != STORAGE_OK) {
            break;
        }
        status = sat->save(address, shortPrefix, pagesCount + 1, wdata, sizeof(wdata));
        ASSERT_EQ(status, STORAGE_OK);
        memset(rdata, 0, sizeof(rdata));
        status = sat->load(address, rdata, sizeof(rdata));
        ASSERT_EQ(status, STORAGE_OK);
        ASSERT_FALSE(memcmp(wdata, rdata, sizeof(wdata)));

        pagesCount++;
    }

    ASSERT_EQ(pagesCount, StorageAT::getPayloadPagesCount());
    for (unsigned i = 0; i < pagesCount; i++) {
        ASSERT_EQ(sat->find(FIND_MODE_EQUAL, &address, shortPrefix, i + 1), STORAGE_OK);
    }
    ASSERT_EQ(sat->find(FIND_MODE_MIN, &address, shortPrefix), STORAGE_OK);
    ASSERT_EQ(address, STORAGE_PAGE_SIZE * StorageMacroblock::RESERVED_PAGES_COUNT);
    ASSERT_EQ(sat->find(FIND_MODE_MAX, &address, shortPrefix), STORAGE_OK);
    ASSERT_EQ(address, STORAGE_PAGE_SIZE * (StorageAT::getStoragePagesCount() - 1));
    ASSERT_EQ(sat->find(FIND_MODE_NEXT, &address, shortPrefix, 1), STORAGE_OK);
    ASSERT_EQ(address, STORAGE_PAGE_SIZE * (StorageMacroblock::RESERVED_PAGES_COUNT + 1));
}

TEST_F(StorageFixture, SaveDataWithMaxSize) {
    uint32_t maxSize = StorageAT::getPayloadSize();
    uint8_t* wdata = new uint8_t[maxSize];
    uint8_t* rdata = new uint8_t[maxSize];
    memset(wdata, 0xAA, maxSize); // Заполняем данные

    ASSERT_EQ(sat->find(FIND_MODE_EMPTY, &address), STORAGE_OK);
    ASSERT_EQ(sat->save(address, shortPrefix, 1, wdata, maxSize), STORAGE_OK);
    ASSERT_EQ(sat->load(address, rdata, maxSize), STORAGE_OK);
    ASSERT_FALSE(memcmp(wdata, rdata, maxSize));

    delete[] wdata;
    delete[] rdata;
}

TEST_F(StorageFixture, DeleteData)
{
    uint8_t wdata[STORAGE_PAGE_PAYLOAD_SIZE] = { 1, 2, 3, 4, 5 };

    ASSERT_EQ(sat->find(FIND_MODE_EMPTY, &address), STORAGE_OK);
    ASSERT_EQ(sat->save(address, shortPrefix, 0, wdata, sizeof(wdata)), STORAGE_OK);
    ASSERT_EQ(sat->find(FIND_MODE_EQUAL, &address, shortPrefix, 0), STORAGE_OK);
    ASSERT_EQ(sat->clearAddress(address), STORAGE_OK);
    ASSERT_EQ(sat->find(FIND_MODE_EQUAL, &address, shortPrefix, 0), STORAGE_NOT_FOUND);
}

TEST_F(StorageFixture, LoadDataWithInvalidAddress) {
    uint8_t rdata[STORAGE_PAGE_PAYLOAD_SIZE] = { 0 };

    // Некорректный адрес (не кратный размеру страницы)
    address = 1;
    ASSERT_EQ(sat->load(address, rdata, sizeof(rdata)), STORAGE_ERROR);

    // Адрес за пределами памяти
    address = StorageAT::getStorageSize() + STORAGE_PAGE_SIZE;
    ASSERT_EQ(sat->load(address, rdata, sizeof(rdata)), STORAGE_OOM);
}

TEST_F(StorageFixture, SaveDataWithInvalidAddress) {
    uint8_t wdata[STORAGE_PAGE_PAYLOAD_SIZE] = { 1, 2, 3, 4, 5 };

    // Некорректный адрес (не кратный размеру страницы)
    address = 1;
    ASSERT_EQ(sat->save(address, shortPrefix, 1, wdata, sizeof(wdata)), STORAGE_ERROR);

    // Адрес за пределами памяти
    address = StorageAT::getStorageSize() + STORAGE_PAGE_SIZE;
    ASSERT_EQ(sat->save(address, shortPrefix, 1, wdata, sizeof(wdata)), STORAGE_OOM);
}

TEST_F(StorageFixture, LoadEmptyPage)
{
    uint8_t data[STORAGE_PAGE_PAYLOAD_SIZE] = {};

    ASSERT_EQ(sat->load(0, data, 0), STORAGE_ERROR);
    ASSERT_EQ(sat->load(0, data, sizeof(data)), STORAGE_ERROR);
}

TEST_F(StorageFixture, SaveSimplePageInSectorHeader)
{
    uint8_t data[STORAGE_PAGE_PAYLOAD_SIZE] = {};

    ASSERT_EQ(sat->save(0, shortPrefix, 1, data, sizeof(data)), STORAGE_ERROR);
    ASSERT_EQ(sat->load(0, data, sizeof(data)), STORAGE_ERROR);
}

TEST_F(StorageFixture, SavePageWithFindEmptyAddress)
{
    uint8_t wdata[STORAGE_PAGE_PAYLOAD_SIZE] = { 1, 2, 3, 4, 5 };
    uint8_t rdata[STORAGE_PAGE_PAYLOAD_SIZE] = {};

    ASSERT_EQ(sat->find(FIND_MODE_EMPTY, &address), STORAGE_OK);
    ASSERT_EQ(address, StorageMacroblock::RESERVED_PAGES_COUNT * STORAGE_PAGE_SIZE);
    ASSERT_EQ(sat->save(address, shortPrefix, 1, wdata, sizeof(wdata)), STORAGE_OK);
    ASSERT_EQ(sat->load(address, rdata, sizeof(rdata)), STORAGE_OK);
    ASSERT_FALSE(memcmp(wdata, rdata, sizeof(wdata)));
}


TEST_F(StorageFixture, SavePageWithFindEmptyAddressWithOverwrite)
{
    uint8_t wdata[STORAGE_PAGE_PAYLOAD_SIZE] = { 1, 2, 3, 4, 5 };

    ASSERT_EQ(sat->find(FIND_MODE_EMPTY, &address), STORAGE_OK);
    ASSERT_EQ(sat->save(address, shortPrefix, 1, wdata, sizeof(wdata)), STORAGE_OK);
    ASSERT_EQ(sat->save(address, shortPrefix, 1, wdata, sizeof(wdata)), STORAGE_OK);
    ASSERT_EQ(sat->save(address, shortPrefix, 2, wdata, sizeof(wdata)), STORAGE_DATA_EXISTS);
}

TEST_F(StorageFixture, SaveMultiPageInHeader)
{
    uint8_t data[STORAGE_PAGE_SIZE * 4] = {};

    ASSERT_EQ(sat->save(0, shortPrefix, 1, data, sizeof(data)), STORAGE_ERROR);
    ASSERT_EQ(sat->load(0, data, sizeof(data)), STORAGE_ERROR);
}

TEST_F(StorageFixture, SaveMultiPageWithFindEmptyAddress)
{
    uint8_t wdata[STORAGE_PAGE_SIZE * 4] = { 1, 2, 3, 4, 5 };
    uint8_t rdata[STORAGE_PAGE_SIZE * 4] = {};

    ASSERT_EQ(sat->find(FIND_MODE_EMPTY, &address), STORAGE_OK);
    ASSERT_EQ(address, StorageMacroblock::RESERVED_PAGES_COUNT * STORAGE_PAGE_SIZE);
    ASSERT_EQ(sat->save(address, shortPrefix, 1, wdata, sizeof(wdata)), STORAGE_OK);
    ASSERT_EQ(sat->load(address, rdata, sizeof(rdata)), STORAGE_OK);
    ASSERT_FALSE(memcmp(wdata, rdata, sizeof(wdata)));
}

TEST_F(StorageFixture, SaveMultiPageWithFindEmptyAddressWithOverwrite)
{
    uint8_t wdata[STORAGE_PAGE_SIZE * 4] = { 1, 2, 3, 4, 5 };

    ASSERT_EQ(sat->find(FIND_MODE_EMPTY, &address), STORAGE_OK);
    ASSERT_EQ(sat->save(address, shortPrefix, 1, wdata, sizeof(wdata)), STORAGE_OK);
    ASSERT_EQ(sat->save(address, shortPrefix, 1, wdata, sizeof(wdata)), STORAGE_OK);
    ASSERT_EQ(sat->save(address, shortPrefix, 2, wdata, sizeof(wdata)), STORAGE_DATA_EXISTS);
}

TEST_F(StorageFixture, SaveDataInBusyPages)
{
    uint8_t wdata1[STORAGE_PAGE_SIZE * 4] = { 1, 2, 3, 4, 5 };
    uint8_t wdata2[STORAGE_PAGE_PAYLOAD_SIZE] = { 6, 7, 8, 9, 10 };
    uint8_t rdata1[STORAGE_PAGE_SIZE * 4] = {};

    ASSERT_EQ(sat->find(FIND_MODE_EMPTY, &address), STORAGE_OK);
    ASSERT_EQ(sat->save(address, shortPrefix, 1, wdata1, sizeof(wdata1)), STORAGE_OK);
    ASSERT_EQ(sat->save(address, shortPrefix, 2, wdata2, sizeof(wdata2)), STORAGE_DATA_EXISTS);
    ASSERT_EQ(sat->save(address + PAGE_LEN, shortPrefix, 2, wdata2, sizeof(wdata2)), STORAGE_DATA_EXISTS);
    ASSERT_EQ(sat->load(address, rdata1, sizeof(rdata1)), STORAGE_OK);
    ASSERT_FALSE(memcmp(wdata1, rdata1, sizeof(wdata1)));
}

TEST_F(StorageFixture, LoadNotAlignedAddress)
{
    uint8_t rdata[STORAGE_PAGE_PAYLOAD_SIZE] = {};

    ASSERT_EQ(sat->find(FIND_MODE_EMPTY, &address), STORAGE_OK);
    ASSERT_EQ(sat->load(address + 1, rdata, sizeof(rdata)), STORAGE_ERROR);
}

TEST_F(StorageFixture, SaveNotAlignedAddress)
{
    uint8_t wdata[STORAGE_PAGE_PAYLOAD_SIZE] = {};

    ASSERT_EQ(sat->find(FIND_MODE_EMPTY, &address), STORAGE_OK);
    ASSERT_EQ(sat->save(address + 1, shortPrefix, 1, wdata, sizeof(wdata)), STORAGE_ERROR);
}

TEST_F(StorageFixture, SaveDataOnBlockedPage)
{
    address = StorageMacroblock::RESERVED_PAGES_COUNT * STORAGE_PAGE_SIZE;
    uint8_t wdata[STORAGE_PAGE_PAYLOAD_SIZE] = { 1, 2, 3, 4, 5 };
    Header header(address);
    uint32_t tmpAddress = 0;

    storage.setBlocked(address, true);

    ASSERT_EQ(sat->save(address, shortPrefix, 1, wdata, sizeof(wdata)), STORAGE_OK);
    ASSERT_EQ(StorageMacroblock::loadHeader(&header), STORAGE_OK);

    uint32_t pageIndex = StorageMacroblock::getPageIndexByAddress(address);
    ASSERT_TRUE(header.isPageStatus(pageIndex, Header::PAGE_BLOCKED));
    ASSERT_EQ(sat->find(FIND_MODE_EQUAL, &tmpAddress, shortPrefix, 1), STORAGE_OK);
    ASSERT_NE(address, tmpAddress);
    ASSERT_EQ(header.load(), STORAGE_OK);
    ASSERT_TRUE(header.isPageStatus(pageIndex, Header::PAGE_BLOCKED));
}

TEST_F(StorageFixture, SaveDataOnBlockedSector)
{
    address = StorageMacroblock::RESERVED_PAGES_COUNT * STORAGE_PAGE_SIZE;
    uint8_t wdata[STORAGE_PAGE_PAYLOAD_SIZE] = { 1, 2, 3, 4, 5 };
    Header header(address);
    uint32_t tmpAddress = 0;

    for (unsigned i = 0; i < Header::PAGES_COUNT * STORAGE_PAGE_SIZE; i++) {
        storage.setBlocked(address + i, true);
    }

    ASSERT_EQ(sat->save(address, shortPrefix, 1, wdata, sizeof(wdata)), STORAGE_OK);
    ASSERT_EQ(StorageMacroblock::loadHeader(&header), STORAGE_OK);

    for (unsigned i = 0; i < Header::PAGES_COUNT; i++) {
        ASSERT_TRUE(header.isPageStatus(i, Header::PAGE_BLOCKED));
    }
    ASSERT_EQ(sat->find(FIND_MODE_EQUAL, &tmpAddress, shortPrefix, 1), STORAGE_OK);
    ASSERT_NE(address, tmpAddress);
    ASSERT_EQ(header.load(), STORAGE_OK);
    ASSERT_TRUE(header.isPageStatus(StorageMacroblock::getPageIndexByAddress(address), Header::PAGE_BLOCKED));
}

TEST_F(StorageFixture, BlockAllMemory)
{
    uint8_t wdata[STORAGE_PAGE_PAYLOAD_SIZE] = { 1, 2, 3, 4, 5 };
    address = StorageMacroblock::RESERVED_PAGES_COUNT * STORAGE_PAGE_SIZE;
    
    for (unsigned i = 0; i < storage.getSize(); i++) {
        storage.setBlocked(i, true);
    }

    ASSERT_EQ(sat->save(address, shortPrefix, 1, wdata, sizeof(wdata)), STORAGE_OOM);
    ASSERT_EQ(sat->find(FIND_MODE_EQUAL, &address, shortPrefix, 1), STORAGE_NOT_FOUND);
}

TEST_F(StorageFixture, SetHeaderBlocked)
{
    address = 0;
    Header header(address);

    storage.setBlocked(address, true);

    ASSERT_EQ(StorageMacroblock::loadHeader(&header), STORAGE_OK);
    ASSERT_NE(header.getAddress(), address);
}

TEST_F(StorageFixture, SetAllHeadersBlocked)
{
    Header header(0);

    for (unsigned i = 0; i < StorageMacroblock::RESERVED_PAGES_COUNT; i++) {
        storage.setBlocked(address + STORAGE_PAGE_SIZE * i, true);
    }

    ASSERT_EQ(StorageMacroblock::loadHeader(&header), STORAGE_OK);
}

TEST_F(StorageFixture, SaveAndFindDataWithBlockedAllHeaders)
{
    uint8_t wdata[STORAGE_PAGE_PAYLOAD_SIZE] = { 1, 2, 3, 4, 5 };
    uint8_t rdata[STORAGE_PAGE_PAYLOAD_SIZE] = { 0 };

    for (unsigned i = 0; i < StorageMacroblock::RESERVED_PAGES_COUNT * STORAGE_PAGE_SIZE; i++) {
        storage.setBlocked(i, true);
    }

    ASSERT_EQ(sat->find(FIND_MODE_EMPTY, &address), STORAGE_OK);
    ASSERT_EQ(address, StorageMacroblock::RESERVED_PAGES_COUNT * STORAGE_PAGE_SIZE);
    ASSERT_EQ(sat->save(address, shortPrefix, 1, wdata, sizeof(wdata)), STORAGE_OK);
    ASSERT_EQ(sat->find(FIND_MODE_EQUAL, &address, shortPrefix, 1), STORAGE_OK);
    ASSERT_EQ(sat->load(address, rdata, sizeof(rdata)), STORAGE_OK);
    ASSERT_FALSE(memcmp(wdata, rdata, sizeof(wdata)));
}

TEST_F(StorageFixture, FindEqual)
{
    uint8_t wdata[STORAGE_PAGE_PAYLOAD_SIZE] = { 1, 2, 3, 4, 5 };
    uint8_t rdata[STORAGE_PAGE_PAYLOAD_SIZE] = {};
    uint32_t emptyAddress = 0;

    ASSERT_EQ(sat->find(FIND_MODE_EMPTY, &emptyAddress), STORAGE_OK);
    ASSERT_EQ(sat->save(emptyAddress, shortPrefix, 1, wdata, sizeof(wdata)), STORAGE_OK);

    ASSERT_EQ(sat->find(FIND_MODE_EQUAL, &address, shortPrefix, 1), STORAGE_OK);
    ASSERT_EQ(address, emptyAddress);
    ASSERT_EQ(sat->load(address, rdata, sizeof(rdata)), STORAGE_OK);
    ASSERT_FALSE(memcmp(wdata, rdata, sizeof(wdata)));
}

TEST_F(StorageFixture, SaveFindLoadPartitionedData)
{
    uint32_t nextAddress = 0;
    uint8_t wdata1[STORAGE_PAGE_PAYLOAD_SIZE] = {};
    uint8_t wdata2[STORAGE_PAGE_SIZE * 4] = {};
    uint8_t rdata1[STORAGE_PAGE_PAYLOAD_SIZE] = {};
    uint8_t rdata2[STORAGE_PAGE_SIZE * 4] = {};


    memset(wdata1, 0xF0, sizeof(wdata1));
    memset(wdata2, 0xF0, sizeof(wdata2));

    ASSERT_EQ(sat->find(FIND_MODE_EMPTY, &address), STORAGE_OK);
    nextAddress = address + STORAGE_PAGE_SIZE;
    ASSERT_EQ(sat->save(nextAddress, shortPrefix, 1, wdata1, sizeof(wdata1)), STORAGE_OK);
    ASSERT_EQ(sat->save(address, shortPrefix, 2, wdata2, sizeof(wdata2)), STORAGE_OK);
    address = 0;
    ASSERT_EQ(sat->find(FIND_MODE_EQUAL, &address, shortPrefix, 1), STORAGE_OK);
    ASSERT_EQ(sat->load(address, rdata1, sizeof(rdata1)), STORAGE_OK);
    ASSERT_FALSE(memcmp(wdata1, rdata1, sizeof(wdata1)));
    address = 0;
    ASSERT_EQ(sat->find(FIND_MODE_EQUAL, &address, shortPrefix, 2), STORAGE_OK);
    ASSERT_EQ(sat->load(address, rdata2, sizeof(rdata2)), STORAGE_OK);
    ASSERT_FALSE(memcmp(wdata2, rdata2, sizeof(wdata2)));
}

TEST_F(StorageFixture, FindForAnyAddress)
{
    uint8_t wdata[STORAGE_PAGE_PAYLOAD_SIZE] = {};

    ASSERT_EQ(sat->find(FIND_MODE_EMPTY, &address), STORAGE_OK);
    ASSERT_EQ(address, StorageMacroblock::RESERVED_PAGES_COUNT * STORAGE_PAGE_SIZE);
    ASSERT_EQ(sat->save(address, shortPrefix, 1, wdata, sizeof(wdata)), STORAGE_OK);
    uint32_t lastAddress = address;
    ASSERT_EQ(sat->find(FIND_MODE_NEXT, &address, "", 0), STORAGE_OK);
    ASSERT_EQ(lastAddress, address);
}

TEST_F(StorageFixture, DeleteDataWithBlockedHeader)
{
    uint8_t wdata[STORAGE_PAGE_PAYLOAD_SIZE] = { 1, 2, 3, 4, 5 };

    for (unsigned i = 0; i < StorageMacroblock::RESERVED_PAGES_COUNT * STORAGE_PAGE_SIZE; i++) {
        storage.setBlocked(i, true);
    }

    ASSERT_EQ(sat->find(FIND_MODE_EMPTY, &address), STORAGE_OK);
    ASSERT_EQ(address, StorageMacroblock::RESERVED_PAGES_COUNT * STORAGE_PAGE_SIZE);
    ASSERT_EQ(sat->save(address, shortPrefix, 1, wdata, sizeof(wdata)), STORAGE_OK);
    ASSERT_EQ(sat->deleteData(shortPrefix, 1), STORAGE_OK);
    ASSERT_EQ(sat->find(FIND_MODE_EQUAL, &address, shortPrefix, 1), STORAGE_NOT_FOUND);
}

TEST_F(StorageFixture, DeleteDataWithBlockedPage)
{
    uint8_t wdata[STORAGE_PAGE_PAYLOAD_SIZE] = { 1, 2, 3, 4, 5 };

    ASSERT_EQ(sat->find(FIND_MODE_EMPTY, &address), STORAGE_OK);
    ASSERT_EQ(address, StorageMacroblock::RESERVED_PAGES_COUNT * STORAGE_PAGE_SIZE);
    ASSERT_EQ(sat->save(address, shortPrefix, 1, wdata, sizeof(wdata)), STORAGE_OK);

    for (unsigned i = 0; i < StorageMacroblock::RESERVED_PAGES_COUNT * STORAGE_PAGE_SIZE; i++) {
        storage.setBlocked(StorageMacroblock::RESERVED_PAGES_COUNT * STORAGE_PAGE_SIZE + i, true);
    }

    ASSERT_EQ(sat->deleteData(shortPrefix, 1), STORAGE_OK);
    ASSERT_EQ(sat->find(FIND_MODE_EQUAL, &address, shortPrefix, 1), STORAGE_NOT_FOUND);
}

TEST_F(StorageFixture, TimeCheck)
{
    uint8_t data[STORAGE_PAGE_PAYLOAD_SIZE] = { 0 };

    auto startTime = std::chrono::high_resolution_clock::now();
    sat->format();
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
    std::cout << "Format: " << (double)(duration.count() / 1000.0) << "ms" << std::endl;

    startTime = std::chrono::high_resolution_clock::now();
    sat->save(
        StorageMacroblock::getPageAddressByIndex(StorageMacroblock::getMacroblocksCount() - 1, Header::PAGES_COUNT),
        shortPrefix,
        1,
        data,
        sizeof(data)
    );
    endTime = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
    std::cout << "Save:   " << (double)(duration.count() / 1000.0) << "ms" << std::endl;

    startTime = std::chrono::high_resolution_clock::now();
    sat->find(
        FIND_MODE_EQUAL,
        &address,
        shortPrefix,
        1
    );
    endTime = std::chrono::high_resolution_clock::now();
    std::cout << "Find:   " << (double)(duration.count() / 1000.0) << "ms" << std::endl;

    startTime = std::chrono::high_resolution_clock::now();
    sat->load(address, data, sizeof(data));
    endTime = std::chrono::high_resolution_clock::now();
    std::cout << "Load:   " << (double)(duration.count() / 1000.0) << "ms" << std::endl;

    startTime = std::chrono::high_resolution_clock::now();
    sat->clearAddress(address);
    endTime = std::chrono::high_resolution_clock::now();
    std::cout << "Delete: " << (double)(duration.count() / 1000.0) << "ms" << std::endl;
}

TEST_F(StorageFixture, ChangePagesCount)
{
    uint8_t wdata[STORAGE_PAGE_PAYLOAD_SIZE] = { 1, 2, 3, 4, 5 };
    uint8_t rdata[STORAGE_PAGE_PAYLOAD_SIZE] = { 0 };
    address = StorageMacroblock::RESERVED_PAGES_COUNT * STORAGE_PAGE_SIZE;
    ASSERT_EQ(sat->save(address, shortPrefix, 1, wdata, sizeof(wdata)), STORAGE_OK);
    ASSERT_EQ(sat->load(address, rdata, sizeof(rdata)), STORAGE_OK);
    ASSERT_FALSE(memcmp(wdata, rdata, sizeof(wdata)));

    storage.clear();
    sat->setPagesCount(0);
    ASSERT_EQ(sat->save(address, shortPrefix, 1, wdata, sizeof(wdata)), STORAGE_OOM);
    ASSERT_EQ(sat->load(address, rdata, sizeof(rdata)), STORAGE_OOM);

    sat->setPagesCount(StorageMacroblock::RESERVED_PAGES_COUNT + 1);
    memset(rdata, 0, sizeof(rdata));
    ASSERT_EQ(sat->save(address, shortPrefix, 1, wdata, sizeof(wdata)), STORAGE_OK);
    ASSERT_EQ(sat->load(address, rdata, sizeof(rdata)), STORAGE_OK);
    ASSERT_FALSE(memcmp(wdata, rdata, sizeof(wdata)));

    address += STORAGE_PAGE_SIZE;
    ASSERT_EQ(sat->save(address, shortPrefix, 1, wdata, sizeof(wdata)), STORAGE_OOM);
    ASSERT_EQ(sat->load(address, rdata, sizeof(rdata)), STORAGE_OOM);
}

TEST_F(StorageFixture, DeleteExistingData) {
    uint8_t wdata[STORAGE_PAGE_PAYLOAD_SIZE] = { 1, 2, 3, 4, 5 };

    // Сохраняем данные
    ASSERT_EQ(sat->find(FIND_MODE_EMPTY, &address), STORAGE_OK);
    ASSERT_EQ(sat->save(address, shortPrefix, 1, wdata, sizeof(wdata)), STORAGE_OK);

    // Удаляем данные
    ASSERT_EQ(sat->deleteData(shortPrefix, 1), STORAGE_OK);

    // Проверяем, что данные удалены
    ASSERT_EQ(sat->find(FIND_MODE_EQUAL, &address, shortPrefix, 1), STORAGE_NOT_FOUND);
}

TEST_F(StorageFixture, DeleteNonExistingData) {
    // Попытка удаления несуществующих данных
    ASSERT_EQ(sat->deleteData(shortPrefix, 1), STORAGE_OK); // Должно вернуть OK, даже если данных нет
}

TEST_F(StorageFixture, DeleteDataWithInvalidHeader) {
    uint8_t wdata[STORAGE_PAGE_PAYLOAD_SIZE] = { 1, 2, 3, 4, 5 };

    // Портим заголовок
    Header header(StorageMacroblock::getMacroblockAddress(0));
    for (unsigned i = 0; i < StorageMacroblock::RESERVED_PAGES_COUNT * STORAGE_PAGE_SIZE; i++) {
        storage.setBlocked(i, true);
    }
    ASSERT_EQ(header.save(), STORAGE_HEADER_ERROR);
    ASSERT_EQ(header.create(), STORAGE_OK);

    // Сохраняем данные
    ASSERT_EQ(sat->find(FIND_MODE_EMPTY, &address), STORAGE_OK);
    ASSERT_EQ(sat->save(address, shortPrefix, 1, wdata, sizeof(wdata)), STORAGE_OK);

    // Удаляем данные, несмотря на ошибку в заголовке
    ASSERT_EQ(sat->deleteData(shortPrefix, 1), STORAGE_OK);

    // Проверяем, что данные удалены
    ASSERT_EQ(sat->find(FIND_MODE_EQUAL, &address, shortPrefix, 1), STORAGE_NOT_FOUND);
}
TEST_F(StorageFixture, ClearExistingAddress) {
    uint8_t wdata[STORAGE_PAGE_PAYLOAD_SIZE] = { 1, 2, 3, 4, 5 };

    // Сохраняем данные
    ASSERT_EQ(sat->find(FIND_MODE_EMPTY, &address), STORAGE_OK);
    ASSERT_EQ(sat->save(address, shortPrefix, 1, wdata, sizeof(wdata)), STORAGE_OK);

    // Очищаем адрес
    ASSERT_EQ(sat->clearAddress(address), STORAGE_OK);

    // Проверяем, что данные удалены
    ASSERT_EQ(sat->find(FIND_MODE_EQUAL, &address, shortPrefix, 1), STORAGE_NOT_FOUND);
}

TEST_F(StorageFixture, ClearNonExistingAddress) {
    // Попытка очистки несуществующего адреса
    uint32_t invalidAddress = StorageAT::getStorageSize() + STORAGE_PAGE_SIZE; // Адрес за пределами памяти
    ASSERT_EQ(sat->clearAddress(invalidAddress), STORAGE_ERROR); // Должно вернуть ошибку
}

TEST_F(StorageFixture, ClearAddressWithInvalidHeader) {
    uint8_t wdata[STORAGE_PAGE_PAYLOAD_SIZE] = { 1, 2, 3, 4, 5 };

    // Портим заголовок
    Header header(StorageMacroblock::getMacroblockAddress(0));
    for (unsigned i = 0; i < StorageMacroblock::RESERVED_PAGES_COUNT * STORAGE_PAGE_SIZE; i++) {
        storage.setBlocked(i, true);
    }
    ASSERT_EQ(header.save(), STORAGE_HEADER_ERROR);
    ASSERT_EQ(header.create(), STORAGE_OK);

    // Сохраняем данные
    ASSERT_EQ(sat->find(FIND_MODE_EMPTY, &address), STORAGE_OK);
    ASSERT_EQ(sat->save(address, shortPrefix, 1, wdata, sizeof(wdata)), STORAGE_OK);

    // Очищаем адрес, несмотря на ошибку в заголовке
    ASSERT_EQ(sat->clearAddress(address), STORAGE_OK);

    // Проверяем, что данные удалены
    ASSERT_EQ(sat->find(FIND_MODE_EQUAL, &address, shortPrefix, 1), STORAGE_NOT_FOUND);
}

TEST_F(StorageFixture, FormatMacroblock) {
    uint32_t macroblockIndex = 0;
    ASSERT_EQ(StorageMacroblock::formatMacroblock(macroblockIndex), STORAGE_OK);

    Header header(StorageMacroblock::getMacroblockAddress(macroblockIndex));
    ASSERT_EQ(header.load(), STORAGE_OK);
    ASSERT_TRUE(header.isPageStatus(0, Header::PAGE_EMPTY));
}

TEST_F(StorageFixture, FormatMacroblockWithInvalidHeader) {
    uint32_t macroblockIndex = 0;

    // Портим заголовок
    for (unsigned i = 0; i < StorageMacroblock::RESERVED_PAGES_COUNT * STORAGE_PAGE_SIZE; i++) {
        storage.setBlocked(i, true);
    }
    Header header(StorageMacroblock::getMacroblockAddress(macroblockIndex));
    ASSERT_EQ(header.save(), STORAGE_HEADER_ERROR);
    ASSERT_EQ(header.load(), STORAGE_HEADER_ERROR);

    ASSERT_EQ(StorageMacroblock::formatMacroblock(macroblockIndex), STORAGE_OK);
}

TEST_F(StorageFixture, FillMemoryBreakFirstPayloadAndDeleteSaveNew) {
    uint8_t wdata[STORAGE_PAGE_PAYLOAD_SIZE] = { 1, 2, 3, 4, 5 };
    uint8_t newData[STORAGE_PAGE_PAYLOAD_SIZE] = { 6, 7, 8, 9, 10 };
    uint8_t rdata[STORAGE_PAGE_PAYLOAD_SIZE] = { 0 };

    // Шаг 1: Заполняем всю память одинаковыми данными
    StorageStatus status = STORAGE_OK;
    uint32_t pagesCount = 0;
    while (status == STORAGE_OK) {
        status = sat->find(FIND_MODE_EMPTY, &address);
        if (status != STORAGE_OK) {
            break;
        }
        status = sat->save(address, shortPrefix, pagesCount + 1, wdata, sizeof(wdata));
        EXPECT_EQ(status, STORAGE_OK);
        pagesCount++;
    }

    // Проверяем, что память полностью заполнена
    EXPECT_EQ(pagesCount, StorageAT::getPayloadPagesCount());

    // Шаг 2: Повреждаем первую страницу полезной нагрузки
    uint32_t firstPayloadAddress = StorageMacroblock::RESERVED_PAGES_COUNT * STORAGE_PAGE_SIZE;
    Page firstPage(firstPayloadAddress);
    EXPECT_EQ(firstPage.load(), STORAGE_OK);
    EXPECT_EQ(firstPage.save(), STORAGE_OK);

    // Портим данные первой страницы
    storage.setByte(firstPayloadAddress + STORAGE_PAGE_SIZE / 2, 0xFF);
    EXPECT_EQ(firstPage.load(), STORAGE_ERROR);

    // Шаг 3: Пытаемся удалить данные и сохранить новые
    // Удаляем данные по префиксу и индексу
    EXPECT_EQ(sat->deleteData(shortPrefix, 1), STORAGE_OK);

    // Проверяем, что данные удалены
    EXPECT_EQ(sat->find(FIND_MODE_EQUAL, &address, shortPrefix, 1), STORAGE_NOT_FOUND);

    // Сохраняем новые данные
    EXPECT_EQ(sat->find(FIND_MODE_EMPTY, &address), STORAGE_OK);
    EXPECT_EQ(sat->save(address, shortPrefix, 1, newData, sizeof(newData)), STORAGE_OK);

    // Проверяем, что новые данные сохранены корректно
    EXPECT_EQ(sat->load(address, rdata, sizeof(rdata)), STORAGE_OK);
    EXPECT_FALSE(memcmp(newData, rdata, sizeof(newData)));
}

TEST_F(StorageFixture, FillMemoryWithLongDataBreakFirstPayloadAndDeleteSaveNew) {
    // Шаг 1: Заполняем всю память очень длинными данными
    uint32_t longDataSize = StorageAT::getPayloadSize(); // Очень длинные данные, занимающие всю полезную память
    uint8_t* longData = new uint8_t[longDataSize];
    memset(longData, 0xAA, longDataSize); // Заполняем данные значением 0xAA

    uint32_t address = 0;
    EXPECT_EQ(sat->find(FIND_MODE_EMPTY, &address), STORAGE_OK);
    EXPECT_EQ(sat->save(address, shortPrefix, 1, longData, longDataSize), STORAGE_OK);

    // Проверяем, что память полностью заполнена
    EXPECT_EQ(sat->find(FIND_MODE_EMPTY, &address), STORAGE_NOT_FOUND); // Память должна быть заполнена

    // Шаг 2: Повреждаем первую страницу полезной нагрузки
    uint32_t firstPayloadAddress = StorageMacroblock::RESERVED_PAGES_COUNT * STORAGE_PAGE_SIZE;
    Page firstPage(firstPayloadAddress);
    EXPECT_EQ(firstPage.load(), STORAGE_OK);
    EXPECT_EQ(firstPage.save(), STORAGE_OK);

    // Портим данные первой страницы
    storage.setByte(firstPayloadAddress + STORAGE_PAGE_SIZE / 2, 0xFF);
    EXPECT_EQ(firstPage.load(), STORAGE_ERROR);

    // Шаг 3: Пытаемся удалить данные и сохранить новые
    // Удаляем данные по префиксу и индексу
    EXPECT_EQ(sat->deleteData(shortPrefix, 1), STORAGE_OK);

    // Проверяем, что данные удалены
    EXPECT_EQ(sat->find(FIND_MODE_EQUAL, &address, shortPrefix, 1), STORAGE_NOT_FOUND);

    // Сохраняем новые данные
    uint8_t newData[STORAGE_PAGE_PAYLOAD_SIZE] = { 6, 7, 8, 9, 10 };
    EXPECT_EQ(sat->find(FIND_MODE_EMPTY, &address), STORAGE_OK);
    EXPECT_EQ(sat->save(address, shortPrefix, 2, newData, sizeof(newData)), STORAGE_OK);

    // Проверяем, что новые данные сохранены корректно
    uint8_t rdata[STORAGE_PAGE_PAYLOAD_SIZE] = { 0 };
    EXPECT_EQ(sat->load(address, rdata, sizeof(rdata)), STORAGE_OK);
    EXPECT_FALSE(memcmp(newData, rdata, sizeof(newData)));

    // Освобождаем память
    delete[] longData;
}

/*
 * Tasks:
 * 1. if true header will be blocked, how to find out that?
 * 2. erase several sectors
 */

int main(int args, char** argv)
{
    testing::InitGoogleTest(&args, argv);

    clock_t tStart = clock();
    int result1 = RUN_ALL_TESTS();
    clock_t tEnd = clock();

    storage.showReadWrite();

    std::cout << "Test 1 execution time: " << static_cast<double>(tEnd - tStart) << "ms" << std::endl;

    if (result1) {
        return result1;
    }

    minMemoryEraseSize = STORAGE_PAGE_SIZE * 16;
    tStart = clock();
    int result2 = RUN_ALL_TESTS();
    tEnd = clock();

    storage.showReadWrite();

    std::cout << "Test 2 execution time: " << static_cast<double>(tEnd - tStart) << "ms" << std::endl;

    return result2;
}
