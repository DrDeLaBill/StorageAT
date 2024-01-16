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
const int PAGE_LEN      = 256;


StorageEmulator storage(PAGES_COUNT);


class StorageDriver: public IStorageDriver
{
public: 
    StorageDriver() {}
	StorageStatus read(uint32_t address, uint8_t* data, uint32_t len) override
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
	StorageStatus write(uint32_t address, uint8_t* data, uint32_t len) override
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
            &driver
        );
    }

    void TearDown() override
    {
        sat.reset();
    }
};


TEST(StorageType, Pack)
{
    STORAGE_PACK(typedef struct, _TmpStruct {
        uint8_t value1;
	} TmpStruct);
    EXPECT_EQ(sizeof(struct _TmpStruct), 1);
}

TEST(Page, Struct)
{
    EXPECT_EQ(sizeof(struct Page::_PageMeta), 20);
    EXPECT_EQ(sizeof(struct Page::_PageStruct), PAGE_LEN);
}

TEST(Header, Struct)
{
    EXPECT_EQ(sizeof(struct Header::_MetaUnit), 7);
    EXPECT_EQ(sizeof(struct Header::_MetaStatus), 1);
    EXPECT_EQ(sizeof(struct Header::_HeaderMeta), 232);
}

TEST(StorageMacroblock, Struct)
{
    EXPECT_EQ(StorageMacroblock::PAGES_COUNT, ((Page::PAYLOAD_SIZE * 8) / (sizeof(struct Header::_MetaUnit) * 8 + 2)) + StorageMacroblock::RESERVED_PAGES_COUNT);
}

TEST(StorageMacroblock, CheckSectorAddresses)
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
            /*.pageAddress         = */Page::PAGE_SIZE * StorageMacroblock::RESERVED_PAGES_COUNT,
            /*.isMacroblockAddress = */true
        },
        {
            /*.address             = */1,
            /*.sectorAddress       = */0,
            /*.sectorIndex         = */0,
            /*.pageIndex           = */0,
            /*.pageAddress         = */Page::PAGE_SIZE * StorageMacroblock::RESERVED_PAGES_COUNT,
            /*.isMacroblockAddress = */true
        },
        {
            /*.address             = */Page::PAGE_SIZE - 1,
            /*.sectorAddress       = */0,
            /*.sectorIndex         = */0,
            /*.pageIndex           = */0,
            /*.pageAddress         = */Page::PAGE_SIZE * StorageMacroblock::RESERVED_PAGES_COUNT,
            /*.isMacroblockAddress = */true
        },
        {
            /*.address             = */Page::PAGE_SIZE,
            /*.sectorAddress       = */0,
            /*.sectorIndex         = */0,
            /*.pageIndex           = */0,
            /*.pageAddress         = */Page::PAGE_SIZE * StorageMacroblock::RESERVED_PAGES_COUNT,
            /*.isMacroblockAddress = */true
        },
        {
            /*.address             = */Page::PAGE_SIZE * StorageMacroblock::RESERVED_PAGES_COUNT,
            /*.sectorAddress       = */0,
            /*.sectorIndex         = */0,
            /*.pageIndex           = */0,
            /*.pageAddress         = */Page::PAGE_SIZE * StorageMacroblock::RESERVED_PAGES_COUNT,
            /*.isMacroblockAddress = */false
        },
        {
            /*.address             = */Page::PAGE_SIZE * StorageMacroblock::RESERVED_PAGES_COUNT + 1,
            /*.sectorAddress       = */0,
            /*.sectorIndex         = */0,
            /*.pageIndex           = */0,
            /*.pageAddress         = */Page::PAGE_SIZE * StorageMacroblock::RESERVED_PAGES_COUNT,
            /*.isMacroblockAddress = */false
        },
        {
            /*.address             = */StorageMacroblock::getMacroblockAddress(1) - 1,
            /*.sectorAddress       = */0,
            /*.sectorIndex         = */0,
            /*.pageIndex           = */Header::PAGES_COUNT - 1,
            /*.pageAddress         = */Page::PAGE_SIZE * StorageMacroblock::PAGES_COUNT - Page::PAGE_SIZE,
            /*.isMacroblockAddress = */false
        },
        {
            /*.address             = */StorageMacroblock::getMacroblockAddress(1),
            /*.sectorAddress       = */StorageMacroblock::getMacroblockAddress(1),
            /*.sectorIndex         = */1,
            /*.pageIndex           = */0,
            /*.pageAddress         = */StorageMacroblock::getMacroblockAddress(1) + Page::PAGE_SIZE * StorageMacroblock::RESERVED_PAGES_COUNT,
            /*.isMacroblockAddress = */true
        },
        {
            /*.address             = */StorageMacroblock::getMacroblockAddress(1),
            /*.sectorAddress       = */StorageMacroblock::getMacroblockAddress(1),
            /*.sectorIndex         = */1,
            /*.pageIndex           = */0,
            /*.pageAddress         = */StorageMacroblock::getMacroblockAddress(1) + Page::PAGE_SIZE * StorageMacroblock::RESERVED_PAGES_COUNT,
            /*.isMacroblockAddress = */true
        },
        {
            /*.address             = */StorageMacroblock::getMacroblockAddress(1) + Page::PAGE_SIZE / 2,
            /*.sectorAddress       = */StorageMacroblock::getMacroblockAddress(1),
            /*.sectorIndex         = */1,
            /*.pageIndex           = */0,
            /*.pageAddress         = */StorageMacroblock::getMacroblockAddress(1) + Page::PAGE_SIZE * StorageMacroblock::RESERVED_PAGES_COUNT,
            /*.isMacroblockAddress = */true
        },
        {
            /*.address             = */StorageMacroblock::getMacroblockAddress(1) + Page::PAGE_SIZE * StorageMacroblock::RESERVED_PAGES_COUNT - 1,
            /*.sectorAddress       = */StorageMacroblock::getMacroblockAddress(1),
            /*.sectorIndex         = */1,
            /*.pageIndex           = */0,
            /*.pageAddress         = */StorageMacroblock::getMacroblockAddress(1) + Page::PAGE_SIZE * StorageMacroblock::RESERVED_PAGES_COUNT,
            /*.isMacroblockAddress = */true
        },
        {
            /*.address             = */StorageMacroblock::getMacroblockAddress(1) + Page::PAGE_SIZE * (StorageMacroblock::RESERVED_PAGES_COUNT + 1),
            /*.sectorAddress       = */StorageMacroblock::getMacroblockAddress(1),
            /*.sectorIndex         = */1,
            /*.pageIndex           = */1,
            /*.pageAddress         = */StorageMacroblock::getMacroblockAddress(1) + Page::PAGE_SIZE * (StorageMacroblock::RESERVED_PAGES_COUNT + 1),
            /*.isMacroblockAddress = */false
        },
    };

    for (unsigned i = 0; i < sizeof(addrIndex) / sizeof(*addrIndex); i++) {
        // std::cout << "addr=" << addrIndex[i].address << ",sectorIdx=" << addrIndex[i].sectorIndex << ",pageIdx=" << addrIndex[i].pageIndex << std::endl;
        EXPECT_EQ(StorageMacroblock::getMacroblockAddress(addrIndex[i].sectorIndex), addrIndex[i].sectorAddress);
        EXPECT_EQ(StorageMacroblock::getMacroblockIndex(addrIndex[i].sectorAddress), addrIndex[i].sectorIndex);
        EXPECT_EQ(StorageMacroblock::getMacroblockIndex(addrIndex[i].address), addrIndex[i].sectorIndex);
        EXPECT_EQ(StorageMacroblock::getPageAddressByIndex(addrIndex[i].sectorIndex, addrIndex[i].pageIndex), addrIndex[i].pageAddress);
        EXPECT_EQ(StorageMacroblock::getPageIndexByAddress(addrIndex[i].address), addrIndex[i].pageIndex);
        EXPECT_EQ(StorageMacroblock::isMacroblockAddress(addrIndex[i].address), addrIndex[i].isMacroblockAddress);
        EXPECT_EQ(Header::getMacroblockStartAddress(addrIndex[i].address), addrIndex[i].sectorAddress);
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
    uint32_t address = 0;

    EXPECT_CALL(mockDriver, read)
        .Times(::testing::AtLeast(1));

    EXPECT_CALL(mockDriver, write)
        .Times(::testing::AtLeast(1));

    EXPECT_EQ(sat.find(FIND_MODE_EMPTY, &address), STORAGE_OK);
}

