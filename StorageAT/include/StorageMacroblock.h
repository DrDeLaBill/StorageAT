/* Copyright © 2023 Georgy E. All rights reserved. */

#pragma once


#include <stdint.h>
#include <stdbool.h>

#include "StoragePage.h"
#include "StorageType.h"


class StorageMacroblock
{
public:
	/* Macroblock pages count that reserved for header page at the beginning of the macroblock */
	static const uint32_t RESERVED_PAGES_COUNT = 4;

	/* Macroblock pages count */
	static const uint32_t PAGES_COUNT = RESERVED_PAGES_COUNT + Header::PAGES_COUNT;


	/*
	 * Calculates macroblock start address
	 *
	 * @param macroblockIndex Macroblock index in memory
	 * @return                Returns macroblock start address
	 */
	static uint32_t getMacroblockAddress(uint32_t macroblockIndex);

	/*
	 * Calculates macroblock index by address
	 *
	 * @param macroblockAddress Macroblock start address in memory
	 * @return                  Returns macroblock index
	 */
	static uint32_t getMacroblockIndex(uint32_t macroblockAddress);

	/*
	 * Calculates macroblock count in memory
	 *
	 * @return Returns macroblocks count
	 */
	static uint32_t getMacroblocksCount();

	/*
	 * Calculates page address
	 *
	 * @param macroblockIndex Macroblock index in memory
	 * @param pageIndex       Page index in macroblock
	 * @return                Returns page address
	 */
	static uint32_t getPageAddressByIndex(uint32_t macroblockIndex, uint32_t pageIndex);

	/*
	 * Calculates page index in macroblock
	 *
	 * @param address Page address
	 * @return        Returns page index in macroblock
	 */
	static uint32_t getPageIndexByAddress(uint32_t address);

	/*
	 * Checks that the target address is macroblock header address
	 * 
	 * @param address Page address
	 * @return        Returns true if address is a macroblock address
	 */
	static bool isMacroblockAddress(uint32_t address);

	/*
	 * Formats target macroblock
	 * 
	 * @param macroblockIndex Target macroblock index
	 * @return                Returns STORAGE_OK if the macroblock was formatted successfully
	 */
	static StorageStatus formatMacroblock(uint32_t macroblockIndex);

	/*
	 * Loads header from the header macroblock
	 * 
	 * @param header Pointer to the target header
	 * @return       Returns STORAGE_OK if the header was loaded successfully
	 */
	static StorageStatus loadHeader(Header *header);
};
