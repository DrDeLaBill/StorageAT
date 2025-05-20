/* Copyright © 2025 Georgy E. All rights reserved. */

#include <iostream>
#include <string>
#include <chrono>

#include "StorageAT.h"
#include "StorageDriver.hpp"
#include "StorageFixture.hpp"
#include "StorageEmulator.hpp"


using SF = StorageFixtureAsync;


TEST_F(StorageFixtureAsync, BadFindRequest)
{
    ASSERT_EQ(SF::asyncFind(static_cast<StorageFindMode>(0), &address, nullptr, 0), STORAGE_ERROR);
    ASSERT_EQ(SF::asyncFind(FIND_MODE_EQUAL, &address, nullptr, 0), STORAGE_ERROR);
    ASSERT_EQ(SF::asyncFind(FIND_MODE_MAX, &address, nullptr, 0), STORAGE_ERROR);
    ASSERT_EQ(SF::asyncFind(FIND_MODE_MIN, &address, nullptr, 0), STORAGE_ERROR);
    ASSERT_EQ(SF::asyncFind(FIND_MODE_NEXT, &address, nullptr, 0), STORAGE_ERROR);
    ASSERT_EQ(SF::asyncFind(FIND_MODE_EMPTY, nullptr), STORAGE_ERROR);
    ASSERT_EQ(SF::asyncFind(FIND_MODE_EQUAL, nullptr, nullptr, 0), STORAGE_ERROR);
    ASSERT_EQ(SF::asyncFind(FIND_MODE_MAX, nullptr, nullptr, 0), STORAGE_ERROR);
    ASSERT_EQ(SF::asyncFind(FIND_MODE_MIN, nullptr, nullptr, 0), STORAGE_ERROR);
    ASSERT_EQ(SF::asyncFind(FIND_MODE_NEXT, nullptr, nullptr, 0), STORAGE_ERROR);
    ASSERT_EQ(SF::asyncFind(FIND_MODE_EQUAL, nullptr, longPrefix, 0), STORAGE_ERROR);
    ASSERT_EQ(SF::asyncFind(FIND_MODE_MAX, nullptr, longPrefix, 0), STORAGE_ERROR);
    ASSERT_EQ(SF::asyncFind(FIND_MODE_MIN, nullptr, longPrefix, 0), STORAGE_ERROR);
    ASSERT_EQ(SF::asyncFind(FIND_MODE_NEXT, nullptr, longPrefix, 0), STORAGE_ERROR);
    ASSERT_EQ(SF::asyncFind(FIND_MODE_EQUAL, nullptr, longPrefix, 0), STORAGE_ERROR);
    ASSERT_EQ(SF::asyncFind(FIND_MODE_MAX, nullptr, shortPrefix, 0), STORAGE_ERROR);
    ASSERT_EQ(SF::asyncFind(FIND_MODE_MIN, nullptr, shortPrefix, 0), STORAGE_ERROR);
    ASSERT_EQ(SF::asyncFind(FIND_MODE_NEXT, nullptr, shortPrefix, 0), STORAGE_ERROR);
}

TEST_F(StorageFixtureAsync, BadSaveRequest)
{
    char emptyStr[] = "";

    ASSERT_EQ(SF::asyncSave(address, emptyStr, 0, nullptr, 0), STORAGE_ERROR);
    ASSERT_EQ(SF::asyncSave(address, emptyStr, 0, nullptr, STORAGE_PAGE_SIZE * 4), STORAGE_ERROR);
    ASSERT_EQ(SF::asyncSave(address, emptyStr, 0, nullptr, STORAGE_PAGE_SIZE * 4000), STORAGE_ERROR);
    ASSERT_EQ(SF::asyncSave(address, nullptr, 0, nullptr, 0), STORAGE_ERROR);
    ASSERT_EQ(SF::asyncSave(address, nullptr, 0, nullptr, STORAGE_PAGE_SIZE * 4), STORAGE_ERROR);
    ASSERT_EQ(SF::asyncSave(address, nullptr, 0, nullptr, STORAGE_PAGE_SIZE * 4000), STORAGE_ERROR);
}

TEST_F(StorageFixtureAsync, BadLoadRequest)
{
    ASSERT_EQ(SF::asyncLoad(address, nullptr, 0), STORAGE_ERROR);
    ASSERT_EQ(SF::asyncLoad(address, nullptr, STORAGE_PAGE_SIZE * 4), STORAGE_ERROR);
    ASSERT_EQ(SF::asyncLoad(address, nullptr, STORAGE_PAGE_SIZE * 4000), STORAGE_ERROR);
}

TEST_F(StorageFixtureAsync, UseWrongPrefix)
{
    uint8_t wdata[100] = {};
    uint8_t rdata[100] = {};

    ASSERT_EQ(SF::asyncFind(FIND_MODE_EMPTY, &address), STORAGE_OK);
    ASSERT_EQ(SF::asyncSave(address, brokenPrefix, 1, wdata, sizeof(wdata)), STORAGE_OK);
    ASSERT_EQ(SF::asyncFind(FIND_MODE_EQUAL, &address, brokenPrefix, 1), STORAGE_OK);
    ASSERT_EQ(SF::asyncLoad(address, rdata, sizeof(rdata)), STORAGE_OK);
    ASSERT_FALSE(memcmp(wdata, rdata, sizeof(wdata)));
}

TEST_F(StorageFixtureAsync, StorageBusy)
{
    SF::storage.setBusy(true);

    ASSERT_EQ(SF::asyncFind(FIND_MODE_EQUAL, &address, shortPrefix, 1), STORAGE_BUSY);
    ASSERT_EQ(SF::asyncFind(FIND_MODE_MAX, &address, shortPrefix, 1), STORAGE_BUSY);
    ASSERT_EQ(SF::asyncFind(FIND_MODE_MIN, &address, shortPrefix, 1), STORAGE_BUSY);
    ASSERT_EQ(SF::asyncFind(FIND_MODE_NEXT, &address, shortPrefix, 1), STORAGE_BUSY);
    ASSERT_EQ(SF::asyncFind(FIND_MODE_EMPTY, &address), STORAGE_BUSY);

    address = StorageMacroblock::RESERVED_PAGES_COUNT * STORAGE_PAGE_SIZE;
    ASSERT_EQ(SF::asyncLoad(address, (new uint8_t[PAGE_LEN]), PAGE_LEN), STORAGE_BUSY);
    ASSERT_EQ(SF::asyncSave(address, shortPrefix, 1, (new uint8_t[PAGE_LEN]), PAGE_LEN), STORAGE_BUSY);
}


TEST_F(StorageFixtureAsync, FindEmptyAddress)
{
    ASSERT_EQ(SF::asyncFind(FIND_MODE_EMPTY, &address), STORAGE_OK);
}

TEST_F(StorageFixtureAsync, CheckEmptyPageLoad)
{
    Page page(0);

    ASSERT_EQ(page.load(), STORAGE_ERROR);
    ASSERT_EQ(page.load(/*startPage=*/true), STORAGE_ERROR);
}

TEST_F(StorageFixtureAsync, CheckSingleStartPageSave)
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

TEST_F(StorageFixtureAsync, CheckSingleMiddlePageSave)
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

TEST_F(StorageFixtureAsync, CheckSingleEndPageSave)
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

TEST_F(StorageFixtureAsync, CheckBrokenMagicLoad)
{
    uint8_t wdata[STORAGE_PAGE_PAYLOAD_SIZE] = { 1, 2, 3, 4, 5 };
    Page page(STORAGE_PAGE_SIZE * 2);

    memcpy(page.page.payload, wdata, sizeof(page.page.payload));
    page.page.header.prev_addr = 0;
    page.page.header.next_addr = STORAGE_PAGE_SIZE * 2;
    ASSERT_EQ(page.save(), STORAGE_OK);
    page.page.header.magic = 0;
    SF::storage.writePage(page.getAddress(), reinterpret_cast<uint8_t*>(&page.page), sizeof(page.page));

    ASSERT_EQ(page.load(), STORAGE_ERROR);
    ASSERT_EQ(page.load(/*startPage=*/true), STORAGE_ERROR);
}

TEST_F(StorageFixtureAsync, CheckBrokenVersionLoad)
{
    uint8_t wdata[STORAGE_PAGE_PAYLOAD_SIZE] = { 1, 2, 3, 4, 5 };
    Page page(STORAGE_PAGE_SIZE * 2);

    memcpy(page.page.payload, wdata, sizeof(page.page.payload));
    page.page.header.prev_addr = 0;
    page.page.header.next_addr = STORAGE_PAGE_SIZE * 2;
    ASSERT_EQ(page.save(), STORAGE_OK);
    page.page.header.version = STORAGE_VERSION - 1;
    SF::storage.writePage(page.getAddress(), reinterpret_cast<uint8_t*>(&page.page), sizeof(page.page));

    ASSERT_EQ(page.load(), STORAGE_ERROR);
    ASSERT_EQ(page.load(/*startPage=*/true), STORAGE_ERROR);
}