TEST_F(StorageFixture, BadFindRequest)
{
    EXPECT_EQ(sat->find(static_cast<StorageFindMode>(0), &address, nullptr, 0), STORAGE_ERROR);
    EXPECT_EQ(sat->find(FIND_MODE_EQUAL, &address, nullptr, 0), STORAGE_ERROR);
    EXPECT_EQ(sat->find(FIND_MODE_MAX, &address, nullptr, 0), STORAGE_ERROR);
    EXPECT_EQ(sat->find(FIND_MODE_MIN, &address, nullptr, 0), STORAGE_ERROR);
    EXPECT_EQ(sat->find(FIND_MODE_NEXT, &address, nullptr, 0), STORAGE_ERROR);
    EXPECT_EQ(sat->find(FIND_MODE_EMPTY, nullptr), STORAGE_ERROR);
    EXPECT_EQ(sat->find(FIND_MODE_EQUAL, nullptr, nullptr, 0), STORAGE_ERROR);
    EXPECT_EQ(sat->find(FIND_MODE_MAX, nullptr, nullptr, 0), STORAGE_ERROR);
    EXPECT_EQ(sat->find(FIND_MODE_MIN, nullptr, nullptr, 0), STORAGE_ERROR);
    EXPECT_EQ(sat->find(FIND_MODE_NEXT, nullptr, nullptr, 0), STORAGE_ERROR);
    EXPECT_EQ(sat->find(FIND_MODE_EQUAL, nullptr, longPrefix, 0), STORAGE_ERROR);
    EXPECT_EQ(sat->find(FIND_MODE_MAX, nullptr, longPrefix, 0), STORAGE_ERROR);
    EXPECT_EQ(sat->find(FIND_MODE_MIN, nullptr, longPrefix, 0), STORAGE_ERROR);
    EXPECT_EQ(sat->find(FIND_MODE_NEXT, nullptr, longPrefix, 0), STORAGE_ERROR);
    EXPECT_EQ(sat->find(FIND_MODE_EQUAL, nullptr, longPrefix, 0), STORAGE_ERROR);
    EXPECT_EQ(sat->find(FIND_MODE_MAX, nullptr, shortPrefix, 0), STORAGE_ERROR);
    EXPECT_EQ(sat->find(FIND_MODE_MIN, nullptr, shortPrefix, 0), STORAGE_ERROR);
    EXPECT_EQ(sat->find(FIND_MODE_NEXT, nullptr, shortPrefix, 0), STORAGE_ERROR);
}

TEST_F(StorageFixture, BadSaveRequest)
{
    char emptyStr[] = "";

    EXPECT_EQ(sat->save(address, emptyStr, 0, nullptr, 0), STORAGE_ERROR);
    EXPECT_EQ(sat->save(address, emptyStr, 0, nullptr, Page::PAGE_SIZE * 4), STORAGE_ERROR);
    EXPECT_EQ(sat->save(address, emptyStr, 0, nullptr, Page::PAGE_SIZE * 4000), STORAGE_ERROR);
    EXPECT_EQ(sat->save(address, nullptr, 0, nullptr, 0), STORAGE_ERROR);
    EXPECT_EQ(sat->save(address, nullptr, 0, nullptr, Page::PAGE_SIZE * 4), STORAGE_ERROR);
    EXPECT_EQ(sat->save(address, nullptr, 0, nullptr, Page::PAGE_SIZE * 4000), STORAGE_ERROR);
}

TEST_F(StorageFixture, BadLoadRequest)
{
    EXPECT_EQ(sat->load(address, nullptr, 0), STORAGE_ERROR);
    EXPECT_EQ(sat->load(address, nullptr, Page::PAGE_SIZE * 4), STORAGE_ERROR);
    EXPECT_EQ(sat->load(address, nullptr, Page::PAGE_SIZE * 4000), STORAGE_ERROR);
}

TEST_F(StorageFixture, UseWrongPrefix)
{
    uint8_t wdata[100] = {};
    uint8_t rdata[100] = {};

    EXPECT_EQ(sat->find(FIND_MODE_EMPTY, &address), STORAGE_OK);
    EXPECT_EQ(sat->save(address, brokenPrefix, 1, wdata, sizeof(wdata)), STORAGE_OK);
    EXPECT_EQ(sat->find(FIND_MODE_EQUAL, &address, brokenPrefix, 1), STORAGE_OK);
    EXPECT_EQ(sat->load(address, rdata, sizeof(rdata)), STORAGE_OK);
    EXPECT_FALSE(memcmp(wdata, rdata, sizeof(wdata)));
}

