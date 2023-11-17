/* Copyright © 2023 Georgy E. All rights reserved. */

#pragma once


#include "StoragePage.h"
#include "StorageType.h"


/*
 * StorageData allows to write data larger than Page::PAYLOAD_SIZE to storage
 */
class StorageData
{
private:
	/* Data start address */
	uint32_t m_startAddress;

	/* 
	 * Checks that there is no data on the address
	 *
	 * @return Returns STORAGE_OK if address is empty
	 */
	StorageStatus isEmptyAddress(uint32_t address);

public:
	/*
	 * Storage data constructor
	 *
	 * @param startAddress Data start address in memory
	 */
	StorageData(uint32_t startAddress);

	/*
	 * Loads user data from m_startAddress storage address
	 * 
	 * @param data Pointer to data array for load data
	 * @param len  Data array length
	 * @return     Returns STORAGE_OK if the data was loaded successfully
	 */
	StorageStatus load(uint8_t* data, uint32_t len);

	/*
	 * Saves user data on m_startAddress storage address
	 *
	 * @param data Pointer to data array for save data
	 * @param len  Array size
	 * @return     Returns STORAGE_OK if the data was saved successfully
	 */
	StorageStatus save(
		uint8_t  prefix[Page::PREFIX_SIZE],
		uint32_t id,
		uint8_t* data,
		uint32_t len
	);
	
	/*
	 * Rewrite user data on m_startAddress storage address
	 *
	 * @param data Pointer to data array for save data
	 * @param len  Array size
	 * @return     Returns STORAGE_OK if the data was rewritten successfully
	 */
	StorageStatus rewrite(
		uint8_t  prefix[Page::PREFIX_SIZE],
		uint32_t id,
		uint8_t* data,
		uint32_t len
	);
	
	/*
	 * Delete data from m_startAddress storage address
	 *
	 * @return Returns STORAGE_OK if the data was deleted successfully
	 */
	StorageStatus deleteData();
	
	/*
	 * Searches data start address in storage
	 * 
	 * @param address Pointer to address of the data
	 * @return        Returns STORAGE_OK if the address was found successfully
	 */
	static StorageStatus findStartAddress(uint32_t* address);
	
	/*
	 * Searches data end address in storage
	 * 
	 * @param address Pointer to address of the data
	 * @return        Returns STORAGE_OK if the address was found successfully
	 */
	static StorageStatus findEndAddress(uint32_t* address);
};