TEST_F(StorageFixtureAsync, CheckDataStartPageLoad)
{
    uint8_t wdata[STORAGE_PAGE_PAYLOAD_SIZE * 3] = {};
    for (unsigned i = 0; i < sizeof(wdata); i++) {
        wdata[i] = (uint8_t)i;
    }

    ASSERT_EQ(SF::asyncFind(FIND_MODE_EMPTY, &address), STORAGE_OK);
    ASSERT_EQ(SF::asyncSave(address, shortPrefix, 1, wdata, sizeof(wdata)), STORAGE_OK);
    
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

    ASSERT_EQ(page.loadNext(), STORAGE_OK);
    ASSERT_EQ(page.loadPrev(), STORAGE_OK);

    address += STORAGE_PAGE_SIZE;
    page.setAddress(address);
    ASSERT_EQ(page.load(/*startPage=*/true), STORAGE_ERROR);
    ASSERT_EQ(page.load(), STORAGE_OK);
    ASSERT_FALSE(memcmp(page.page.payload, wdata + STORAGE_PAGE_PAYLOAD_SIZE, sizeof(page.page.payload)));
    ASSERT_FALSE(page.isStart());
    ASSERT_TRUE(page.isMiddle());
    ASSERT_FALSE(page.isEnd());
    ASSERT_TRUE(page.validatePrevAddress());
    ASSERT_TRUE(page.validateNextAddress());
    ASSERT_EQ(page.loadPrev(), STORAGE_OK);

    address += STORAGE_PAGE_SIZE;
    page.setAddress(address);
    ASSERT_EQ(page.load(/*startPage=*/true), STORAGE_ERROR);
    ASSERT_EQ(page.load(), STORAGE_OK);
    ASSERT_FALSE(memcmp(page.page.payload, wdata + 2 * STORAGE_PAGE_PAYLOAD_SIZE, sizeof(page.page.payload)));
    ASSERT_FALSE(page.isStart());
    ASSERT_FALSE(page.isMiddle());
    ASSERT_TRUE(page.isEnd());
    ASSERT_TRUE(page.validatePrevAddress());
    ASSERT_FALSE(page.validateNextAddress());
    ASSERT_EQ(page.loadPrev(), STORAGE_OK);
}

TEST_F(StorageFixtureAsync, CheckDataMiddlePageLoad)
{
    uint8_t wdata[STORAGE_PAGE_PAYLOAD_SIZE * 3] = { 1, 2, 3, 4, 5 };

    ASSERT_EQ(SF::asyncFind(FIND_MODE_EMPTY, &address), STORAGE_OK);
    ASSERT_EQ(SF::asyncSave(address, shortPrefix, 1, wdata, sizeof(wdata)), STORAGE_OK);
    
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

    page.setAddress(address + STORAGE_PAGE_SIZE);
    ASSERT_EQ(page.load(), STORAGE_OK);
    ASSERT_EQ(page.loadNext(), STORAGE_OK);

    page.setAddress(/*startPage=*/address);
    ASSERT_EQ(page.load(true), STORAGE_OK);
    ASSERT_EQ(page.loadNext(), STORAGE_OK);
    ASSERT_EQ(page.loadNext(), STORAGE_OK);
    ASSERT_EQ(page.loadNext(), STORAGE_NOT_FOUND);
}

TEST_F(StorageFixtureAsync, CheckDataEndPageLoad)
{
    uint8_t wdata[STORAGE_PAGE_PAYLOAD_SIZE * 3] = { 1, 2, 3, 4, 5 };

    ASSERT_EQ(SF::asyncFind(FIND_MODE_EMPTY, &address), STORAGE_OK);
    ASSERT_EQ(SF::asyncSave(address, shortPrefix, 1, wdata, sizeof(wdata)), STORAGE_OK);
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

TEST_F(StorageFixtureAsync, CheckDataBrokenPayloadStartPageLoad)
{
    uint8_t wdata[STORAGE_PAGE_PAYLOAD_SIZE * 3] = { 1, 2, 3, 4, 5 };
    uint8_t rdata[STORAGE_PAGE_PAYLOAD_SIZE * 3] = { 0 };

    ASSERT_EQ(SF::asyncFind(FIND_MODE_EMPTY, &address), STORAGE_OK);
    ASSERT_EQ(SF::asyncSave(address, shortPrefix, 1, wdata, sizeof(wdata)), STORAGE_OK);
    Page page(address);
    ASSERT_EQ(page.load(/*startPage=*/true), STORAGE_OK);
    page.page.header.magic = 0;
    SF::storage.writePage(page.getAddress(), reinterpret_cast<uint8_t*>(&page.page), sizeof(page.page));

    ASSERT_EQ(SF::asyncFind(FIND_MODE_EQUAL, &address, shortPrefix, 1), STORAGE_NOT_FOUND);
    ASSERT_EQ(SF::asyncLoad(address, rdata, sizeof(rdata)), STORAGE_ERROR);
}

TEST_F(StorageFixtureAsync, CheckDataBrokenPayloadSinglePageLoad)
{
    uint8_t wdata[STORAGE_PAGE_PAYLOAD_SIZE * 3] = { 1, 2, 3, 4, 5 };
    uint8_t rdata[STORAGE_PAGE_PAYLOAD_SIZE * 3] = { 0 };

    ASSERT_EQ(SF::asyncFind(FIND_MODE_EMPTY, &address), STORAGE_OK);
    ASSERT_EQ(SF::asyncSave(address, shortPrefix, 1, wdata, sizeof(wdata)), STORAGE_OK);
    Page page(address + STORAGE_PAGE_SIZE);
    ASSERT_EQ(page.load(), STORAGE_OK);
    page.page.header.magic = 0;
    SF::storage.writePage(page.getAddress(), reinterpret_cast<uint8_t*>(&page.page), sizeof(page.page));

    ASSERT_EQ(SF::asyncFind(FIND_MODE_EQUAL, &address, shortPrefix, 1), STORAGE_OK);
    ASSERT_EQ(SF::asyncLoad(address, rdata, sizeof(rdata)), STORAGE_ERROR);
}

TEST_F(StorageFixtureAsync, FindPageAfterFormat)
{
    uint8_t wdata[STORAGE_PAGE_PAYLOAD_SIZE * 3] = { 1, 2, 3, 4, 5 };

    ASSERT_EQ(SF::asyncFind(FIND_MODE_EMPTY, &address), STORAGE_OK);
    ASSERT_EQ(SF::asyncSave(address, shortPrefix, 1, wdata, sizeof(wdata)), STORAGE_OK);
    ASSERT_EQ(SF::sat->format(), STORAGE_OK);
    ASSERT_EQ(SF::asyncFind(FIND_MODE_EQUAL, &address, shortPrefix, 1), STORAGE_NOT_FOUND);
}

TEST_F(StorageFixtureAsync, IsSetStatusesInHeader)
{
    Header header(0);

    ASSERT_EQ(SF::sat->format(), STORAGE_OK);
    ASSERT_EQ(header.load(), STORAGE_OK);

    for (unsigned i = 0; i < Header::PAGES_COUNT; i++) {
        uint32_t targetAddress = StorageMacroblock::getPageAddressByIndex(0, i);
        ASSERT_TRUE(header.isAddressEmpty(targetAddress));
    }
}

TEST_F(StorageFixtureAsync, SetStatusesInHeader)
{
    Header header(0);

    ASSERT_EQ(SF::sat->format(), STORAGE_OK);
    ASSERT_EQ(header.load(), STORAGE_OK);

    for (unsigned i = 0; i < Header::PAGES_COUNT; i++) {
        memcpy(header.data->metaUnits[i].prefix, shortPrefix, STORAGE_PAGE_PREFIX_SIZE);
        ASSERT_FALSE(header.isAddressEmpty((i + StorageMacroblock::RESERVED_PAGES_COUNT) * STORAGE_PAGE_SIZE));
    }
}

TEST_F(StorageFixtureAsync, SetBlockStatusesInHeader)
{
    Header header(0);

    ASSERT_EQ(SF::sat->format(), STORAGE_OK);
    ASSERT_EQ(header.load(), STORAGE_OK);
    
    for (unsigned i = 0; i < Header::PAGES_COUNT; i++) {
        uint32_t targetAddress = StorageMacroblock::getPageAddressByIndex(0, i);
        header.setAddressBlocked(targetAddress);
        ASSERT_TRUE(header.isAddressBlocked(targetAddress));
    }
}

TEST_F(StorageFixtureAsync, AutomaticHeadersCreation)
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

    ASSERT_EQ(SF::asyncFind(FIND_MODE_EQUAL, &address, shortPrefix, 1), STORAGE_OK);
    ASSERT_EQ(address, StorageMacroblock::RESERVED_PAGES_COUNT * STORAGE_PAGE_SIZE);
}

TEST_F(StorageFixtureAsync, HeaderSameMeta)
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

    ASSERT_EQ(SF::asyncFind(FIND_MODE_EQUAL, &address, shortPrefix, 1), STORAGE_OK);
    ASSERT_EQ(header.load(), STORAGE_OK);
    ASSERT_TRUE(header.isSameMeta(StorageMacroblock::getPageIndexByAddress(page.getAddress()), reinterpret_cast<const uint8_t*>(shortPrefix), 1));
}

TEST_F(StorageFixtureAsync, RewritePageWithSameData)
{
    uint8_t wdata1[STORAGE_PAGE_PAYLOAD_SIZE] = { 1, 2, 3, 4, 5 };
    uint8_t wdata2[STORAGE_PAGE_PAYLOAD_SIZE] = { 6, 7, 8, 9, 10 };
    uint8_t rdata1[STORAGE_PAGE_PAYLOAD_SIZE] = { 0 };
    uint8_t rdata2[STORAGE_PAGE_PAYLOAD_SIZE] = { 0 };

    ASSERT_EQ(SF::asyncFind(FIND_MODE_EMPTY, &address), STORAGE_OK);
    ASSERT_EQ(SF::asyncSave(address, shortPrefix, 1, wdata1, sizeof(wdata1)), STORAGE_OK);
    ASSERT_EQ(SF::asyncLoad(address, rdata1, sizeof(rdata1)), STORAGE_OK);
    ASSERT_FALSE(memcmp(wdata1, rdata1, sizeof(wdata1)));
    ASSERT_EQ(SF::asyncRewrite(address, shortPrefix, 1, wdata2, sizeof(wdata2)), STORAGE_OK);
    ASSERT_EQ(SF::asyncLoad(address, rdata2, sizeof(rdata2)), STORAGE_OK);
    ASSERT_FALSE(memcmp(wdata2, rdata2, sizeof(wdata2)));
}

