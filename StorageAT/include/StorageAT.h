/* Copyright © 2023 Georgy E. All rights reserved. */

#pragma once


#include <stdint.h>

#include "StoragePage.h"
#include "StorageType.h"
#include "StorageMacroblock.h"


/*
 * class IStorageDriver
 *
 * IStorageDriver is an interface for memory adapter
 * 
 */
class IStorageDriver
{
public:
	virtual StorageStatus read(uint32_t, uint8_t*, uint32_t) { return STORAGE_ERROR; }
	virtual StorageStatus write(uint32_t, uint8_t*, uint32_t) { return STORAGE_ERROR; }
};

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

	/* Storage read/write driver */
	static IStorageDriver* m_driver;

public:
	/* Max available address for StorageFS */
	static const uint32_t MAX_ADDRESS = 0xFFFFFFFF;

	/*
	 * Storage Allocation Table constructor
	 *
	 * @param pagesCount Physical drive pages count
	 * @param driver     Physical drive read/write driver
	 */
	StorageAT(
		uint32_t        pagesCount,
		IStorageDriver* driver // TODO: pointer to reference
	);

	/*
	 * Find data in storage
	 * 
	 * @param mode    Current search mode
	 * @param address Pointer that used to find needed page address
	 * @param prefix  String page prefix of header that needed to be found in storage
	 * @param id      Integer page prefix of header that needed to be found in storage
	 * @return        Returns STORAGE_OK if the data was found
	 */
	StorageStatus find(
		StorageFindMode mode,
		uint32_t*       address,
		const char*     prefix = "",
		uint32_t        id = 0
	);


	/*
	 * Load the data from storage address
	 *
	 * @param address Storage page address to load
	 * @param data    Pointer to data array for load data
	 * @param len     Data array length
	 * @return        Returns STORAGE_OK if the data was loaded successfully
	 */
	StorageStatus load(uint32_t address, uint8_t* data, uint32_t len);


	/*
	 * Save the data to storage address
	 *
	 * @param address Storage page address to save
	 * @param prefix  String page prefix of header
	 * @param id      Integer page prefix of header
	 * @param data    Pointer to data array for save data
	 * @param len     Array size
	 * @return        Returns STORAGE_OK if the data was saved successfully
	 */
	StorageStatus save(
		uint32_t    address,
		const char* prefix,
		uint32_t    id,
		uint8_t*    data,
		uint32_t    len
	);
	
	/*
	 * Rewrite the data contained in storage address
	 *
	 * @param address Storage page address to save
	 * @param prefix  String page prefix of header
	 * @param id      Integer page prefix of header
	 * @param data    Pointer to data array for save data
	 * @param len     Array size
	 * @return        Returns STORAGE_OK if the data was rewritten successfully
	 */
	StorageStatus rewrite(
		uint32_t    address,
		const char* prefix,
		uint32_t    id,
		uint8_t*    data,
		uint32_t    len
	);

	/*
	 * Format FLASH memory
	 *
	 * @return  Returns STORAGE_OK if the memory was formatted successfully
	 */
	StorageStatus format();

	/*
	 * Removes data from address
	 *
	 * @param address Data start address
	 * @return        Returns STORAGE_OK if the data was removed successfully
	 */
	StorageStatus deleteData(uint32_t address);

	/*
	 * @return Returns pages count of physical drive
	 */
	static uint32_t getStoragePagesCount();

	/*
	 * @return Returns bytes count on physical storage
	 */
	static uint32_t getStorageSize();

	/*
	 * @return Returns max payload pages count in storage allocation table
	 */
	static uint32_t getPayloadPagesCount();

	/*
	 * @return Returns max payload bytes count in storage allocation table
	 */
	static uint32_t getPayloadSize();

	/*
	 * @return Returns read/write driver of physical drive 
	 */
	static IStorageDriver* driverCallback();
};