TEST_F(StorageFixture, StorageBusy)
{
    storage.setBusy(true);

    EXPECT_EQ(sat->find(FIND_MODE_EQUAL, &address, "", 0), STORAGE_BUSY);
    EXPECT_EQ(sat->find(FIND_MODE_MAX, &address, "", 0), STORAGE_BUSY);
    EXPECT_EQ(sat->find(FIND_MODE_MIN, &address, "", 0), STORAGE_BUSY);
    EXPECT_EQ(sat->find(FIND_MODE_NEXT, &address, "", 0), STORAGE_BUSY);

    address = StorageMacroblock::RESERVED_PAGES_COUNT * Page::PAGE_SIZE;
    EXPECT_EQ(sat->load(address, (new uint8_t[PAGE_LEN]), PAGE_LEN), STORAGE_BUSY);
    EXPECT_EQ(sat->save(address, shortPrefix, 1, (new uint8_t[PAGE_LEN]), PAGE_LEN), STORAGE_BUSY);
}


TEST_F(StorageFixture, FindEmptyAddress)
{
    EXPECT_EQ(sat->find(FIND_MODE_EMPTY, &address), STORAGE_OK);
}

TEST_F(StorageFixture, CheckEmptyPageLoad)
{
    Page page(0);

    EXPECT_EQ(page.load(), STORAGE_ERROR);
    EXPECT_EQ(page.load(/*startPage=*/true), STORAGE_ERROR);
}

TEST_F(StorageFixture, CheckSingleStartPageSave)
{
    uint8_t wdata[Page::PAYLOAD_SIZE] = { 1, 2, 3, 4, 5 };
    Page page(0);

    memcpy(page.page.payload, wdata, sizeof(page.page.payload));
    page.setPrevAddress(0);
    page.setNextAddress(Page::PAGE_SIZE);

    EXPECT_EQ(page.save(), STORAGE_OK);
    EXPECT_EQ(page.load(/*startPage=*/true), STORAGE_OK);
    EXPECT_EQ(page.load(), STORAGE_OK);
    EXPECT_FALSE(memcmp(page.page.payload, wdata, sizeof(page.page.payload)));
    EXPECT_TRUE(page.isStart());
    EXPECT_FALSE(page.isMiddle());
    EXPECT_FALSE(page.isEnd());
    EXPECT_FALSE(page.validatePrevAddress());
    EXPECT_TRUE(page.validateNextAddress());
    EXPECT_EQ(page.loadPrev(), STORAGE_NOT_FOUND);
    EXPECT_EQ(page.loadNext(), STORAGE_ERROR);
    Page tmpPage(0);
    EXPECT_EQ(tmpPage.load(), STORAGE_OK);
    EXPECT_EQ(tmpPage.loadNext(), STORAGE_ERROR);
}

TEST_F(StorageFixture, CheckSingleMiddlePageSave)
{
    uint8_t wdata[Page::PAYLOAD_SIZE] = { 1, 2, 3, 4, 5 };
    Page page(Page::PAGE_SIZE);

    memcpy(page.page.payload, wdata, sizeof(page.page.payload));
    page.setPrevAddress(0);
    page.setNextAddress(Page::PAGE_SIZE * 2);

    EXPECT_EQ(page.save(), STORAGE_OK);
    EXPECT_EQ(page.load(), STORAGE_OK);
    EXPECT_EQ(page.load(/*startPage=*/true), STORAGE_ERROR);
    EXPECT_FALSE(memcmp(page.page.payload, wdata, sizeof(page.page.payload)));
    EXPECT_FALSE(page.isStart());
    EXPECT_TRUE(page.isMiddle());
    EXPECT_FALSE(page.isEnd());
    EXPECT_TRUE(page.validatePrevAddress());
    EXPECT_TRUE(page.validateNextAddress());
    EXPECT_EQ(page.loadPrev(), STORAGE_ERROR);
    Page tmpPage(Page::PAGE_SIZE);
    EXPECT_EQ(tmpPage.load(), STORAGE_OK);
    EXPECT_EQ(tmpPage.loadNext(), STORAGE_ERROR);
}

TEST_F(StorageFixture, CheckSingleEndPageSave)
{
    uint8_t wdata[Page::PAYLOAD_SIZE] = { 1, 2, 3, 4, 5 };
    Page page(Page::PAGE_SIZE * 2);

    memcpy(page.page.payload, wdata, sizeof(page.page.payload));
    page.page.header.prev_addr = 0;
    page.page.header.next_addr = Page::PAGE_SIZE * 2;

    EXPECT_EQ(page.save(), STORAGE_OK);
    EXPECT_EQ(page.load(), STORAGE_OK);
    EXPECT_EQ(page.load(/*startPage=*/true), STORAGE_ERROR);
    EXPECT_FALSE(memcmp(page.page.payload, wdata, sizeof(page.page.payload)));
    EXPECT_FALSE(page.isStart());
    EXPECT_FALSE(page.isMiddle());
    EXPECT_TRUE(page.isEnd());
    EXPECT_TRUE(page.validatePrevAddress());
    EXPECT_FALSE(page.validateNextAddress());
    EXPECT_EQ(page.loadPrev(), STORAGE_ERROR);
    Page tmpPage(Page::PAGE_SIZE * 2);
    EXPECT_EQ(tmpPage.load(), STORAGE_OK);
    EXPECT_EQ(tmpPage.loadNext(), STORAGE_NOT_FOUND);
}

TEST_F(StorageFixture, CheckBrokenMagicLoad)
{
    uint8_t wdata[Page::PAYLOAD_SIZE] = { 1, 2, 3, 4, 5 };
    Page page(Page::PAGE_SIZE * 2);

    memcpy(page.page.payload, wdata, sizeof(page.page.payload));
    page.page.header.prev_addr = 0;
    page.page.header.next_addr = Page::PAGE_SIZE * 2;
    EXPECT_EQ(page.save(), STORAGE_OK);
    page.page.header.magic = 0;
    storage.writePage(page.getAddress(), reinterpret_cast<uint8_t*>(&page.page), sizeof(page.page));

    EXPECT_EQ(page.load(), STORAGE_ERROR);
    EXPECT_EQ(page.load(/*startPage=*/true), STORAGE_ERROR);
}

