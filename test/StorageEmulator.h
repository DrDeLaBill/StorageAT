/* Copyright © 2023 Georgy E. All rights reserved. */

#pragma once


#include <memory>
#include <stdint.h>


enum StorageEmulatorStatus {
    EMULATOR_OK    = 0x00,
    EMULATOR_BUSY  = 0x01,
    EMULATOR_ERROR = 0x02,
    EMULATOR_OOM   = 0x03
};


class StorageEmulator
{
private:
    uint32_t pagesCount;
    uint32_t size;
    std::unique_ptr<uint8_t[]> memory;
    std::unique_ptr<bool[]> blocked;

    bool isBusy;

public:
    typedef struct _RequestsCount {
        unsigned read;
        unsigned write;
    } RequestsCount;

    std::unique_ptr<RequestsCount[]> requestsCount;

    StorageEmulator(uint32_t pagesCount);

    StorageEmulatorStatus writePage(const uint32_t address, const uint8_t* data, const uint32_t len);
    StorageEmulatorStatus readPage(const uint32_t address, uint8_t* data, const uint32_t len);
    StorageEmulatorStatus erase(const uint32_t* addresses, const uint32_t count);

    uint32_t getSize();
    uint32_t getPayloadSize();
    uint32_t getPagesCount();

    void setBusy(bool busy);
    void setBlocked(uint32_t idx, bool blockState);
    void setByte(uint32_t idx, uint8_t byte);

    void clear();

    void showReadWrite();
    void showPage(uint32_t address);
};