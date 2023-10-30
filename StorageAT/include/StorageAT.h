/* Copyright © 2023 Georgy E. All rights reserved. */

#ifndef STORAGE_FS_HPP
#define STORAGE_FS_HPP


#include <stdint.h>

#include "StoragePage.h"
#include "StorageType.h"
#include "StorageSector.h"


/*
 * class StorageAT
 *
 * StorageAT is a data allocation table for storing arrays and structures
 *
 */
class StorageAT
{
private:
	/* Storage pages count */
	static uint32_t m_pagesCount;

	// TODO: docs
	static StorageDriverCallback m_readDriver;
	static StorageDriverCallback m_writeDriver;

public:
	/* Max available address for StorageFS */
	static const uint32_t STORAGE_MAX_ADDRESS = 0xFFFFFFFF;

	/*
	 * Storage File System constructor
	 *
	 * @param // TODO: params
	 */
	StorageAT(
		uint32_t              pagesCount,
		StorageDriverCallback read_driver,
		StorageDriverCallback write_driver
	);

	/*
	 * Find data in storage
	 * TODO: params
	 * @param searchData Name or index, or another value of header that needed to be found in storage
	 * @param address    Variable pointer that used to find needed page address
	 * @return           Returns STORAGE_OK if the data was found
	 */
	StorageStatus find(
		StorageFindMode mode,
		uint32_t*       address,
		uint8_t         prefix[Page::STORAGE_PAGE_PREFIX_SIZE] = {},
		uint32_t        id = 0
	);


	/*
	 * Load data from storage
	 *
	 * @param address Storage page address to load
	 * @param data    Pointer to data array for load data
	 * @param len     Data length
	 * @return        Returns STORAGE_OK if the data was loaded successfully
	 */
	StorageStatus load(uint32_t address, uint8_t* data, uint32_t len);


	/*
	 * Save data to storage
	 *
	 * @param address Storage page address to save
	 * @param data    Pointer to data array for save data
	 * @param len     Array size
	 * @return        Returns STORAGE_OK if the data was saved successfully
	 */
	StorageStatus save(
		uint32_t address,
		uint8_t  prefix[Page::STORAGE_PAGE_PREFIX_SIZE],
		uint32_t id,
		uint8_t* data,
		uint32_t len
	);

	/*
	 * Removes data from address
	 *
	 * @param address Data start address
	 * @return        Returns STORAGE_OK if the data was removed successfully
	 */
	StorageStatus deleteData(uint32_t address);

	/*
	 * @return Returns pages count on physical storage
	 */
	static uint32_t getPagesCount();

	/*
	 * @return Returns bytes count on physical storage
	 */
	static uint32_t getBytesSize();
// TODO: docs
	static StorageDriverCallback readCallback();
	static StorageDriverCallback writeCallback();
};


#endif
