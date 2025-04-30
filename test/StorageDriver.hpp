/* Copyright © 2025 Georgy E. All rights reserved. */

#ifndef _STORAGE_DRIVER_HPP_
#define _STORAGE_DRIVER_HPP_

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "StorageAT.h"


class StorageDriver: public IStorageDriver
{
public: 
	StorageStatus read(const uint32_t address, uint8_t* data, const uint32_t len) override;
	StorageStatus write(const uint32_t address, const uint8_t* data, const uint32_t len) override;
	StorageStatus erase(const uint32_t* addresses, const uint32_t count) override;

	StorageStatus asyncRead(const uint32_t address, uint8_t* data, const uint32_t len) override;
	StorageStatus asyncWrite(const uint32_t address, const uint8_t* data, const uint32_t len) override;
    StorageStatus asyncErase(const uint32_t* addresses, const uint32_t count) override;
};

#endif