TEST_F(StorageFixture, CheckBrokenVersionLoad)
{
    uint8_t wdata[Page::PAYLOAD_SIZE] = { 1, 2, 3, 4, 5 };
    Page page(Page::PAGE_SIZE * 2);

    memcpy(page.page.payload, wdata, sizeof(page.page.payload));
    page.page.header.prev_addr = 0;
    page.page.header.next_addr = Page::PAGE_SIZE * 2;
    EXPECT_EQ(page.save(), STORAGE_OK);
    page.page.header.version = Page::STORAGE_VERSION - 1;
    storage.writePage(page.getAddress(), reinterpret_cast<uint8_t*>(&page.page), sizeof(page.page));

    EXPECT_EQ(page.load(), STORAGE_ERROR);
    EXPECT_EQ(page.load(/*startPage=*/true), STORAGE_ERROR);
}

TEST_F(StorageFixture, CheckDataStartPageLoad)
{
    uint8_t wdata[Page::PAYLOAD_SIZE * 3] = { 1, 2, 3, 4, 5 };

    EXPECT_EQ(sat->find(FIND_MODE_EMPTY, &address), STORAGE_OK);
    EXPECT_EQ(sat->save(address, shortPrefix, 1, wdata, sizeof(wdata)), STORAGE_OK);
    Page page(address);

    EXPECT_EQ(page.load(/*startPage=*/true), STORAGE_OK);
    EXPECT_EQ(page.load(), STORAGE_OK);
    EXPECT_FALSE(memcmp(page.page.payload, wdata, sizeof(page.page.payload)));
    EXPECT_TRUE(page.isStart());
    EXPECT_FALSE(page.isMiddle());
    EXPECT_FALSE(page.isEnd());
    EXPECT_FALSE(page.validatePrevAddress());
    EXPECT_TRUE(page.validateNextAddress());
    EXPECT_EQ(page.loadPrev(), STORAGE_NOT_FOUND);
    Page tmpPage(address);
    EXPECT_EQ(tmpPage.load(), STORAGE_OK);
    EXPECT_EQ(tmpPage.loadNext(), STORAGE_OK);
}

TEST_F(StorageFixture, CheckDataMiddlePageLoad)
{
    uint8_t wdata[Page::PAYLOAD_SIZE * 3] = { 1, 2, 3, 4, 5 };

    EXPECT_EQ(sat->find(FIND_MODE_EMPTY, &address), STORAGE_OK);
    EXPECT_EQ(sat->save(address, shortPrefix, 1, wdata, sizeof(wdata)), STORAGE_OK);
    Page page(address + Page::PAGE_SIZE);

    EXPECT_EQ(page.load(/*startPage=*/true), STORAGE_ERROR);
    EXPECT_EQ(page.load(), STORAGE_OK);
    EXPECT_FALSE(memcmp(page.page.payload, wdata + Page::PAYLOAD_SIZE, sizeof(page.page.payload)));
    EXPECT_FALSE(page.isStart());
    EXPECT_TRUE(page.isMiddle());
    EXPECT_FALSE(page.isEnd());
    EXPECT_TRUE(page.validatePrevAddress());
    EXPECT_TRUE(page.validateNextAddress());
    EXPECT_EQ(page.loadPrev(), STORAGE_OK);
    Page tmpPage(address + Page::PAGE_SIZE);
    EXPECT_EQ(tmpPage.load(), STORAGE_OK);
    EXPECT_EQ(tmpPage.loadNext(), STORAGE_OK);
}

TEST_F(StorageFixture, CheckDataEndPageLoad)
{
    uint8_t wdata[Page::PAYLOAD_SIZE * 3] = { 1, 2, 3, 4, 5 };

    EXPECT_EQ(sat->find(FIND_MODE_EMPTY, &address), STORAGE_OK);
    EXPECT_EQ(sat->save(address, shortPrefix, 1, wdata, sizeof(wdata)), STORAGE_OK);
    Page page(address + Page::PAGE_SIZE * 2);

    EXPECT_EQ(page.load(/*startPage=*/true), STORAGE_ERROR);
    EXPECT_EQ(page.load(), STORAGE_OK);
    EXPECT_FALSE(memcmp(page.page.payload, wdata + Page::PAYLOAD_SIZE * 2, sizeof(page.page.payload)));
    EXPECT_FALSE(page.isStart());
    EXPECT_FALSE(page.isMiddle());
    EXPECT_TRUE(page.isEnd());
    EXPECT_TRUE(page.validatePrevAddress());
    EXPECT_FALSE(page.validateNextAddress());
    EXPECT_EQ(page.loadPrev(), STORAGE_OK);
    Page tmpPage(address + Page::PAGE_SIZE * 2);
    EXPECT_EQ(tmpPage.load(), STORAGE_OK);
    EXPECT_EQ(tmpPage.loadNext(), STORAGE_NOT_FOUND);
}

TEST_F(StorageFixture, CheckDataBrokenPayloadStartPageLoad)
{
    uint8_t wdata[Page::PAYLOAD_SIZE * 3] = { 1, 2, 3, 4, 5 };
    uint8_t rdata[Page::PAYLOAD_SIZE * 3] = { 0 };

    EXPECT_EQ(sat->find(FIND_MODE_EMPTY, &address), STORAGE_OK);
    EXPECT_EQ(sat->save(address, shortPrefix, 1, wdata, sizeof(wdata)), STORAGE_OK);
    Page page(address);
    EXPECT_EQ(page.load(/*startPage=*/true), STORAGE_OK);
    page.page.header.magic = 0;
    storage.writePage(page.getAddress(), reinterpret_cast<uint8_t*>(&page.page), sizeof(page.page));

    EXPECT_EQ(sat->find(FIND_MODE_EQUAL, &address, shortPrefix, 1), STORAGE_NOT_FOUND);
    EXPECT_EQ(sat->load(address, rdata, sizeof(rdata)), STORAGE_ERROR);
}

TEST_F(StorageFixture, CheckDataBrokenPayloadSinglePageLoad)
{
    uint8_t wdata[Page::PAYLOAD_SIZE * 3] = { 1, 2, 3, 4, 5 };
    uint8_t rdata[Page::PAYLOAD_SIZE * 3] = { 0 };

    EXPECT_EQ(sat->find(FIND_MODE_EMPTY, &address), STORAGE_OK);
    EXPECT_EQ(sat->save(address, shortPrefix, 1, wdata, sizeof(wdata)), STORAGE_OK);
    Page page(address + Page::PAGE_SIZE);
    EXPECT_EQ(page.load(), STORAGE_OK);
    page.page.header.magic = 0;
    storage.writePage(page.getAddress(), reinterpret_cast<uint8_t*>(&page.page), sizeof(page.page));

    EXPECT_EQ(sat->find(FIND_MODE_EQUAL, &address, shortPrefix, 1), STORAGE_OK);
    EXPECT_EQ(sat->load(address, rdata, sizeof(rdata)), STORAGE_ERROR);
}

