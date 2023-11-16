#ifndef STORAGE_EMULATOR_H
#define STORAGE_EMULATOR_H


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

    bool isBusy;

public:
    typedef struct _RequestsCount {
        unsigned read;
        unsigned write;
    } RequestsCount;

    std::unique_ptr<RequestsCount[]> requestsCount;

    StorageEmulator(uint32_t pagesCount);

    StorageEmulatorStatus writePage(uint32_t address, uint8_t* data, uint32_t len);
    StorageEmulatorStatus readPage(uint32_t address, uint8_t* data, uint32_t len);

    uint32_t getSize();
    uint32_t getPayloadSize();
    uint32_t getPagesCount();

    void setBusy(bool busy);

    void clear();

    void showReadWrite();
    void showPage(uint32_t address);
};


#endif