#include <iostream>

#include "StorageAT.hpp"


const int MEMORY_SIZE = 65535; // bytes

StorageDriverCallback read_driver = [](uint32_t address, uint8_t* data, uint32_t len) {
	return STORAGE_OK;
};

StorageDriverCallback write_driver = [](uint32_t address, uint8_t* data, uint32_t len) {
	return STORAGE_OK;
};

int main()
{
    StorageAT sat(
        MEMORY_SIZE,
        read_driver,
        write_driver
    );

    std::cout << "Start testing" << std::endl;

    return 0;
}