TEST_F(StorageFixture, FindPageAfterFormat)
{
    uint8_t wdata[Page::PAYLOAD_SIZE * 3] = { 1, 2, 3, 4, 5 };

    EXPECT_EQ(sat->find(FIND_MODE_EMPTY, &address), STORAGE_OK);
    EXPECT_EQ(sat->save(address, shortPrefix, 1, wdata, sizeof(wdata)), STORAGE_OK);
    EXPECT_EQ(sat->format(), STORAGE_OK);
    EXPECT_EQ(sat->find(FIND_MODE_EQUAL, &address, shortPrefix, 1), STORAGE_NOT_FOUND);
}

TEST_F(StorageFixture, IsSetStatusesInHeader)
{
    Header header(0);

    EXPECT_EQ(sat->format(), STORAGE_OK);
    EXPECT_EQ(header.load(), STORAGE_OK);

    for (unsigned i = 0; i < Header::PAGES_COUNT; i++) {
        uint32_t targetAddress = StorageMacroblock::getPageAddressByIndex(0, i);
        EXPECT_TRUE(header.isPageStatus(i, Header::PAGE_EMPTY));
        EXPECT_TRUE(header.isAddressEmpty(targetAddress));
    }
}

TEST_F(StorageFixture, SetStatusesInHeader)
{
    Header header(0);

    EXPECT_EQ(sat->format(), STORAGE_OK);
    EXPECT_EQ(header.load(), STORAGE_OK);

    for (unsigned i = 0; i < Header::PAGES_COUNT; i++) {
        header.setPageStatus(i, Header::PAGE_OK);
        EXPECT_TRUE(header.isPageStatus(i, Header::PAGE_OK));
    }
}

TEST_F(StorageFixture, SetBlockStatusesInHeader)
{
    Header header(0);

    EXPECT_EQ(sat->format(), STORAGE_OK);
    EXPECT_EQ(header.load(), STORAGE_OK);
    
    for (unsigned i = 0; i < Header::PAGES_COUNT; i++) {
        uint32_t targetAddress = StorageMacroblock::getPageAddressByIndex(0, i);
        header.setAddressBlocked(targetAddress);
        EXPECT_TRUE(header.isPageStatus(i, Header::PAGE_BLOCKED));
    }
}

TEST_F(StorageFixture, AutomaticHeadersCreation)
{
    uint8_t wdata[Page::PAYLOAD_SIZE] = { 1, 2, 3, 4, 5 };
    uint32_t pageAddress = StorageMacroblock::RESERVED_PAGES_COUNT * Page::PAGE_SIZE;
    Page page(pageAddress);

    memcpy(reinterpret_cast<void*>(&page.page), wdata, Page::PAYLOAD_SIZE);
    memcpy(page.page.header.prefix, shortPrefix, sizeof(page.page.header.prefix));
    page.page.header.id = 1;
    page.page.header.prev_addr = pageAddress;
    page.page.header.next_addr = pageAddress;
    page.save();

    EXPECT_EQ(sat->find(FIND_MODE_EQUAL, &address, shortPrefix, 1), STORAGE_OK);
    EXPECT_EQ(address, StorageMacroblock::RESERVED_PAGES_COUNT * Page::PAGE_SIZE);
}

TEST_F(StorageFixture, HeaderSameMeta)
{
    uint8_t wdata[Page::PAYLOAD_SIZE] = { 1, 2, 3, 4, 5 };
    uint32_t pageAddress = StorageMacroblock::RESERVED_PAGES_COUNT * Page::PAGE_SIZE;
    Header header(0);
    Page page(pageAddress);

    memcpy(reinterpret_cast<void*>(&page.page), wdata, Page::PAYLOAD_SIZE);
    memcpy(page.page.header.prefix, shortPrefix, sizeof(page.page.header.prefix));
    page.page.header.id = 1;
    page.page.header.prev_addr = pageAddress;
    page.page.header.next_addr = pageAddress;
    page.save();

    EXPECT_EQ(sat->find(FIND_MODE_EQUAL, &address, shortPrefix, 1), STORAGE_OK);
    EXPECT_EQ(header.load(), STORAGE_OK);
    EXPECT_TRUE(header.isSameMeta(StorageMacroblock::getPageIndexByAddress(page.getAddress()), reinterpret_cast<const uint8_t*>(shortPrefix), 1));
}

TEST_F(StorageFixture, RewritePageWithSameData)
{
    uint8_t wdata1[Page::PAYLOAD_SIZE] = { 1, 2, 3, 4, 5 };
    uint8_t wdata2[Page::PAYLOAD_SIZE] = { 6, 7, 8, 9, 10 };
    uint8_t rdata1[Page::PAYLOAD_SIZE] = { 0 };
    uint8_t rdata2[Page::PAYLOAD_SIZE] = { 0 };

    EXPECT_EQ(sat->find(FIND_MODE_EMPTY, &address), STORAGE_OK);
    EXPECT_EQ(sat->save(address, shortPrefix, 1, wdata1, sizeof(wdata1)), STORAGE_OK);
    EXPECT_EQ(sat->load(address, rdata1, sizeof(rdata1)), STORAGE_OK);
    EXPECT_FALSE(memcmp(wdata1, rdata1, sizeof(wdata1)));
    EXPECT_EQ(sat->save(address, shortPrefix, 1, wdata2, sizeof(wdata2)), STORAGE_OK);
    EXPECT_EQ(sat->load(address, rdata2, sizeof(rdata2)), STORAGE_OK);
    EXPECT_FALSE(memcmp(wdata2, rdata2, sizeof(wdata2)));
}

TEST_F(StorageFixture, RewriteSaveDataByAnotherData)
{
    uint8_t wdata1[Page::PAYLOAD_SIZE] = { 1, 2, 3, 4, 5 };
    uint8_t wdata2[Page::PAYLOAD_SIZE] = { 6, 7, 8, 9, 10 };
    uint8_t rdata1[Page::PAYLOAD_SIZE] = { 0 };
    uint8_t rdata2[Page::PAYLOAD_SIZE] = { 0 };

    EXPECT_EQ(sat->find(FIND_MODE_EMPTY, &address), STORAGE_OK);
    EXPECT_EQ(sat->save(address, shortPrefix, 1, wdata1, sizeof(wdata1)), STORAGE_OK);
    EXPECT_EQ(sat->load(address, rdata1, sizeof(rdata1)), STORAGE_OK);
    EXPECT_FALSE(memcmp(wdata1, rdata1, sizeof(wdata1)));
    EXPECT_EQ(sat->rewrite(address, shortPrefix, 2, wdata2, sizeof(wdata2)), STORAGE_OK);
    EXPECT_EQ(sat->load(address, rdata2, sizeof(rdata2)), STORAGE_OK);
    EXPECT_FALSE(memcmp(wdata2, rdata2, sizeof(wdata2)));
}