TEST_F(StorageFixtureAsync, RewriteSaveDataByAnotherData)
{
    uint8_t wdata1[STORAGE_PAGE_PAYLOAD_SIZE] = { 1, 2, 3, 4, 5 };
    uint8_t wdata2[STORAGE_PAGE_PAYLOAD_SIZE] = { 6, 7, 8, 9, 10 };
    uint8_t rdata1[STORAGE_PAGE_PAYLOAD_SIZE] = { 0 };
    uint8_t rdata2[STORAGE_PAGE_PAYLOAD_SIZE] = { 0 };

    ASSERT_EQ(SF::asyncFind(FIND_MODE_EMPTY, &address), STORAGE_OK);
    ASSERT_EQ(SF::asyncSave(address, shortPrefix, 1, wdata1, sizeof(wdata1)), STORAGE_OK);
    ASSERT_EQ(SF::asyncLoad(address, rdata1, sizeof(rdata1)), STORAGE_OK);
    ASSERT_FALSE(memcmp(wdata1, rdata1, sizeof(wdata1)));
    ASSERT_EQ(SF::asyncRewrite(address, shortPrefix, 2, wdata2, sizeof(wdata2)), STORAGE_OK);
    ASSERT_EQ(SF::asyncLoad(address, rdata2, sizeof(rdata2)), STORAGE_OK);
    ASSERT_FALSE(memcmp(wdata2, rdata2, sizeof(wdata2)));
}

TEST_F(StorageFixtureAsync, WriteAllStorageBytes)
{
    uint32_t storageSize = StorageAT::getStorageSize();

    ASSERT_EQ(SF::asyncFind(FIND_MODE_EMPTY, &address), STORAGE_OK);
    ASSERT_EQ(SF::asyncSave(address, shortPrefix, 0, (new uint8_t[storageSize]), storageSize), STORAGE_OOM);
}

TEST_F(StorageFixtureAsync, FindAllData)
{
    uint8_t wdata[STORAGE_PAGE_PAYLOAD_SIZE] = { 1, 2, 3, 4, 5 };
    uint8_t rdata[STORAGE_PAGE_PAYLOAD_SIZE] = { 0 };

    status = STORAGE_OK;
    uint32_t pagesCount = 0;
    while (status == STORAGE_OK) {
        address = 0;
        status = SF::asyncFind(FIND_MODE_EMPTY, &address);
        if (status != STORAGE_OK) {
            break;
        }
        status = SF::asyncSave(address, shortPrefix, pagesCount + 1, wdata, sizeof(wdata));
        ASSERT_EQ(status, STORAGE_OK);
        memset(rdata, 0, sizeof(rdata));
        status = SF::asyncLoad(address, rdata, sizeof(rdata));
        ASSERT_EQ(status, STORAGE_OK);
        ASSERT_FALSE(memcmp(wdata, rdata, sizeof(wdata)));

        pagesCount++;
    }

    ASSERT_EQ(pagesCount, StorageAT::getPayloadPagesCount());
    for (unsigned i = 0; i < pagesCount; i++) {\
        ASSERT_EQ(SF::asyncFind(FIND_MODE_EQUAL, &address, shortPrefix, i + 1), STORAGE_OK);
    }

    ASSERT_EQ(SF::asyncFind(FIND_MODE_MIN, &address, shortPrefix), STORAGE_OK);
    ASSERT_EQ(address, STORAGE_PAGE_SIZE * StorageMacroblock::RESERVED_PAGES_COUNT);
    ASSERT_EQ(SF::asyncFind(FIND_MODE_MAX, &address, shortPrefix), STORAGE_OK);
    ASSERT_EQ(address, STORAGE_PAGE_SIZE * (StorageAT::getStoragePagesCount() - 1));
    ASSERT_EQ(SF::asyncFind(FIND_MODE_NEXT, &address, shortPrefix, 1), STORAGE_OK);
    ASSERT_EQ(address, STORAGE_PAGE_SIZE * (StorageMacroblock::RESERVED_PAGES_COUNT + 1));
}

TEST_F(StorageFixtureAsync, SaveDataWithMaxSize) {
    uint32_t maxSize = StorageAT::getPayloadSize();
    uint8_t* wdata = new uint8_t[maxSize];
    uint8_t* rdata = new uint8_t[maxSize];
    memset(wdata, 0xAA, maxSize);
    memset(rdata, 0,    maxSize);

    ASSERT_EQ(SF::sat->format(), STORAGE_OK);
    ASSERT_EQ(SF::asyncFind(FIND_MODE_EMPTY, &address), STORAGE_OK);
    ASSERT_EQ(SF::asyncSave(address, shortPrefix, 1, wdata, maxSize), STORAGE_OK);
    ASSERT_EQ(SF::asyncLoad(address, rdata, maxSize), STORAGE_OK);
    ASSERT_FALSE(memcmp(wdata, rdata, maxSize));

    delete[] wdata;
    delete[] rdata;
}

TEST_F(StorageFixtureAsync, DeleteData)
{
    uint8_t wdata[STORAGE_PAGE_PAYLOAD_SIZE] = { 1, 2, 3, 4, 5 };

    ASSERT_EQ(SF::asyncFind(FIND_MODE_EMPTY, &address), STORAGE_OK);
    ASSERT_EQ(SF::asyncSave(address, shortPrefix, 0, wdata, sizeof(wdata)), STORAGE_OK);
    ASSERT_EQ(SF::asyncFind(FIND_MODE_EQUAL, &address, shortPrefix, 0), STORAGE_OK);
    ASSERT_EQ(SF::sat->clearAddress(address), STORAGE_OK);
    ASSERT_EQ(SF::asyncFind(FIND_MODE_EQUAL, &address, shortPrefix, 0), STORAGE_NOT_FOUND);
}

TEST_F(StorageFixtureAsync, LoadDataWithInvalidAddress) {
    uint8_t rdata[STORAGE_PAGE_PAYLOAD_SIZE] = { 0 };

    // Некорректный адрес (не кратный размеру страницы)
    address = 1;
    ASSERT_EQ(SF::asyncLoad(address, rdata, sizeof(rdata)), STORAGE_ERROR);

    // Адрес за пределами памяти
    address = StorageAT::getStorageSize() + STORAGE_PAGE_SIZE;
    ASSERT_EQ(SF::asyncLoad(address, rdata, sizeof(rdata)), STORAGE_OOM);
}

TEST_F(StorageFixtureAsync, SaveDataWithInvalidAddress) {
    uint8_t wdata[STORAGE_PAGE_PAYLOAD_SIZE] = { 1, 2, 3, 4, 5 };

    // Некорректный адрес (не кратный размеру страницы)
    address = 1;
    ASSERT_EQ(SF::asyncSave(address, shortPrefix, 1, wdata, sizeof(wdata)), STORAGE_ERROR);

    // Адрес за пределами памяти
    address = StorageAT::getStorageSize() + STORAGE_PAGE_SIZE;
    ASSERT_EQ(SF::asyncSave(address, shortPrefix, 1, wdata, sizeof(wdata)), STORAGE_OOM);
}

TEST_F(StorageFixtureAsync, LoadEmptyPage)
{
    uint8_t data[STORAGE_PAGE_PAYLOAD_SIZE] = {};

    ASSERT_EQ(SF::asyncLoad(0, data, 0), STORAGE_ERROR);
    ASSERT_EQ(SF::asyncLoad(0, data, sizeof(data)), STORAGE_ERROR);
}

TEST_F(StorageFixtureAsync, SaveSimplePageInSectorHeader)
{
    uint8_t data[STORAGE_PAGE_PAYLOAD_SIZE] = {};

    ASSERT_EQ(SF::asyncSave(0, shortPrefix, 1, data, sizeof(data)), STORAGE_ERROR);
    ASSERT_EQ(SF::asyncLoad(0, data, sizeof(data)), STORAGE_ERROR);
}

TEST_F(StorageFixtureAsync, SavePageWithFindEmptyAddress)
{
    uint8_t wdata[STORAGE_PAGE_PAYLOAD_SIZE] = { 1, 2, 3, 4, 5 };
    uint8_t rdata[STORAGE_PAGE_PAYLOAD_SIZE] = {};

    ASSERT_EQ(SF::asyncFind(FIND_MODE_EMPTY, &address), STORAGE_OK);
    ASSERT_EQ(address, StorageMacroblock::RESERVED_PAGES_COUNT * STORAGE_PAGE_SIZE);
    ASSERT_EQ(SF::asyncSave(address, shortPrefix, 1, wdata, sizeof(wdata)), STORAGE_OK);
    ASSERT_EQ(SF::asyncLoad(address, rdata, sizeof(rdata)), STORAGE_OK);
    ASSERT_FALSE(memcmp(wdata, rdata, sizeof(wdata)));
}


TEST_F(StorageFixtureAsync, SavePageWithFindEmptyAddressWithOverwrite)
{
    uint8_t wdata[STORAGE_PAGE_PAYLOAD_SIZE] = { 1, 2, 3, 4, 5 };

    ASSERT_EQ(SF::asyncFind(FIND_MODE_EMPTY, &address), STORAGE_OK);
    ASSERT_EQ(SF::asyncSave(address, shortPrefix, 1, wdata, sizeof(wdata)), STORAGE_OK);
    ASSERT_EQ(SF::asyncSave(address, shortPrefix, 1, wdata, sizeof(wdata)), STORAGE_DATA_EXISTS);
    ASSERT_EQ(SF::asyncSave(address, shortPrefix, 2, wdata, sizeof(wdata)), STORAGE_DATA_EXISTS);
}

TEST_F(StorageFixtureAsync, SaveMultiPageInHeader)
{
    uint8_t data[STORAGE_PAGE_SIZE * 4] = {};

    ASSERT_EQ(SF::asyncSave(0, shortPrefix, 1, data, sizeof(data)), STORAGE_ERROR);
    ASSERT_EQ(SF::asyncLoad(0, data, sizeof(data)), STORAGE_ERROR);
}

TEST_F(StorageFixtureAsync, SaveMultiPageWithFindEmptyAddress)
{
    uint8_t wdata[STORAGE_PAGE_SIZE * 4] = { 1, 2, 3, 4, 5 };
    uint8_t rdata[STORAGE_PAGE_SIZE * 4] = {};

    ASSERT_EQ(SF::asyncFind(FIND_MODE_EMPTY, &address), STORAGE_OK);
    ASSERT_EQ(address, StorageMacroblock::RESERVED_PAGES_COUNT * STORAGE_PAGE_SIZE);
    ASSERT_EQ(SF::asyncSave(address, shortPrefix, 1, wdata, sizeof(wdata)), STORAGE_OK);
    ASSERT_EQ(SF::asyncLoad(address, rdata, sizeof(rdata)), STORAGE_OK);
    ASSERT_FALSE(memcmp(wdata, rdata, sizeof(wdata)));
}

