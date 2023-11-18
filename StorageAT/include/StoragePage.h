/* Copyright © 2023 Georgy E. All rights reserved. */

#pragma once


#include <stdint.h>

#include "StorageType.h"


/*
 * Storage data minimal operable unit
 */
class Page
{
public:
	/* Data storage page size in bytes */
	static const uint16_t PAGE_SIZE      = 256;

	/* Page structure validator */
	static const uint32_t STORAGE_MAGIC  = 0xBEDAC0DE;

	/* Current page structure version */
	static const uint8_t STORAGE_VERSION = 0x05;

	/* Available page title bytes in block header */
	static const uint8_t PREFIX_SIZE     = 3;


	/* Packed page header meta data structure */
	STORAGE_PACK(typedef struct, _PageMeta {
		uint32_t magic;               // Special code
		uint8_t  version;             // StorageAT library version
		uint32_t prev_addr;           // Previosly data address
		uint32_t next_addr;           // Next data address
		uint8_t  prefix[PREFIX_SIZE]; // String page prefix for searching
		uint32_t id;                  // ID for searching
	} PageMeta);


	/* Available payload bytes in page structure */
	static const uint16_t PAYLOAD_SIZE =
		PAGE_SIZE -
		sizeof(struct _PageMeta) -
		sizeof(uint16_t);

	/* Page structure */
	STORAGE_PACK(typedef struct, _PageStruct {
		PageMeta header;                // Page meta data
		uint8_t  payload[PAYLOAD_SIZE]; // User payload data
		uint16_t crc;                   // Page CRC16
	} PageStruct);

	/* Current page */
	PageStruct page;

	/*
	 * Page constructor
	 * 
	 * @param address Page address in memory
	 */
	Page(uint32_t address);

	Page& operator=(Page* other);

	/*
	 * Loads and validates page from memory
	 *
	 * @param startPage Flag for validate that page page being loaded is the data start page
	 * @return          Returns STORAGE_OK if the page was loaded successfully
	 */
	virtual StorageStatus load(bool startPage = false);

	/*
	 * Saves page to memory
	 *
	 * @return Returns STORAGE_OK if page was loaded successfully
	 */
	virtual StorageStatus save();

	/*
	 * Loads and validates previously data page from memory
	 *
	 * @return Returns STORAGE_OK if the previously page exists and was loaded successfully
	 */
	StorageStatus loadPrev();

	/*
	 * Loads and validates next data page from memory
	 *
	 * @return Returns STORAGE_OK if the next page exists and was loaded successfully
	 */
	StorageStatus loadNext();

	/*
	 * Checks that the page is a data start page
	 * 
	 * @return Returns true if the page is a data start page
	 */
	bool isStart();

	/*
	 * Checks that the page is a data middle page
	 * 
	 * @return Returns true if the page is a data middle page
	 */
	bool isMiddle();

	/*
	 * Checks that the page is a data end page
	 * 
	 * @return Returns true if the page is a data end page
	 */
	bool isEnd();

	/*
	 * Validates the data page previously address
	 * 
	 * @return Returns true if the data page previously address is correct
	 */
	bool validatePrevAddress();

	/*
	 * Validates the data page next address
	 * 
	 * @return Returns true if the data page next address is correct
	 */
	bool validateNextAddress();

	/*
	 * @return Returns the page address
	 */
	uint32_t getAddress();

	/*
	 * Sets previously page address of the data
	 * 
	 * @param address previously page address
	 */
	void setPrevAddress(uint32_t address);

	/*
	 * Sets next page address of the data
	 * 
	 * @param address next page address
	 */
	void setNextAddress(uint32_t address);

protected:
	/* Page address */
	uint32_t address;

	/*
	 * Validates the page data
	 * 
	 * @return Returns true if the page data is correct
	 */
	virtual bool validate();

	/*
	 * Calculates the page data CRC16
	 * 
	 * @return Returns CRC16 of the page data
	 */
	uint16_t getCRC16(uint8_t* buf, uint16_t len);
};