TEST_F(StorageFixture, WriteAllStorageBytes)
{
    uint32_t storageSize = StorageAT::getStorageSize();

    EXPECT_EQ(sat->find(FIND_MODE_EMPTY, &address), STORAGE_OK);
    EXPECT_EQ(sat->save(address, shortPrefix, 0, (new uint8_t[storageSize]), storageSize), STORAGE_OOM);
}

TEST_F(StorageFixture, FindAllData)
{
    uint8_t wdata[Page::PAYLOAD_SIZE] = { 1, 2, 3, 4, 5 };
    uint8_t rdata[Page::PAYLOAD_SIZE] = { 0 };

    StorageStatus status = STORAGE_OK;
    uint32_t pagesCount = 0;
    while (status == STORAGE_OK) {
        status = sat->find(FIND_MODE_EMPTY, &address);
        if (status != STORAGE_OK) {
            break;
        }
        status = sat->save(address, shortPrefix, pagesCount + 1, wdata, sizeof(wdata));
        EXPECT_EQ(status, STORAGE_OK);
        memset(rdata, 0, sizeof(rdata));
        status = sat->load(address, rdata, sizeof(rdata));
        EXPECT_EQ(status, STORAGE_OK);
        EXPECT_FALSE(memcmp(wdata, rdata, sizeof(wdata)));

        pagesCount++;
    }

    EXPECT_EQ(pagesCount, StorageAT::getPayloadPagesCount());
    for (unsigned i = 0; i < pagesCount; i++) {
        EXPECT_EQ(sat->find(FIND_MODE_EQUAL, &address, shortPrefix, i + 1), STORAGE_OK);
    }
    EXPECT_EQ(sat->find(FIND_MODE_MIN, &address, shortPrefix), STORAGE_OK);
    EXPECT_EQ(address, Page::PAGE_SIZE * StorageMacroblock::RESERVED_PAGES_COUNT);
    EXPECT_EQ(sat->find(FIND_MODE_MAX, &address, shortPrefix), STORAGE_OK);
    EXPECT_EQ(address, Page::PAGE_SIZE * (StorageAT::getStoragePagesCount() - 1));
    EXPECT_EQ(sat->find(FIND_MODE_NEXT, &address, shortPrefix, 1), STORAGE_OK);
    EXPECT_EQ(address, Page::PAGE_SIZE * (StorageMacroblock::RESERVED_PAGES_COUNT + 1));
}

TEST_F(StorageFixture, WriteAllPayloadBytes)
{
    uint32_t payloadSize = StorageAT::getPayloadSize();

    EXPECT_EQ(sat->find(FIND_MODE_EMPTY, &address), STORAGE_OK);
    EXPECT_EQ(sat->save(address, shortPrefix, 0, (new uint8_t[payloadSize]), payloadSize), STORAGE_OK);
}

TEST_F(StorageFixture, DeleteData)
{
    uint8_t wdata[Page::PAYLOAD_SIZE] = { 1, 2, 3, 4, 5 };

    EXPECT_EQ(sat->find(FIND_MODE_EMPTY, &address), STORAGE_OK);
    EXPECT_EQ(sat->save(address, shortPrefix, 0, wdata, sizeof(wdata)), STORAGE_OK);
    EXPECT_EQ(sat->find(FIND_MODE_EQUAL, &address, shortPrefix, 0), STORAGE_OK);
    EXPECT_EQ(sat->deleteData(address), STORAGE_OK);
    EXPECT_EQ(sat->find(FIND_MODE_EQUAL, &address, shortPrefix, 0), STORAGE_NOT_FOUND);
}

TEST_F(StorageFixture, ReadUnacceptableAddress)
{
    address = StorageAT::getStorageSize();

    EXPECT_EQ(sat->load(address, (new uint8_t[Page::PAGE_SIZE]), Page::PAGE_SIZE), STORAGE_OOM);
}

TEST_F(StorageFixture, WriteUnacceptableAddress)
{
    address = StorageAT::getStorageSize() + Page::PAGE_SIZE;

    EXPECT_EQ(sat->save(address, shortPrefix, 1, (new uint8_t[Page::PAGE_SIZE]), Page::PAGE_SIZE), STORAGE_OOM);
}

TEST_F(StorageFixture, LoadEmptyPage)
{
    uint8_t data[Page::PAYLOAD_SIZE] = {};

    EXPECT_EQ(sat->load(0, data, 0), STORAGE_ERROR);
    EXPECT_EQ(sat->load(0, data, sizeof(data)), STORAGE_ERROR);
}

TEST_F(StorageFixture, SaveSimplePageInSectorHeader)
{
    uint8_t data[Page::PAYLOAD_SIZE] = {};

    EXPECT_EQ(sat->save(0, shortPrefix, 1, data, sizeof(data)), STORAGE_ERROR);
    EXPECT_EQ(sat->load(0, data, sizeof(data)), STORAGE_ERROR);
}

TEST_F(StorageFixture, SavePageWithFindEmptyAddress)
{
    uint8_t wdata[Page::PAYLOAD_SIZE] = { 1, 2, 3, 4, 5 };
    uint8_t rdata[Page::PAYLOAD_SIZE] = {};

    EXPECT_EQ(sat->find(FIND_MODE_EMPTY, &address), STORAGE_OK);
    EXPECT_EQ(address, StorageMacroblock::RESERVED_PAGES_COUNT * Page::PAGE_SIZE);
    EXPECT_EQ(sat->save(address, shortPrefix, 1, wdata, sizeof(wdata)), STORAGE_OK);
    EXPECT_EQ(sat->load(address, rdata, sizeof(rdata)), STORAGE_OK);
    EXPECT_FALSE(memcmp(wdata, rdata, sizeof(wdata)));
}