TEST_F(StorageFixtureAsync, SaveMultiPageWithFindEmptyAddressWithOverwrite)
{
    uint8_t wdata[STORAGE_PAGE_SIZE * 4] = { 1, 2, 3, 4, 5 };

    ASSERT_EQ(SF::asyncFind(FIND_MODE_EMPTY, &address), STORAGE_OK);
    ASSERT_EQ(SF::asyncSave(address, shortPrefix, 1, wdata, sizeof(wdata)), STORAGE_OK);
    ASSERT_EQ(SF::asyncSave(address, shortPrefix, 1, wdata, sizeof(wdata)), STORAGE_DATA_EXISTS);
    ASSERT_EQ(SF::asyncSave(address, shortPrefix, 2, wdata, sizeof(wdata)), STORAGE_DATA_EXISTS);
}

TEST_F(StorageFixtureAsync, SaveDataInBusyPages)
{
    uint8_t wdata1[STORAGE_PAGE_SIZE * 4] = { 1, 2, 3, 4, 5 };
    uint8_t wdata2[STORAGE_PAGE_PAYLOAD_SIZE] = { 6, 7, 8, 9, 10 };
    uint8_t rdata1[STORAGE_PAGE_SIZE * 4] = {};

    ASSERT_EQ(SF::asyncFind(FIND_MODE_EMPTY, &address), STORAGE_OK);
    ASSERT_EQ(SF::asyncSave(address, shortPrefix, 1, wdata1, sizeof(wdata1)), STORAGE_OK);
    ASSERT_EQ(SF::asyncSave(address, shortPrefix, 2, wdata2, sizeof(wdata2)), STORAGE_DATA_EXISTS);
    ASSERT_EQ(SF::asyncSave(address + PAGE_LEN, shortPrefix, 2, wdata2, sizeof(wdata2)), STORAGE_DATA_EXISTS);
    ASSERT_EQ(SF::asyncLoad(address, rdata1, sizeof(rdata1)), STORAGE_OK);
    ASSERT_FALSE(memcmp(wdata1, rdata1, sizeof(wdata1)));
}

TEST_F(StorageFixtureAsync, LoadNotAlignedAddress)
{
    uint8_t rdata[STORAGE_PAGE_PAYLOAD_SIZE] = {};

    ASSERT_EQ(SF::asyncFind(FIND_MODE_EMPTY, &address), STORAGE_OK);
    ASSERT_EQ(SF::asyncLoad(address + 1, rdata, sizeof(rdata)), STORAGE_ERROR);
}

TEST_F(StorageFixtureAsync, SaveNotAlignedAddress)
{
    uint8_t wdata[STORAGE_PAGE_PAYLOAD_SIZE] = {};

    ASSERT_EQ(SF::asyncFind(FIND_MODE_EMPTY, &address), STORAGE_OK);
    ASSERT_EQ(SF::asyncSave(address + 1, shortPrefix, 1, wdata, sizeof(wdata)), STORAGE_ERROR);
}

TEST_F(StorageFixtureAsync, SaveDataOnBlockedPage)
{
    address = StorageMacroblock::RESERVED_PAGES_COUNT * STORAGE_PAGE_SIZE;
    uint8_t wdata[STORAGE_PAGE_PAYLOAD_SIZE] = { 1, 2, 3, 4, 5 };
    Header header(address);
    uint32_t tmpAddress = 0;

    SF::storage.writeBlock(address, true);

    ASSERT_EQ(SF::asyncSave(address, shortPrefix, 1, wdata, sizeof(wdata)), STORAGE_OK);
    ASSERT_EQ(StorageMacroblock::loadHeader(&header), STORAGE_OK);

    uint32_t pageIndex = StorageMacroblock::getPageIndexByAddress(address);
    ASSERT_TRUE(header.isAddressBlocked((pageIndex + StorageMacroblock:: RESERVED_PAGES_COUNT) * STORAGE_PAGE_SIZE));
    ASSERT_EQ(SF::asyncFind(FIND_MODE_EQUAL, &tmpAddress, shortPrefix, 1), STORAGE_OK);
    ASSERT_NE(address, tmpAddress);
    ASSERT_EQ(header.load(), STORAGE_OK);
    ASSERT_TRUE(header.isAddressBlocked((pageIndex + StorageMacroblock::RESERVED_PAGES_COUNT) * STORAGE_PAGE_SIZE));
}

TEST_F(StorageFixtureAsync, SaveDataOnBlockedSector)
{
    address = StorageMacroblock::RESERVED_PAGES_COUNT * STORAGE_PAGE_SIZE;
    uint8_t wdata[STORAGE_PAGE_PAYLOAD_SIZE] = {};
    uint8_t rdata[STORAGE_PAGE_PAYLOAD_SIZE] = {};
    Header header(address);
    uint32_t tmpAddress = 0;
    for (unsigned i = 0; i < sizeof(wdata); i++) {
        wdata[i] = (uint8_t)i;
    }

    for (unsigned i = 0; i < StorageMacroblock::PAGES_COUNT * STORAGE_PAGE_SIZE; i++) {
        SF::storage.writeBlock(i, true);
    }

    ASSERT_EQ(SF::asyncSave(address, shortPrefix, 1, wdata, sizeof(wdata)), STORAGE_OK);
    ASSERT_EQ(StorageMacroblock::loadHeader(&header), STORAGE_OK);

    for (unsigned i = 0; i < Header::PAGES_COUNT; i++) {
        ASSERT_FALSE(header.isAddressBlocked((i + StorageMacroblock::RESERVED_PAGES_COUNT) * STORAGE_PAGE_SIZE));
    }
    ASSERT_EQ(SF::asyncFind(FIND_MODE_EQUAL, &tmpAddress, shortPrefix, 1), STORAGE_OK);
    ASSERT_EQ(SF::asyncLoad(tmpAddress, rdata, sizeof(rdata)), STORAGE_OK);
    ASSERT_NE(address, tmpAddress); // 1024 11008
    ASSERT_EQ(header.load(), STORAGE_HEADER_ERROR);
}

TEST_F(StorageFixtureAsync, SaveDataOnPartiedBlockedSector1)
{
    address = StorageMacroblock::RESERVED_PAGES_COUNT * STORAGE_PAGE_SIZE;
    uint8_t wdata[STORAGE_PAGE_PAYLOAD_SIZE * 3] = {};
    uint8_t rdata[STORAGE_PAGE_PAYLOAD_SIZE * 3] = {};
    Header header(address);
    uint32_t tmpAddress = 0;
    for (unsigned i = 0; i < sizeof(wdata); i++) {
        wdata[i] = (uint8_t)i;
    }

    for (unsigned i = STORAGE_PAGE_SIZE; i < Header::PAGES_COUNT * STORAGE_PAGE_SIZE; i++) {
        SF::storage.writeBlock(address + i, true);
    }

    ASSERT_EQ(SF::asyncSave(address, shortPrefix, 1, wdata, sizeof(wdata)), STORAGE_OK);
    ASSERT_EQ(StorageMacroblock::loadHeader(&header), STORAGE_OK);

    ASSERT_FALSE(header.isAddressBlocked((StorageMacroblock::RESERVED_PAGES_COUNT) * STORAGE_PAGE_SIZE));
    for (unsigned i = 1; i < Header::PAGES_COUNT; i++) {
        ASSERT_TRUE(header.isAddressBlocked((i + StorageMacroblock::RESERVED_PAGES_COUNT) * STORAGE_PAGE_SIZE));
    }
    ASSERT_EQ(SF::asyncFind(FIND_MODE_EQUAL, &tmpAddress, shortPrefix, 1), STORAGE_OK);
    ASSERT_EQ(SF::asyncLoad(tmpAddress, rdata, sizeof(rdata)), STORAGE_OK);
    ASSERT_FALSE(memcmp(wdata, rdata, sizeof(wdata)));
    ASSERT_EQ(address, tmpAddress);
    ASSERT_EQ(address, StorageMacroblock::RESERVED_PAGES_COUNT * STORAGE_PAGE_SIZE);
}

TEST_F(StorageFixtureAsync, SaveDataOnPartiedBlockedSector2) // TODO: add to test.cpp
{
    address = StorageMacroblock::RESERVED_PAGES_COUNT * STORAGE_PAGE_SIZE;
    uint8_t wdata[STORAGE_PAGE_PAYLOAD_SIZE * 3] = {};
    uint8_t rdata[STORAGE_PAGE_PAYLOAD_SIZE * 3] = {};
    Header header(address);
    uint32_t tmpAddress = 0;
    for (unsigned i = 0; i < sizeof(wdata); i++) {
        wdata[i] = (uint8_t)i;
    }

    for (unsigned i = 0; i < STORAGE_PAGE_SIZE; i++) {
        SF::storage.writeBlock(address + i, true);
    }
    for (unsigned i = 2 * STORAGE_PAGE_SIZE; i < Header::PAGES_COUNT * STORAGE_PAGE_SIZE; i++) {
        SF::storage.writeBlock(address + i, true);
    }

    ASSERT_EQ(SF::asyncSave(address, shortPrefix, 1, wdata, sizeof(wdata)), STORAGE_OK);
    ASSERT_EQ(StorageMacroblock::loadHeader(&header), STORAGE_OK);

    ASSERT_TRUE(header.isAddressBlocked(StorageMacroblock::RESERVED_PAGES_COUNT * STORAGE_PAGE_SIZE));
    ASSERT_FALSE(header.isAddressBlocked(StorageMacroblock::RESERVED_PAGES_COUNT * STORAGE_PAGE_SIZE + STORAGE_PAGE_SIZE));
    for (unsigned i = 2; i < Header::PAGES_COUNT; i++) {
        ASSERT_TRUE(header.isAddressBlocked((i + StorageMacroblock::RESERVED_PAGES_COUNT) * STORAGE_PAGE_SIZE));
    }
    ASSERT_EQ(SF::asyncFind(FIND_MODE_EQUAL, &tmpAddress, shortPrefix, 1), STORAGE_OK);
    ASSERT_EQ(SF::asyncLoad(tmpAddress, rdata, sizeof(rdata)), STORAGE_OK);
    ASSERT_FALSE(memcmp(wdata, rdata, sizeof(wdata)));
    ASSERT_NE(address, tmpAddress);
    ASSERT_EQ(address, StorageMacroblock::RESERVED_PAGES_COUNT * STORAGE_PAGE_SIZE);
    ASSERT_EQ(tmpAddress, (StorageMacroblock::RESERVED_PAGES_COUNT + 1) * STORAGE_PAGE_SIZE);
}

