/* Copyright © 2025 Georgy E. All rights reserved. */

#ifndef _STORAGE_SEARCH_H_
#define _STORAGE_SEARCH_H_


#include <stdint.h>
#include <stdbool.h>

#include "StorageAT.h"
#include "StoragePage.h"
#include "StorageType.h"


/*
 * StorageSearchBase is a parent base class for search data in storage allocation table
 */
class StorageSearchBase
{
public:
	/*
	 * StorageSearchBase constructor
	 *
	 * @param startSearchAddress The address from which the search begins
	 */
	StorageSearchBase(uint32_t startSearchAddress = 0);

	/*
	 * StorageSearchBase destructor
	 */
	virtual ~StorageSearchBase() { foundOnce = false; }

	/*
	 * Searches data in all memory
	 * 
	 * @param prefix     String page prefix of header
	 * @param id         Integer page prefix of header
	 * @param resAddress Pointer that used to find needed page address
	 * @return           Returns STORAGE_OK if data was found
	 */
	virtual StorageStatus searchPageAddress(
		const uint8_t  prefix[STORAGE_PAGE_PREFIX_SIZE],
		const uint32_t id,
		uint32_t*      resAddress
	);

	/*
	 * Searches data in current macroblock
	 * 
	 * @param header Current macroblock header
	 * @param prefix String page prefix of header
	 * @param id     Integer page prefix of header
	 * @return       Returns STORAGE_OK if data was found
	 */
	virtual StorageStatus searchPageAddressInMacroblock(
		Header*        header,
		const uint8_t  prefix[STORAGE_PAGE_PREFIX_SIZE],
		const uint32_t id,
		const bool     start = true
	);

	/*
	 * @return Returns true if current mode needed first result
	 */
	virtual bool isNeededFirstResult() { return false; }

	/*
	 * @return Returns current mode start search address
	 */
	virtual uint32_t getStartCmpId() { return 0; }

	// TODO: docs
	static bool found();
	static uint32_t getAddress();
	static void setStartId(const uint32_t id);
	static void reset();

protected:
	/* Start search address */
	static uint32_t startSearchAddress;

	/* Flag that indicates that the needed prefix (and id) was found in memory */
	static bool     foundOnce;
	
	/* Flag that indicates that the needed prefix (and id) was found in current macroblock */
	static bool     foundInMacroblock;

	/* Previously result of search */
	static uint32_t prevAddress;

	/* Previously header ID of search */
	static uint32_t prevId;

	/*
	 * @return Returns true if current header ids matches the mode condition
	 */
	virtual bool isIdFound(
		const uint32_t,
		const uint32_t
	) { return false; };
};

/*
 * StorageSearchEqual is a class that searches equal prefix and id
 */
class StorageSearchEqual: public StorageSearchBase
{
public:
	/*
	 * StorageSearchEqual constructor
	 *
	 * @param startSearchAddress The address from which the search begins
	 */
	StorageSearchEqual(uint32_t startSearchAddress = 0): StorageSearchBase(startSearchAddress) {}

	/*
	 * @return Returns true if current mode needed first result
	 */
	bool isNeededFirstResult() override { return true; }

protected:

	/*
	 * @return Returns true if current header ids matches the mode condition
	 */
	bool isIdFound(
		const uint32_t headerId,
		const uint32_t targetId
	) override;
};

/*
 * StorageSearchNext is a class that searches equal prefix and next id
 */
class StorageSearchNext: public StorageSearchBase
{
public:
	/*
	 * StorageSearchNext constructor
	 *
	 * @param startSearchAddress The address from which the search begins
	 */
	StorageSearchNext(uint32_t startSearchAddress = 0): StorageSearchBase(startSearchAddress) {}

	/*
	 * @return Returns current mode start search address
	 */
	uint32_t getStartCmpId() override { return StorageAT::MAX_ADDRESS; }

protected:

	/*
	 * @return Returns true if current header ids matches the mode condition
	 */
	bool isIdFound(
		const uint32_t headerId,
		const uint32_t targetId
	) override;
};

/*
 * StorageSearchMax is a class that searches equal prefix and min id
 */
class StorageSearchMin: public StorageSearchBase
{
public:
	/*
	 * StorageSearchMin constructor
	 *
	 * @param startSearchAddress The address from which the search begins
	 */
	StorageSearchMin(uint32_t startSearchAddress = 0): StorageSearchBase(startSearchAddress) {}

	/*
	 * @return Returns current mode start search address
	 */
	uint32_t getStartCmpId() override { return StorageAT::MAX_ADDRESS; }

protected:

	/*
	 * @return Returns true if current header ids matches the mode condition
	 */
	bool isIdFound(
		const uint32_t headerId,
		const uint32_t
	) override;
};

/*
 * StorageSearchMax is a class that searches equal prefix and max id
 */
class StorageSearchMax: public StorageSearchBase
{
public:
	/*
	 * StorageSearchMax constructor
	 *
	 * @param startSearchAddress The address from which the search begins
	 */
	StorageSearchMax(uint32_t startSearchAddress = 0): StorageSearchBase(startSearchAddress) {}

protected:
	/*
	 * @return Returns true if current header ids matches the mode condition
	 */
	bool isIdFound(
		const uint32_t headerId,
		const uint32_t
	) override;
};

/*
 * StorageSearchMax is a class that searches empty page
 */
class StorageSearchEmpty: public StorageSearchBase
{
public:
	/*
	 * StorageSearchEmpty constructor
	 *
	 * @param startSearchAddress The address from which the search begins
	 */
	StorageSearchEmpty(uint32_t startSearchAddress = 0): StorageSearchBase(startSearchAddress) {}

	/*
	 * @return Returns true if current mode needed first result
	 */
	bool isNeededFirstResult() override { return true; }

	/*
	 * Searches data in current macroblock
	 * 
	 * @param header Current macroblock header
	 * @param prefix String page prefix of header
	 * @param id     Integer page prefix of header
	 * @return       Returns STORAGE_OK if data was found
	 */
	StorageStatus searchPageAddressInMacroblock(
		Header*        header,
		const uint8_t  prefix[STORAGE_PAGE_PREFIX_SIZE],
		const uint32_t id,
		const bool     start = true
	) override;
};


#endif