TEST_F(StorageFixture, SavePageWithFindEmptyAddressWithOverwrite)
{
    uint8_t wdata[Page::PAYLOAD_SIZE] = { 1, 2, 3, 4, 5 };

    EXPECT_EQ(sat->find(FIND_MODE_EMPTY, &address), STORAGE_OK);
    EXPECT_EQ(sat->save(address, shortPrefix, 1, wdata, sizeof(wdata)), STORAGE_OK);
    EXPECT_EQ(sat->save(address, shortPrefix, 1, wdata, sizeof(wdata)), STORAGE_OK);
    EXPECT_EQ(sat->save(address, shortPrefix, 2, wdata, sizeof(wdata)), STORAGE_DATA_EXISTS);
}

TEST_F(StorageFixture, SaveMultiPageInHeader)
{
    uint8_t data[Page::PAGE_SIZE * 4] = {};

    EXPECT_EQ(sat->save(0, shortPrefix, 1, data, sizeof(data)), STORAGE_ERROR);
    EXPECT_EQ(sat->load(0, data, sizeof(data)), STORAGE_ERROR);
}

TEST_F(StorageFixture, SaveMultiPageWithFindEmptyAddress)
{
    uint8_t wdata[Page::PAGE_SIZE * 4] = { 1, 2, 3, 4, 5 };
    uint8_t rdata[Page::PAGE_SIZE * 4] = {};

    EXPECT_EQ(sat->find(FIND_MODE_EMPTY, &address), STORAGE_OK);
    EXPECT_EQ(address, StorageMacroblock::RESERVED_PAGES_COUNT * Page::PAGE_SIZE);
    EXPECT_EQ(sat->save(address, shortPrefix, 1, wdata, sizeof(wdata)), STORAGE_OK);
    EXPECT_EQ(sat->load(address, rdata, sizeof(rdata)), STORAGE_OK);
    EXPECT_FALSE(memcmp(wdata, rdata, sizeof(wdata)));
}

TEST_F(StorageFixture, SaveMultiPageWithFindEmptyAddressWithOverwrite)
{
    uint8_t wdata[Page::PAGE_SIZE * 4] = { 1, 2, 3, 4, 5 };

    EXPECT_EQ(sat->find(FIND_MODE_EMPTY, &address), STORAGE_OK);
    EXPECT_EQ(sat->save(address, shortPrefix, 1, wdata, sizeof(wdata)), STORAGE_OK);
    EXPECT_EQ(sat->save(address, shortPrefix, 1, wdata, sizeof(wdata)), STORAGE_OK);
    EXPECT_EQ(sat->save(address, shortPrefix, 2, wdata, sizeof(wdata)), STORAGE_DATA_EXISTS);
}

TEST_F(StorageFixture, SaveDataInBusyPages)
{
    uint8_t wdata1[Page::PAGE_SIZE * 4] = { 1, 2, 3, 4, 5 };
    uint8_t wdata2[Page::PAYLOAD_SIZE] = { 6, 7, 8, 9, 10 };
    uint8_t rdata1[Page::PAGE_SIZE * 4] = {};

    EXPECT_EQ(sat->find(FIND_MODE_EMPTY, &address), STORAGE_OK);
    EXPECT_EQ(sat->save(address, shortPrefix, 1, wdata1, sizeof(wdata1)), STORAGE_OK);
    EXPECT_EQ(sat->save(address, shortPrefix, 2, wdata2, sizeof(wdata2)), STORAGE_DATA_EXISTS);
    EXPECT_EQ(sat->save(address + PAGE_LEN, shortPrefix, 2, wdata2, sizeof(wdata2)), STORAGE_DATA_EXISTS);
    EXPECT_EQ(sat->load(address, rdata1, sizeof(rdata1)), STORAGE_OK);
    EXPECT_FALSE(memcmp(wdata1, rdata1, sizeof(wdata1)));
}

TEST_F(StorageFixture, LoadNotAlignedAddress)
{
    uint8_t rdata[Page::PAYLOAD_SIZE] = {};

    EXPECT_EQ(sat->find(FIND_MODE_EMPTY, &address), STORAGE_OK);
    EXPECT_EQ(sat->load(address + 1, rdata, sizeof(rdata)), STORAGE_ERROR);
}

TEST_F(StorageFixture, SaveNotAlignedAddress)
{
    uint8_t wdata[Page::PAYLOAD_SIZE] = {};

    EXPECT_EQ(sat->find(FIND_MODE_EMPTY, &address), STORAGE_OK);
    EXPECT_EQ(sat->save(address + 1, shortPrefix, 1, wdata, sizeof(wdata)), STORAGE_ERROR);
}

TEST_F(StorageFixture, SaveDataOnBlockedPage)
{
    address = StorageMacroblock::RESERVED_PAGES_COUNT * Page::PAGE_SIZE;
    uint8_t wdata[Page::PAYLOAD_SIZE] = { 1, 2, 3, 4, 5 };
    Header header(address);
    uint32_t tmpAddress = 0;

    storage.setBlocked(address, true);

    EXPECT_EQ(sat->save(address, shortPrefix, 1, wdata, sizeof(wdata)), STORAGE_OK);
    EXPECT_EQ(StorageMacroblock::loadHeader(&header), STORAGE_OK);

    uint32_t pageIndex = StorageMacroblock::getPageIndexByAddress(address);
    EXPECT_TRUE(header.isPageStatus(pageIndex, Header::PAGE_BLOCKED));
    EXPECT_EQ(sat->find(FIND_MODE_EQUAL, &tmpAddress, shortPrefix, 1), STORAGE_OK);
    EXPECT_NE(address, tmpAddress);
    EXPECT_EQ(header.load(), STORAGE_OK);
    EXPECT_TRUE(header.isPageStatus(pageIndex, Header::PAGE_BLOCKED));
}

TEST_F(StorageFixture, SaveDataOnBlockedSector)
{
    address = StorageMacroblock::RESERVED_PAGES_COUNT * Page::PAGE_SIZE;
    uint8_t wdata[Page::PAYLOAD_SIZE] = { 1, 2, 3, 4, 5 };
    Header header(address);
    uint32_t tmpAddress = 0;

    for (unsigned i = 0; i < StorageMacroblock::PAGES_COUNT * Page::PAGE_SIZE; i++) {
        storage.setBlocked(address + i, true);
    }

    EXPECT_EQ(sat->save(address, shortPrefix, 1, wdata, sizeof(wdata)), STORAGE_OK);
    EXPECT_EQ(StorageMacroblock::loadHeader(&header), STORAGE_OK);

    for (unsigned i = 0; i < Header::PAGES_COUNT; i++) {
        EXPECT_TRUE(header.isPageStatus(i, Header::PAGE_BLOCKED));
    }
    EXPECT_EQ(sat->find(FIND_MODE_EQUAL, &tmpAddress, shortPrefix, 1), STORAGE_OK);
    EXPECT_NE(address, tmpAddress);
    EXPECT_EQ(header.load(), STORAGE_OK);
    EXPECT_TRUE(header.isPageStatus(StorageMacroblock::getPageIndexByAddress(address), Header::PAGE_BLOCKED));
}