TEST_F(StorageFixtureAsync, SaveDataOnPartiedBlockedSector3) // TODO: add to test.cpp
{
    address = StorageMacroblock::RESERVED_PAGES_COUNT * STORAGE_PAGE_SIZE;
    uint8_t wdata[STORAGE_PAGE_PAYLOAD_SIZE * 3] = {};
    uint8_t rdata[STORAGE_PAGE_PAYLOAD_SIZE * 3] = {};
    Header header(address);
    uint32_t tmpAddress = 0;
    for (unsigned i = 0; i < sizeof(wdata); i++) {
        wdata[i] = (uint8_t)i;
    }

    for (unsigned i = STORAGE_PAGE_SIZE; i < (Header::PAGES_COUNT - 1) * STORAGE_PAGE_SIZE; i++) {
        SF::storage.writeBlock(address + i, true);
    }

    ASSERT_EQ(SF::asyncSave(address, shortPrefix, 1, wdata, sizeof(wdata)), STORAGE_OK);
    ASSERT_EQ(StorageMacroblock::loadHeader(&header), STORAGE_OK);

    ASSERT_FALSE(header.isAddressBlocked((StorageMacroblock::RESERVED_PAGES_COUNT) * STORAGE_PAGE_SIZE));
    ASSERT_FALSE(header.isAddressBlocked((StorageMacroblock::PAGES_COUNT - 1) * STORAGE_PAGE_SIZE));
    for (unsigned i = 1; i < Header::PAGES_COUNT - 1; i++) {
        ASSERT_TRUE(header.isAddressBlocked((i + StorageMacroblock::RESERVED_PAGES_COUNT) * STORAGE_PAGE_SIZE));
    }
    ASSERT_EQ(SF::asyncFind(FIND_MODE_EQUAL, &tmpAddress, shortPrefix, 1), STORAGE_OK);
    ASSERT_EQ(SF::asyncLoad(tmpAddress, rdata, sizeof(rdata)), STORAGE_OK);
    ASSERT_FALSE(memcmp(wdata, rdata, sizeof(wdata)));
    ASSERT_EQ(address, tmpAddress);
    ASSERT_EQ(address, StorageMacroblock::RESERVED_PAGES_COUNT * STORAGE_PAGE_SIZE);
}

TEST_F(StorageFixtureAsync, SaveDataOnPartiedBlockedSector4) // TODO: add to test.cpp
{
    address = StorageMacroblock::RESERVED_PAGES_COUNT * STORAGE_PAGE_SIZE;
    uint8_t wdata[STORAGE_PAGE_PAYLOAD_SIZE * 3] = {};
    uint8_t rdata[STORAGE_PAGE_PAYLOAD_SIZE * 3] = {};
    Header header(address);
    uint32_t tmpAddress = 0;
    for (unsigned i = 0; i < sizeof(wdata); i++) {
        wdata[i] = (uint8_t)i;
    }

    for (unsigned i = 0; i < StorageMacroblock::PAGES_COUNT * STORAGE_PAGE_SIZE; i++) {
        SF::storage.writeBlock(address + i, true);
    }

    ASSERT_EQ(SF::asyncSave(address, shortPrefix, 1, wdata, sizeof(wdata)), STORAGE_OK);
    ASSERT_EQ(StorageMacroblock::loadHeader(&header), STORAGE_OK);

    for (unsigned i = 0; i < Header::PAGES_COUNT; i++) {
        ASSERT_TRUE(header.isAddressBlocked((i + StorageMacroblock::RESERVED_PAGES_COUNT) * STORAGE_PAGE_SIZE));
    }
    ASSERT_EQ(SF::asyncFind(FIND_MODE_EQUAL, &tmpAddress, shortPrefix, 1), STORAGE_OK);
    ASSERT_EQ(SF::asyncLoad(tmpAddress, rdata, sizeof(rdata)), STORAGE_OK);
    ASSERT_FALSE(memcmp(wdata, rdata, sizeof(wdata)));
    ASSERT_NE(address, tmpAddress);
    ASSERT_EQ(address, StorageMacroblock::RESERVED_PAGES_COUNT * STORAGE_PAGE_SIZE);
    ASSERT_EQ(tmpAddress, (StorageMacroblock::PAGES_COUNT + StorageMacroblock::RESERVED_PAGES_COUNT) * STORAGE_PAGE_SIZE);
}

TEST_F(StorageFixtureAsync, SaveDataOnPartiedBlockedSector5) // TODO: add to test.cpp
{
    address = StorageMacroblock::RESERVED_PAGES_COUNT * STORAGE_PAGE_SIZE;
    uint8_t wdata[STORAGE_PAGE_PAYLOAD_SIZE * 3] = {};
    uint8_t rdata[STORAGE_PAGE_PAYLOAD_SIZE * 3] = {};
    Header header(address);
    uint32_t tmpAddress = 0;
    for (unsigned i = 0; i < sizeof(wdata); i++) {
        wdata[i] = (uint8_t)i;
    }

    for (unsigned i = 0; i < StorageMacroblock::PAGES_COUNT * STORAGE_PAGE_SIZE; i++) {
        SF::storage.writeBlock(i, true);
    }

    ASSERT_EQ(SF::asyncSave(address, shortPrefix, 1, wdata, sizeof(wdata)), STORAGE_OK);
    ASSERT_EQ(StorageMacroblock::loadHeader(&header), STORAGE_OK);

    for (unsigned i = 0; i < Header::PAGES_COUNT; i++) {
        ASSERT_FALSE(header.isAddressBlocked((i + StorageMacroblock::RESERVED_PAGES_COUNT) * STORAGE_PAGE_SIZE));
    }
    ASSERT_EQ(SF::asyncFind(FIND_MODE_EQUAL, &tmpAddress, shortPrefix, 1), STORAGE_OK);
    ASSERT_EQ(SF::asyncLoad(tmpAddress, rdata, sizeof(rdata)), STORAGE_OK);
    ASSERT_FALSE(memcmp(wdata, rdata, sizeof(wdata)));
    ASSERT_NE(address, tmpAddress);
    ASSERT_EQ(address, StorageMacroblock::RESERVED_PAGES_COUNT * STORAGE_PAGE_SIZE);
    ASSERT_EQ(tmpAddress, (StorageMacroblock::PAGES_COUNT + StorageMacroblock::RESERVED_PAGES_COUNT) * STORAGE_PAGE_SIZE);
}

TEST_F(StorageFixtureAsync, BlockAllMemory)
{
    uint8_t wdata[STORAGE_PAGE_PAYLOAD_SIZE] = { 1, 2, 3, 4, 5 };
    address = StorageMacroblock::RESERVED_PAGES_COUNT * STORAGE_PAGE_SIZE;
    
    for (unsigned i = 0; i < SF::storage.getSize(); i++) {
        SF::storage.writeBlock(i, true);
    }

    ASSERT_EQ(SF::asyncSave(address, shortPrefix, 1, wdata, sizeof(wdata)), STORAGE_OOM);
    ASSERT_EQ(SF::asyncFind(FIND_MODE_EQUAL, &address, shortPrefix, 1), STORAGE_NOT_FOUND);
}

TEST_F(StorageFixtureAsync, SetHeaderBlocked)
{
    address = 0;
    Header header(address);

    SF::storage.writeBlock(address, true);

    ASSERT_EQ(StorageMacroblock::loadHeader(&header), STORAGE_OK);
    ASSERT_NE(header.getAddress(), address);
}

TEST_F(StorageFixtureAsync, SetAllHeadersBlocked)
{
    Header header(0);

    for (unsigned i = 0; i < StorageMacroblock::RESERVED_PAGES_COUNT; i++) {
        SF::storage.writeBlock(address + STORAGE_PAGE_SIZE * i, true);
    }

    ASSERT_EQ(StorageMacroblock::loadHeader(&header), STORAGE_OK);
}

TEST_F(StorageFixtureAsync, SaveAndFindDataWithBlockedAllHeaders)
{
    uint8_t wdata[STORAGE_PAGE_PAYLOAD_SIZE] = { 1, 2, 3, 4, 5 };
    uint8_t rdata[STORAGE_PAGE_PAYLOAD_SIZE] = { 0 };

    for (unsigned i = 0; i < StorageMacroblock::RESERVED_PAGES_COUNT * STORAGE_PAGE_SIZE; i++) {
        SF::storage.writeBlock(i, true);
    }

    ASSERT_EQ(SF::asyncFind(FIND_MODE_EMPTY, &address), STORAGE_OK);
    ASSERT_EQ(address, StorageMacroblock::RESERVED_PAGES_COUNT * STORAGE_PAGE_SIZE);
    ASSERT_EQ(SF::asyncSave(address, shortPrefix, 1, wdata, sizeof(wdata)), STORAGE_OK);
    ASSERT_EQ(SF::asyncFind(FIND_MODE_EQUAL, &address, shortPrefix, 1), STORAGE_OK);
    ASSERT_EQ(SF::asyncLoad(address, rdata, sizeof(rdata)), STORAGE_OK);
    ASSERT_FALSE(memcmp(wdata, rdata, sizeof(wdata)));
}