/*
 * Header is a table of contents of the storage macroblock
 */
class Header: public Page
{
public:
	/* Header page statuses */
	typedef enum _PageHeaderStatus {
		PAGE_OK      = static_cast<uint8_t>(0b00000001), // Data on page exists
		PAGE_EMPTY   = static_cast<uint8_t>(0b00000010), // Page is empty
		PAGE_BLOCKED = static_cast<uint8_t>(0b00000100), // Page is blocked for load and save by StorageAT library
	} PageHeaderStatus;

	/* Single page status structure */
	STORAGE_PACK(typedef struct, _PageHeader {
		uint8_t  prefix[PREFIX_SIZE]; // String page prefix for searching
		uint32_t id;                  // ID for searching
		uint8_t  status;              // Page status (PageHeaderStatus)
	} PageHeader);

	/* Pages in block that header page contains */
	static const uint32_t PAGES_COUNT = PAYLOAD_SIZE / sizeof(struct _PageHeader);

	/* Header page payload data */
	STORAGE_PACK(typedef struct, _HeaderPageStruct {
		PageHeader pages[PAGES_COUNT]; // Macroblock page status structures
	} HeaderPageStruct);

	/* Pointer to payload header data */
	HeaderPageStruct* data;

	/*
	 * Header constructor
	 * 
	 * @param address Header address in memory
	 */
	Header(uint32_t address);

	Header& operator=(Header* other);

	/*
	 * Loads and validates header from memory
	 *
	 * @return Returns STORAGE_OK if the header was loaded successfully
	 */
	StorageStatus load();
	StorageStatus load(bool) override { return this->load(); };

	/*
	 * Saves the header to memory
	 *
	 * @return Returns STORAGE_OK if the header was loaded successfully
	 */
	StorageStatus save() override;

	/*
	 * Creates the header in the current macroblock
	 *
	 * @return Returns STORAGE_OK if the header was created successfully
	 */
	StorageStatus create();

	/*
	 * Deletes the page information from the header
	 *
	 * @return Returns STORAGE_OK if the page was deleted successfully
	 */
	StorageStatus deletePage(uint32_t targetAddress);

	/*
	 * Sets the page status in the header
	 *
	 * @param pageIndex The page index in current macroblock
	 * @param status    Target status
	 */
	void setHeaderStatus(PageHeader* pageHeader, uint8_t status);

	/*
	 * Checks that the target page status is set in the header
	 *
	 * @param pageIndex The page index in current macroblock
	 * @param status    Target status
	 * @return          Returns true if the target page status is set
	 */
	bool isSetHeaderStatus(PageHeader* pageHeader, uint8_t status);

	/*
	 * Sets the page blocked status in the header
	 *
	 * @param pageIndex The page index in current macroblock
	 */
	void setPageBlocked(PageHeader* pageHeader);

	/*
	 * Checks that the target page is empty
	 *
	 * @param targetAddress The page address
	 * @return              Returns true if the target page is empty
	 */
	bool isAddressEmpty(uint32_t targetAddress);

	/*
	 * Checks that the target page has the same header prefix and ID
	 *
	 * @param pageIndex The page index in current macroblock
	 * @param prefix    String page prefix of header
	 * @param id        Integer page prefix of header
	 * @return          Returns true if the target page has equal meta information
	 */
	bool isSameMeta(uint32_t pageIndex, const uint8_t* prefix, uint32_t id);

	/*
	 * Calculates target macroblock start address 
	 *
	 * @param address The page address
	 * @return        Returns target macroblock start address 
	 */
	static uint32_t getMacroblockStartAddress(uint32_t address);

	/*
	 * @return Returns header macroblock index in memory
	 */
	uint32_t getMacroblockIndex();

protected:
	/*
	 * Validates the header data
	 * 
	 * @return Returns true if the header data is correct
	 */
	bool validate() override;

private:
	/* Header macroblock index in memory */
	uint32_t m_macroblockIndex;

};