TEST_F(StorageFixture, BlockAllMemory)
{
    uint8_t wdata[Page::PAYLOAD_SIZE] = { 1, 2, 3, 4, 5 };
    address = StorageMacroblock::RESERVED_PAGES_COUNT * Page::PAGE_SIZE;
    
    for (unsigned i = 0; i < storage.getSize(); i++) {
        storage.setBlocked(i, true);
    }

    EXPECT_EQ(sat->save(address, shortPrefix, 1, wdata, sizeof(wdata)), STORAGE_OOM);
    EXPECT_EQ(sat->find(FIND_MODE_EQUAL, &address, shortPrefix, 1), STORAGE_NOT_FOUND);
}

TEST_F(StorageFixture, SetHeaderBlocked)
{
    address = 0;
    Header header(address);

    storage.setBlocked(address, true);

    EXPECT_EQ(StorageMacroblock::loadHeader(&header), STORAGE_OK);
    EXPECT_NE(header.getAddress(), address);
}

TEST_F(StorageFixture, SetAllHeadersBlocked)
{
    Header header(0);

    for (unsigned i = 0; i < StorageMacroblock::RESERVED_PAGES_COUNT; i++) {
        storage.setBlocked(address + Page::PAGE_SIZE * i, true);
    }

    EXPECT_EQ(StorageMacroblock::loadHeader(&header), STORAGE_ERROR);
}

TEST_F(StorageFixture, SaveAndFindDataWithBlockedAllHeaders)
{
    uint8_t wdata[Page::PAYLOAD_SIZE] = { 1, 2, 3, 4, 5 };
    uint8_t rdata[Page::PAYLOAD_SIZE] = { 0 };

    for (unsigned i = 0; i < StorageMacroblock::RESERVED_PAGES_COUNT; i++) {
        storage.setBlocked(address + Page::PAGE_SIZE * i, true);
    }

    EXPECT_EQ(sat->find(FIND_MODE_EMPTY, &address), STORAGE_OK);
    EXPECT_EQ(address, StorageMacroblock::RESERVED_PAGES_COUNT * Page::PAGE_SIZE);
    EXPECT_EQ(sat->save(address, shortPrefix, 1, wdata, sizeof(wdata)), STORAGE_OK);
    EXPECT_EQ(sat->find(FIND_MODE_EQUAL, &address, shortPrefix, 1), STORAGE_OK);
    EXPECT_EQ(sat->load(address, rdata, sizeof(rdata)), STORAGE_OK);
    EXPECT_FALSE(memcmp(wdata, rdata, sizeof(wdata)));
}

TEST_F(StorageFixture, FindEqual)
{
    uint8_t wdata[Page::PAYLOAD_SIZE] = { 1, 2, 3, 4, 5 };
    uint8_t rdata[Page::PAYLOAD_SIZE] = {};
    uint32_t emptyAddress = 0;

    EXPECT_EQ(sat->find(FIND_MODE_EMPTY, &emptyAddress), STORAGE_OK);
    EXPECT_EQ(sat->save(emptyAddress, shortPrefix, 1, wdata, sizeof(wdata)), STORAGE_OK);

    EXPECT_EQ(sat->find(FIND_MODE_EQUAL, &address, shortPrefix, 1), STORAGE_OK);
    EXPECT_EQ(address, emptyAddress);
    EXPECT_EQ(sat->load(address, rdata, sizeof(rdata)), STORAGE_OK);
    EXPECT_FALSE(memcmp(wdata, rdata, sizeof(wdata)));
}

TEST_F(StorageFixture, SaveFindLoadPartitionedData)
{
    uint32_t nextAddress = 0;
    uint8_t wdata1[Page::PAYLOAD_SIZE] = {};
    uint8_t wdata2[Page::PAGE_SIZE * 4] = {};
    uint8_t rdata1[Page::PAYLOAD_SIZE] = {};
    uint8_t rdata2[Page::PAGE_SIZE * 4] = {};


    memset(wdata1, 0xF0, sizeof(wdata1));
    memset(wdata2, 0xF0, sizeof(wdata2));

    EXPECT_EQ(sat->find(FIND_MODE_EMPTY, &address), STORAGE_OK);
    nextAddress = address + Page::PAGE_SIZE;
    EXPECT_EQ(sat->save(nextAddress, shortPrefix, 1, wdata1, sizeof(wdata1)), STORAGE_OK);
    EXPECT_EQ(sat->save(address, shortPrefix, 2, wdata2, sizeof(wdata2)), STORAGE_OK);
    address = 0;
    EXPECT_EQ(sat->find(FIND_MODE_EQUAL, &address, shortPrefix, 1), STORAGE_OK);
    EXPECT_EQ(sat->load(address, rdata1, sizeof(rdata1)), STORAGE_OK);
    EXPECT_FALSE(memcmp(wdata1, rdata1, sizeof(wdata1)));
    address = 0;
    EXPECT_EQ(sat->find(FIND_MODE_EQUAL, &address, shortPrefix, 2), STORAGE_OK);
    EXPECT_EQ(sat->load(address, rdata2, sizeof(rdata2)), STORAGE_OK);
    EXPECT_FALSE(memcmp(wdata2, rdata2, sizeof(wdata2)));
}

TEST_F(StorageFixture, TimeCheck)
{
    uint8_t data[Page::PAYLOAD_SIZE] = { 0 };

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
    sat->deleteData(address);
    endTime = std::chrono::high_resolution_clock::now();
    std::cout << "Delete: " << (double)(duration.count() / 1000.0) << "ms" << std::endl;
}

/*
 * Tests:
 * 1. search for empty prefix = "" (search for any string)
 * 2. erase several sectors
 */

int main(int args, char** argv)
{
    testing::InitGoogleTest(&args, argv);

    clock_t tStart = clock();
    int result = RUN_ALL_TESTS();
    clock_t tEnd = clock();

    storage.showReadWrite();

    std::cout << "Test execution time: " << static_cast<double>(tEnd - tStart) << "ms" << std::endl;

    return result;
}