TEST_F(StorageFixtureAsync, FindEqual)
{
    uint8_t wdata[STORAGE_PAGE_PAYLOAD_SIZE] = { 1, 2, 3, 4, 5 };
    uint8_t rdata[STORAGE_PAGE_PAYLOAD_SIZE] = {};
    uint32_t emptyAddress = 0;

    ASSERT_EQ(SF::asyncFind(FIND_MODE_EMPTY, &emptyAddress), STORAGE_OK);
    ASSERT_EQ(SF::asyncSave(emptyAddress, shortPrefix, 1, wdata, sizeof(wdata)), STORAGE_OK);

    ASSERT_EQ(SF::asyncFind(FIND_MODE_EQUAL, &address, shortPrefix, 1), STORAGE_OK);
    ASSERT_EQ(address, emptyAddress);
    ASSERT_EQ(SF::asyncLoad(address, rdata, sizeof(rdata)), STORAGE_OK);
    ASSERT_FALSE(memcmp(wdata, rdata, sizeof(wdata)));
}

TEST_F(StorageFixtureAsync, SaveFindLoadPartitionedData)
{
    uint32_t nextAddress = 0;
    uint8_t wdata1[STORAGE_PAGE_PAYLOAD_SIZE] = {};
    uint8_t wdata2[STORAGE_PAGE_SIZE * 4] = {};
    uint8_t rdata1[STORAGE_PAGE_PAYLOAD_SIZE] = {};
    uint8_t rdata2[STORAGE_PAGE_SIZE * 4] = {};


    memset(wdata1, 0xF0, sizeof(wdata1));
    memset(wdata2, 0xF0, sizeof(wdata2));

    ASSERT_EQ(SF::asyncFind(FIND_MODE_EMPTY, &address), STORAGE_OK);
    nextAddress = address + STORAGE_PAGE_SIZE;
    ASSERT_EQ(SF::asyncSave(nextAddress, shortPrefix, 1, wdata1, sizeof(wdata1)), STORAGE_OK);
    ASSERT_EQ(SF::asyncSave(address, shortPrefix, 2, wdata2, sizeof(wdata2)), STORAGE_OK);
    address = 0;
    ASSERT_EQ(SF::asyncFind(FIND_MODE_EQUAL, &address, shortPrefix, 1), STORAGE_OK);
    ASSERT_EQ(SF::asyncLoad(address, rdata1, sizeof(rdata1)), STORAGE_OK);
    ASSERT_FALSE(memcmp(wdata1, rdata1, sizeof(wdata1)));
    address = 0;
    ASSERT_EQ(SF::asyncFind(FIND_MODE_EQUAL, &address, shortPrefix, 2), STORAGE_OK);
    ASSERT_EQ(SF::asyncLoad(address, rdata2, sizeof(rdata2)), STORAGE_OK);
    ASSERT_FALSE(memcmp(wdata2, rdata2, sizeof(wdata2)));
}

TEST_F(StorageFixtureAsync, FindForAnyAddress)
{
    uint8_t wdata[STORAGE_PAGE_PAYLOAD_SIZE] = {};

    ASSERT_EQ(SF::asyncFind(FIND_MODE_EMPTY, &address), STORAGE_OK);
    ASSERT_EQ(address, StorageMacroblock::RESERVED_PAGES_COUNT * STORAGE_PAGE_SIZE);
    ASSERT_EQ(SF::asyncSave(address, shortPrefix, 1, wdata, sizeof(wdata)), STORAGE_OK);
    uint32_t lastAddress = address;
    ASSERT_EQ(SF::asyncFind(FIND_MODE_NEXT, &address, "", 0), STORAGE_OK);
    ASSERT_EQ(lastAddress, address);
}

TEST_F(StorageFixtureAsync, DeleteDataWithBlockedHeader)
{
    uint8_t wdata[STORAGE_PAGE_PAYLOAD_SIZE] = { 1, 2, 3, 4, 5 };

    for (unsigned i = 0; i < StorageMacroblock::RESERVED_PAGES_COUNT * STORAGE_PAGE_SIZE; i++) {
        SF::storage.writeBlock(i, true);
    }

    ASSERT_EQ(SF::asyncFind(FIND_MODE_EMPTY, &address), STORAGE_OK);
    ASSERT_EQ(address, StorageMacroblock::RESERVED_PAGES_COUNT * STORAGE_PAGE_SIZE);
    ASSERT_EQ(SF::asyncSave(address, shortPrefix, 1, wdata, sizeof(wdata)), STORAGE_OK);
    ASSERT_EQ(SF::sat->deleteData(shortPrefix, 1), STORAGE_OK);
    ASSERT_EQ(SF::asyncFind(FIND_MODE_EQUAL, &address, shortPrefix, 1), STORAGE_NOT_FOUND);
}

TEST_F(StorageFixtureAsync, DeleteDataWithBlockedPage)
{
    uint8_t wdata[STORAGE_PAGE_PAYLOAD_SIZE] = { 1, 2, 3, 4, 5 };

    ASSERT_EQ(SF::asyncFind(FIND_MODE_EMPTY, &address), STORAGE_OK);
    ASSERT_EQ(address, StorageMacroblock::RESERVED_PAGES_COUNT * STORAGE_PAGE_SIZE);
    ASSERT_EQ(SF::asyncSave(address, shortPrefix, 1, wdata, sizeof(wdata)), STORAGE_OK);

    for (unsigned i = 0; i < StorageMacroblock::RESERVED_PAGES_COUNT * STORAGE_PAGE_SIZE; i++) {
        SF::storage.writeBlock(StorageMacroblock::RESERVED_PAGES_COUNT * STORAGE_PAGE_SIZE + i, true);
    }

    ASSERT_EQ(SF::sat->deleteData(shortPrefix, 1), STORAGE_OK);
    ASSERT_EQ(SF::asyncFind(FIND_MODE_EQUAL, &address, shortPrefix, 1), STORAGE_NOT_FOUND);
}

