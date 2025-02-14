/* Copyright © 2023 Georgy E. All rights reserved. */

#ifndef _STORAGE_DATA_H_
#define _STORAGE_DATA_H_


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

	/*
	 * Erases page from memory
	 *
	 * @return Returns STORAGE_OK if the page erased successfully
	 */
	StorageStatus erasePage(const uint32_t address);

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
		uint8_t  prefix[STORAGE_PAGE_PREFIX_SIZE],
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
		uint8_t  prefix[STORAGE_PAGE_PREFIX_SIZE],
		uint32_t id,
		uint8_t* data,
		uint32_t len
	);
	
	/*
	 * Delete data
	 *
	 * @param  prefix The prefix of the data for delete
	 * @param  index  The index of the data for delete
	 * @return Returns STORAGE_OK if the data was deleted successfully
	 */
	StorageStatus deleteData(const uint8_t prefix[STORAGE_PAGE_PREFIX_SIZE], const uint32_t index);

	/*
	 * Removes data from address
	 *
	 * @param address The address of the data for delete
	 * @return        Returns STORAGE_OK if the data was removed successfully
	 */
	StorageStatus clearAddress(const uint32_t address);
	
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


#endif