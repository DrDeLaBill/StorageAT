/* Copyright © 2025 Georgy E. All rights reserved. */

#ifndef _STORAGE_FIXTURE_HPP_
#define _STORAGE_FIXTURE_HPP_


#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <memory>
#include <cstdint>

#include "StorageAT.h"
#include "StorageDriver.hpp"
#include "StorageEmulator.hpp"

#include "Timer.h"


class StorageFixture: public testing::Test
{
protected:
    static constexpr char shortPrefix[] = "tst";
    static constexpr char longPrefix [] = "longstringprefix";
    static constexpr char brokenPrefix[5] = { 't', 'e', 's', 't', 's' };


public:
    static constexpr int SECTORS_COUNT = 20;
    static constexpr int PAGES_COUNT = StorageMacroblock::PAGES_COUNT * SECTORS_COUNT;
    static constexpr int PAGE_LEN = STORAGE_PAGE_SIZE;

    static uint32_t MIN_MEMORY_ERASE_SIZE;
    static uint32_t address;
    static StorageStatus status;
    static StorageDriver driver;
    static std::unique_ptr<StorageAT> sat;
    static StorageEmulator storage;

    virtual void SetUp() override;
    void TearDown() override;
};


class StorageFixtureAsync: public StorageFixture
{
private:
    static utl::Timer timer;
    static bool asyncReady;

    static void asyncCallback(StorageStatus _status);

public:
    virtual void SetUp() override;

    static StorageStatus asyncFind(
        StorageFindMode mode,
        uint32_t*       address,
        const char*     prefix = "",
        uint32_t        id = 0
    );

    static StorageStatus asyncLoad(uint32_t address, uint8_t* data, uint32_t len);

    static StorageStatus asyncSave(
        uint32_t       address,
        const char*    prefix,
        uint32_t       id,
        uint8_t*       data,
        uint32_t       len
    );

    static StorageStatus asyncRewrite(
        uint32_t    address,
        const char* prefix,
        uint32_t    id,
        uint8_t*    data,
        uint32_t    len
    );
    
};


#endif