TEST_F(StorageFixtureAsync, TimeCheck)
{
    uint8_t data[STORAGE_PAGE_PAYLOAD_SIZE] = { 0 };

    auto startTime = std::chrono::high_resolution_clock::now();
    SF::sat->format();
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
    std::cout << "Format: " << (double)(duration.count() / 1000.0) << "ms" << std::endl;

    startTime = std::chrono::high_resolution_clock::now();
    SF::asyncSave(
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
    SF::asyncFind(
        FIND_MODE_EQUAL,
        &address,
        shortPrefix,
        1
    );
    endTime = std::chrono::high_resolution_clock::now();
    std::cout << "Find:   " << (double)(duration.count() / 1000.0) << "ms" << std::endl;

    startTime = std::chrono::high_resolution_clock::now();
    SF::asyncLoad(address, data, sizeof(data));
    endTime = std::chrono::high_resolution_clock::now();
    std::cout << "Load:   " << (double)(duration.count() / 1000.0) << "ms" << std::endl;

    startTime = std::chrono::high_resolution_clock::now();
    SF::sat->clearAddress(address);
    endTime = std::chrono::high_resolution_clock::now();
    std::cout << "Delete: " << (double)(duration.count() / 1000.0) << "ms" << std::endl;
}

TEST_F(StorageFixtureAsync, ChangePagesCount)
{
    uint8_t wdata[STORAGE_PAGE_PAYLOAD_SIZE] = { 1, 2, 3, 4, 5 };
    uint8_t rdata[STORAGE_PAGE_PAYLOAD_SIZE] = { 0 };
    address = StorageMacroblock::RESERVED_PAGES_COUNT * STORAGE_PAGE_SIZE;
    ASSERT_EQ(SF::asyncSave(address, shortPrefix, 1, wdata, sizeof(wdata)), STORAGE_OK);
    ASSERT_EQ(SF::asyncLoad(address, rdata, sizeof(rdata)), STORAGE_OK);
    ASSERT_FALSE(memcmp(wdata, rdata, sizeof(wdata)));

    SF::storage.clear();
    SF::sat->setPagesCount(0);
    ASSERT_EQ(SF::asyncSave(address, shortPrefix, 1, wdata, sizeof(wdata)), STORAGE_OOM);
    ASSERT_EQ(SF::asyncLoad(address, rdata, sizeof(rdata)), STORAGE_OOM);

    SF::sat->setPagesCount(StorageMacroblock::RESERVED_PAGES_COUNT + 1);
    memset(rdata, 0, sizeof(rdata));
    ASSERT_EQ(SF::asyncSave(address, shortPrefix, 1, wdata, sizeof(wdata)), STORAGE_OK);
    ASSERT_EQ(SF::asyncLoad(address, rdata, sizeof(rdata)), STORAGE_OK);
    ASSERT_FALSE(memcmp(wdata, rdata, sizeof(wdata)));

    address += STORAGE_PAGE_SIZE;
    ASSERT_EQ(SF::asyncSave(address, shortPrefix, 1, wdata, sizeof(wdata)), STORAGE_OOM);
    ASSERT_EQ(SF::asyncLoad(address, rdata, sizeof(rdata)), STORAGE_OOM);
}

TEST_F(StorageFixtureAsync, DeleteExistingData) {
    uint8_t wdata[STORAGE_PAGE_PAYLOAD_SIZE] = { 1, 2, 3, 4, 5 };

    // Сохраняем данные
    ASSERT_EQ(SF::asyncFind(FIND_MODE_EMPTY, &address), STORAGE_OK);
    ASSERT_EQ(SF::asyncSave(address, shortPrefix, 1, wdata, sizeof(wdata)), STORAGE_OK);

    // Удаляем данные
    ASSERT_EQ(SF::sat->deleteData(shortPrefix, 1), STORAGE_OK);

    // Проверяем, что данные удалены
    ASSERT_EQ(SF::asyncFind(FIND_MODE_EQUAL, &address, shortPrefix, 1), STORAGE_NOT_FOUND);
}

TEST_F(StorageFixtureAsync, DeleteNonExistingData) {
    // Попытка удаления несуществующих данных
    ASSERT_EQ(SF::sat->deleteData(shortPrefix, 1), STORAGE_OK); // Должно вернуть OK, даже если данных нет
}

TEST_F(StorageFixtureAsync, DeleteDataWithInvalidHeader) {
    uint8_t wdata[STORAGE_PAGE_PAYLOAD_SIZE] = { 1, 2, 3, 4, 5 };

    // Портим заголовок
    Header header(StorageMacroblock::getMacroblockAddress(0));
    for (unsigned i = 0; i < StorageMacroblock::RESERVED_PAGES_COUNT * STORAGE_PAGE_SIZE; i++) {
        SF::storage.writeBlock(i, true);
    }
    ASSERT_EQ(header.save(), STORAGE_HEADER_ERROR);
    ASSERT_EQ(header.create(), STORAGE_OK);

    // Сохраняем данные
    ASSERT_EQ(SF::asyncFind(FIND_MODE_EMPTY, &address), STORAGE_OK);
    ASSERT_EQ(SF::asyncSave(address, shortPrefix, 1, wdata, sizeof(wdata)), STORAGE_OK);

    // Удаляем данные, несмотря на ошибку в заголовке
    ASSERT_EQ(SF::sat->deleteData(shortPrefix, 1), STORAGE_OK);

    // Проверяем, что данные удалены
    ASSERT_EQ(SF::asyncFind(FIND_MODE_EQUAL, &address, shortPrefix, 1), STORAGE_NOT_FOUND);
}
TEST_F(StorageFixtureAsync, ClearExistingAddress) {
    uint8_t wdata[STORAGE_PAGE_PAYLOAD_SIZE] = { 1, 2, 3, 4, 5 };

    // Сохраняем данные
    ASSERT_EQ(SF::asyncFind(FIND_MODE_EMPTY, &address), STORAGE_OK);
    ASSERT_EQ(SF::asyncSave(address, shortPrefix, 1, wdata, sizeof(wdata)), STORAGE_OK);

    // Очищаем адрес
    ASSERT_EQ(SF::sat->clearAddress(address), STORAGE_OK);

    // Проверяем, что данные удалены
    ASSERT_EQ(SF::asyncFind(FIND_MODE_EQUAL, &address, shortPrefix, 1), STORAGE_NOT_FOUND);
}

TEST_F(StorageFixtureAsync, ClearNonExistingAddress) {
    // Попытка очистки несуществующего адреса
    uint32_t invalidAddress = StorageAT::getStorageSize() + STORAGE_PAGE_SIZE; // Адрес за пределами памяти
    ASSERT_EQ(SF::sat->clearAddress(invalidAddress), STORAGE_ERROR); // Должно вернуть ошибку
}

TEST_F(StorageFixtureAsync, ClearAddressWithInvalidHeader) {
    uint8_t wdata[STORAGE_PAGE_PAYLOAD_SIZE] = { 1, 2, 3, 4, 5 };

    // Портим заголовок
    Header header(StorageMacroblock::getMacroblockAddress(0));
    for (unsigned i = 0; i < StorageMacroblock::RESERVED_PAGES_COUNT * STORAGE_PAGE_SIZE; i++) {
        SF::storage.writeBlock(i, true);
    }
    ASSERT_EQ(header.save(), STORAGE_HEADER_ERROR);
    ASSERT_EQ(header.create(), STORAGE_OK);

    // Сохраняем данные
    ASSERT_EQ(SF::asyncFind(FIND_MODE_EMPTY, &address), STORAGE_OK);
    ASSERT_EQ(SF::asyncSave(address, shortPrefix, 1, wdata, sizeof(wdata)), STORAGE_OK);

    // Очищаем адрес, несмотря на ошибку в заголовке
    ASSERT_EQ(SF::sat->clearAddress(address), STORAGE_OK);

    // Проверяем, что данные удалены
    ASSERT_EQ(SF::asyncFind(FIND_MODE_EQUAL, &address, shortPrefix, 1), STORAGE_NOT_FOUND);
}

TEST_F(StorageFixtureAsync, UnknownBrokenPage1) {
    uint8_t wdata[4 * STORAGE_PAGE_PAYLOAD_SIZE] = { 1, 2, 3, 4, 5 };
    uint8_t rdata[sizeof(wdata)] = {};

    for (unsigned i = 0; i < STORAGE_PAGE_SIZE; i++) {
        SF::storage.writeBlock(StorageMacroblock::RESERVED_PAGES_COUNT * STORAGE_PAGE_SIZE + i, true);
    }
    
    // Сохраняем данные
    ASSERT_EQ(SF::asyncFind(FIND_MODE_EMPTY, &address), STORAGE_OK);
    ASSERT_EQ(SF::asyncSave(address, shortPrefix, 1, wdata, sizeof(wdata)), STORAGE_OK);

    // Сохраняем данные
    ASSERT_EQ(SF::asyncFind(FIND_MODE_EQUAL, &address, shortPrefix, 1), STORAGE_OK);
    ASSERT_EQ(SF::asyncLoad(address, rdata, sizeof(rdata)), STORAGE_OK);

    ASSERT_FALSE(memcmp(wdata, rdata, sizeof(rdata)));
}

TEST_F(StorageFixtureAsync, UnknownBrokenPage2) {
    uint8_t wdata[4 * STORAGE_PAGE_PAYLOAD_SIZE] = { 1, 2, 3, 4, 5 };
    uint8_t rdata[sizeof(wdata)] = {};

    for (unsigned i = 0; i < STORAGE_PAGE_SIZE; i++) {
        SF::storage.writeBlock((1 + StorageMacroblock::RESERVED_PAGES_COUNT) * STORAGE_PAGE_SIZE + i, true);
    }
    
    // Сохраняем данные
    ASSERT_EQ(SF::asyncFind(FIND_MODE_EMPTY, &address), STORAGE_OK);
    ASSERT_EQ(SF::asyncSave(address, shortPrefix, 1, wdata, sizeof(wdata)), STORAGE_OK);

    // Сохраняем данные
    ASSERT_EQ(SF::asyncFind(FIND_MODE_EQUAL, &address, shortPrefix, 1), STORAGE_OK);
    ASSERT_EQ(SF::asyncLoad(address, rdata, sizeof(rdata)), STORAGE_OK);

    ASSERT_FALSE(memcmp(wdata, rdata, sizeof(rdata)));
}

TEST_F(StorageFixtureAsync, UnknownBrokenPageAndHeaders) {
    uint8_t fill_data[(Header::PAGES_COUNT - 1) * STORAGE_PAGE_PAYLOAD_SIZE] = {};
    uint8_t wdata[4 * STORAGE_PAGE_PAYLOAD_SIZE] = { 1, 2, 3, 4, 5 };
    uint8_t rdata[sizeof(wdata)] = {};

    for (unsigned i = 0; i < StorageMacroblock::RESERVED_PAGES_COUNT * STORAGE_PAGE_SIZE; i++) {
        SF::storage.writeBlock(StorageMacroblock::PAGES_COUNT * STORAGE_PAGE_SIZE + i, true);
    }
    for (unsigned i = 0; i < STORAGE_PAGE_SIZE; i++) {
        SF::storage.writeBlock((StorageMacroblock::PAGES_COUNT + StorageMacroblock::RESERVED_PAGES_COUNT + 1) * STORAGE_PAGE_SIZE + i, true);
    }

    // Заполняем память
    ASSERT_EQ(SF::asyncFind(FIND_MODE_EMPTY, &address), STORAGE_OK);
    ASSERT_EQ(SF::asyncSave(address, shortPrefix, 1, fill_data, sizeof(fill_data)), STORAGE_OK);
    
    // Сохраняем данные
    ASSERT_EQ(SF::asyncFind(FIND_MODE_EMPTY, &address), STORAGE_OK);
    ASSERT_EQ(SF::asyncSave(address, shortPrefix, 2, wdata, sizeof(wdata)), STORAGE_OK);

    // Сохраняем данные
    ASSERT_EQ(SF::asyncFind(FIND_MODE_EQUAL, &address, shortPrefix, 2), STORAGE_OK);
    ASSERT_EQ(SF::asyncLoad(address, rdata, sizeof(rdata)), STORAGE_OK);

    ASSERT_FALSE(memcmp(wdata, rdata, sizeof(rdata)));
}

TEST_F(StorageFixtureAsync, BrokenHeaders1) {
    uint8_t fill_data[Header::PAGES_COUNT * STORAGE_PAGE_PAYLOAD_SIZE] = {};
    uint8_t wdata[4 * STORAGE_PAGE_PAYLOAD_SIZE] = { 1, 2, 3, 4, 5 };
    uint8_t rdata[sizeof(wdata)] = {};

    for (unsigned i = 0; i < StorageMacroblock::RESERVED_PAGES_COUNT * STORAGE_PAGE_SIZE; i++) {
        SF::storage.writeBlock(StorageMacroblock::PAGES_COUNT * STORAGE_PAGE_SIZE + i, true);
        SF::storage.readBlock(StorageMacroblock::PAGES_COUNT * STORAGE_PAGE_SIZE + i, true);
    }

    // Заполняем память
    ASSERT_EQ(SF::asyncFind(FIND_MODE_EMPTY, &address), STORAGE_OK);
    ASSERT_EQ(SF::asyncSave(address, shortPrefix, 1, fill_data, sizeof(fill_data)), STORAGE_OK);

    // Сохраняем данные
    ASSERT_EQ(SF::asyncFind(FIND_MODE_EMPTY, &address), STORAGE_OK);
    ASSERT_EQ(SF::asyncSave(address, shortPrefix, 2, wdata, sizeof(wdata)), STORAGE_OK);

    // Сохраняем данные
    ASSERT_EQ(SF::asyncFind(FIND_MODE_EQUAL, &address, shortPrefix, 2), STORAGE_OK);
    ASSERT_EQ(SF::asyncLoad(address, rdata, sizeof(rdata)), STORAGE_OK);

    ASSERT_FALSE(memcmp(wdata, rdata, sizeof(rdata)));
}

TEST_F(StorageFixtureAsync, BrokenHeaders2) {
    uint8_t fill_data[(Header::PAGES_COUNT - 1) * STORAGE_PAGE_PAYLOAD_SIZE] = {};
    uint8_t wdata[4 * STORAGE_PAGE_PAYLOAD_SIZE] = { 1, 2, 3, 4, 5 };
    uint8_t rdata[sizeof(wdata)] = {};

    for (unsigned i = 0; i < StorageMacroblock::RESERVED_PAGES_COUNT * STORAGE_PAGE_SIZE; i++) {
        SF::storage.writeBlock(StorageMacroblock::PAGES_COUNT * STORAGE_PAGE_SIZE + i, true);
    }

    // Заполняем память
    ASSERT_EQ(SF::asyncFind(FIND_MODE_EMPTY, &address), STORAGE_OK);
    ASSERT_EQ(SF::asyncSave(address, shortPrefix, 1, fill_data, sizeof(fill_data)), STORAGE_OK);

    // Сохраняем данные
    ASSERT_EQ(SF::asyncFind(FIND_MODE_EMPTY, &address), STORAGE_OK);
    ASSERT_EQ(SF::asyncSave(address, shortPrefix, 2, wdata, sizeof(wdata)), STORAGE_OK);

    // Сохраняем данные
    ASSERT_EQ(SF::asyncFind(FIND_MODE_EQUAL, &address, shortPrefix, 2), STORAGE_OK);
    ASSERT_EQ(SF::asyncLoad(address, rdata, sizeof(rdata)), STORAGE_OK);

    ASSERT_FALSE(memcmp(wdata, rdata, sizeof(rdata)));
}

TEST_F(StorageFixtureAsync, BrokenHeaders3) {
    uint8_t fill_data[(Header::PAGES_COUNT - 1) * STORAGE_PAGE_PAYLOAD_SIZE] = {};
    uint8_t wdata[4 * STORAGE_PAGE_PAYLOAD_SIZE] = { 1, 2, 3, 4, 5 };
    uint8_t rdata[sizeof(wdata)] = {};

    for (unsigned i = 0; i < StorageMacroblock::RESERVED_PAGES_COUNT * STORAGE_PAGE_SIZE; i++) {
        SF::storage.writeBlock(StorageMacroblock::PAGES_COUNT * STORAGE_PAGE_SIZE + i, true);
    }
    for (unsigned i = 0; i < StorageMacroblock::RESERVED_PAGES_COUNT; i++) {
        SF::storage.writeBlock((StorageMacroblock::PAGES_COUNT + i) * STORAGE_PAGE_SIZE + 5, false);
    }

    // Заполняем память
    ASSERT_EQ(SF::asyncFind(FIND_MODE_EMPTY, &address), STORAGE_OK);
    ASSERT_EQ(SF::asyncSave(address, shortPrefix, 1, fill_data, sizeof(fill_data)), STORAGE_OK);

    // Сохраняем данные
    ASSERT_EQ(SF::asyncFind(FIND_MODE_EMPTY, &address), STORAGE_OK);
    ASSERT_EQ(SF::asyncSave(address, shortPrefix, 2, wdata, sizeof(wdata)), STORAGE_OK);

    // Сохраняем данные
    ASSERT_EQ(SF::asyncFind(FIND_MODE_EQUAL, &address, shortPrefix, 2), STORAGE_OK);
    ASSERT_EQ(SF::asyncLoad(address, rdata, sizeof(rdata)), STORAGE_OK);

    ASSERT_FALSE(memcmp(wdata, rdata, sizeof(rdata)));
}

TEST_F(StorageFixtureAsync, FormatMacroblock) {
    uint32_t macroblockIndex = 0;
    ASSERT_EQ(StorageMacroblock::formatMacroblock(macroblockIndex), STORAGE_OK);

    Header header(StorageMacroblock::getMacroblockAddress(macroblockIndex));
    ASSERT_EQ(header.load(), STORAGE_OK);
    ASSERT_TRUE(header.isAddressEmpty(StorageMacroblock::RESERVED_PAGES_COUNT * STORAGE_PAGE_SIZE));
}

TEST_F(StorageFixtureAsync, FormatMacroblockWithInvalidHeader) {
    uint32_t macroblockIndex = 0;

    // Портим заголовок
    for (unsigned i = 0; i < StorageMacroblock::RESERVED_PAGES_COUNT * STORAGE_PAGE_SIZE; i++) {
        SF::storage.writeBlock(i, true);
    }
    Header header(StorageMacroblock::getMacroblockAddress(macroblockIndex));
    ASSERT_EQ(header.save(), STORAGE_HEADER_ERROR);
    ASSERT_EQ(header.load(), STORAGE_HEADER_ERROR);

    ASSERT_EQ(StorageMacroblock::formatMacroblock(macroblockIndex), STORAGE_OK);
}

TEST_F(StorageFixtureAsync, FillMemoryBreakFirstPayloadAndDeleteSaveNew) {
    uint8_t wdata[STORAGE_PAGE_PAYLOAD_SIZE] = { 1, 2, 3, 4, 5 };
    uint8_t newData[STORAGE_PAGE_PAYLOAD_SIZE] = { 6, 7, 8, 9, 10 };
    uint8_t rdata[STORAGE_PAGE_PAYLOAD_SIZE] = { 0 };

    // Шаг 1: Заполняем всю память одинаковыми данными
    status = STORAGE_OK;
    uint32_t pagesCount = 0;
    while (status == STORAGE_OK) {
        status = SF::asyncFind(FIND_MODE_EMPTY, &address);
        if (status != STORAGE_OK) {
            break;
        }
        status = SF::asyncSave(address, shortPrefix, pagesCount + 1, wdata, sizeof(wdata));
        ASSERT_EQ(status, STORAGE_OK);
        pagesCount++;
    }

    // Проверяем, что память полностью заполнена
    ASSERT_EQ(pagesCount, StorageAT::getPayloadPagesCount());

    // Шаг 2: Повреждаем первую страницу полезной нагрузки
    uint32_t firstPayloadAddress = StorageMacroblock::RESERVED_PAGES_COUNT * STORAGE_PAGE_SIZE;
    Page firstPage(firstPayloadAddress);
    ASSERT_EQ(firstPage.load(), STORAGE_OK);
    ASSERT_EQ(firstPage.save(), STORAGE_OK);

    // Портим данные первой страницы
    SF::storage.setByte(firstPayloadAddress + STORAGE_PAGE_SIZE / 2, 0xFF);
    ASSERT_EQ(firstPage.load(), STORAGE_ERROR);

    // Шаг 3: Пытаемся удалить данные и сохранить новые
    // Удаляем данные по префиксу и индексу
    ASSERT_EQ(SF::sat->deleteData(shortPrefix, 1), STORAGE_OK);

    // Проверяем, что данные удалены
    ASSERT_EQ(SF::asyncFind(FIND_MODE_EQUAL, &address, shortPrefix, 1), STORAGE_NOT_FOUND);

    // Сохраняем новые данные
    ASSERT_EQ(SF::asyncFind(FIND_MODE_EMPTY, &address), STORAGE_OK);
    ASSERT_EQ(SF::asyncSave(address, shortPrefix, 1, newData, sizeof(newData)), STORAGE_OK);

    // Проверяем, что новые данные сохранены корректно
    ASSERT_EQ(SF::asyncLoad(address, rdata, sizeof(rdata)), STORAGE_OK);
    EXPECT_FALSE(memcmp(newData, rdata, sizeof(newData)));
}

TEST_F(StorageFixtureAsync, FillMemoryWithLongDataBreakFirstPayloadAndDeleteSaveNew) {
    // Шаг 1: Заполняем всю память очень длинными данными
    uint32_t longDataSize = StorageAT::getPayloadSize(); // Очень длинные данные, занимающие всю полезную память
    uint8_t* longData = new uint8_t[longDataSize];
    memset(longData, 0xAA, longDataSize); // Заполняем данные значением 0xAA

    address = 0;
    ASSERT_EQ(SF::asyncFind(FIND_MODE_EMPTY, &address), STORAGE_OK);
    ASSERT_EQ(SF::asyncSave(address, shortPrefix, 1, longData, longDataSize), STORAGE_OK);

    // Проверяем, что память полностью заполнена
    ASSERT_EQ(SF::asyncFind(FIND_MODE_EMPTY, &address), STORAGE_NOT_FOUND); // Память должна быть заполнена

    // Шаг 2: Повреждаем первую страницу полезной нагрузки
    uint32_t firstPayloadAddress = StorageMacroblock::RESERVED_PAGES_COUNT * STORAGE_PAGE_SIZE;
    Page firstPage(firstPayloadAddress);
    ASSERT_EQ(firstPage.load(), STORAGE_OK);
    ASSERT_EQ(firstPage.save(), STORAGE_OK);

    // Портим данные первой страницы
    SF::storage.setByte(firstPayloadAddress + STORAGE_PAGE_SIZE / 2, 0xFF);
    ASSERT_EQ(firstPage.load(), STORAGE_ERROR);

    // Шаг 3: Пытаемся удалить данные и сохранить новые
    // Удаляем данные по префиксу и индексу
    ASSERT_EQ(SF::sat->deleteData(shortPrefix, 1), STORAGE_OK);

    // Проверяем, что данные удалены
    ASSERT_EQ(SF::asyncFind(FIND_MODE_EQUAL, &address, shortPrefix, 1), STORAGE_NOT_FOUND);

    // Сохраняем новые данные
    uint8_t newData[STORAGE_PAGE_PAYLOAD_SIZE] = { 6, 7, 8, 9, 10 };
    ASSERT_EQ(SF::asyncFind(FIND_MODE_EMPTY, &address), STORAGE_OK);
    ASSERT_EQ(SF::asyncSave(address, shortPrefix, 2, newData, sizeof(newData)), STORAGE_OK);

    // Проверяем, что новые данные сохранены корректно
    uint8_t rdata[STORAGE_PAGE_PAYLOAD_SIZE] = { 0 };
    ASSERT_EQ(SF::asyncLoad(address, rdata, sizeof(rdata)), STORAGE_OK);
    EXPECT_FALSE(memcmp(newData, rdata, sizeof(newData)));

    // Освобождаем память
    delete[] longData;
}
