/* Copyright © 2023 Georgy E. All rights reserved. */

#ifndef STORAGE_BOX_HPP
#define STORAGE_BOX_HPP


#include <stdint.h>
#include <stdbool.h>

#include "StoragePage.h"
#include "StorageType.h"


class StorageSector
{
public:
	/* Sector pages count that reserved for header page at the beginning of the sector */
	static const uint32_t SECTOR_RESERVED_PAGES_COUNT = 8;

	/* Sector pages count */
	static const uint32_t SECTOR_PAGES_COUNT = SECTOR_RESERVED_PAGES_COUNT + HeaderPage::PAGE_HEADERS_COUNT;


	/*
	 * Calculates sector start address
	 *
	 * @param sectorIndex Sector index in memory
	 * @return            Returns sector start address
	 */
	static uint32_t getSectorStartAdderss(uint32_t sectorIndex);

	/*
	 * Calculates sector index by address
	 *
	 * @param sectorAdress Sector start address in memory
	 * @return             Returns sector index
	 */
	static uint32_t getSectorIndex(uint32_t sectorAdress);

	/*
	 * Calculates sector count in memory
	 *
	 * @return Returns sectors count
	 */
	static uint32_t getSectorsCount();

	/*
	 * Calculates page address
	 *
	 * @param sectorIndex Sector index in memory
	 * @param pageIndex   Page index in sector
	 * @return            Returns page address
	 */
	static uint32_t getPageAddressByIndex(uint32_t sectorIndex, uint32_t pageIndex);

	/*
	 * Calculates page index in sector
	 *
	 * @param address Page address
	 * @return        Returns page index in sector
	 */
	static uint32_t getPageIndexByAddress(uint32_t address);

	static bool isSectorAddress(uint32_t address);

	static StorageStatus loadHeader(HeaderPage *header); // TODO: do operator= for Page class
};

#endif
