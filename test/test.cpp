#include <iostream>

#include "StorageAT.h"
#include "StorageEmulator.h"


const int PAGES_COUNT = 200; // bytes

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

int main()
{
    StorageAT sat(
        storage.getPagesCount(),
        read_driver,
        write_driver
    );

    std::cout << "Start testing" << std::endl;

    return 0;
}