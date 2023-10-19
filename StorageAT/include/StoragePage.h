/* Copyright © 2023 Georgy E. All rights reserved. */

#ifndef STORAGE_STRUCT_HPP
#define STORAGE_STRUCT_HPP


#include <stdint.h>

#include "StorageType.h"


class Page
{
public:
	/* Data storage page size in bytes */
	static const uint16_t STORAGE_PAGE_SIZE       = 256;

	/* Page structure validator */
	static const uint32_t STORAGE_MAGIC           = 0xBEDAC0DE;

	/* Current page structure version */
	static const uint8_t STORAGE_VERSION          = 0x04;

	/* Available page title bytes in block header */
	static const uint8_t STORAGE_PAGE_PREFIX_SIZE = 4;


	typedef enum _PageStatus {
		PAGE_STATUS_EMPTY = static_cast<uint8_t>(0b00000001),
		PAGE_STATUS_START = static_cast<uint8_t>(0b00000010),
		PAGE_STATUS_MID   = static_cast<uint8_t>(0b00000100),
		PAGE_STATUS_END   = static_cast<uint8_t>(0b00001000)
	} PageStatus;

	/* Page header meta data structure */
	PACK(typedef struct, _PageMeta {
		uint32_t magic;
		uint8_t  version;
		uint8_t  status;
		uint32_t next_addr;
		uint8_t  prefix[STORAGE_PAGE_PREFIX_SIZE];
		uint32_t id;
	} PageMeta);


	/* Available payload bytes in page structure */
	static const uint16_t STORAGE_PAGE_PAYLOAD_SIZE =
		STORAGE_PAGE_SIZE -
		sizeof(struct _PageMeta) -
		sizeof(uint16_t);

	/* Page structure */
	PACK(typedef struct, _PageStruct {
		PageMeta header;
		uint8_t  payload[STORAGE_PAGE_PAYLOAD_SIZE];
		uint16_t crc;
	} PageStruct);


	uint32_t   address;
	PageStruct page;

	Page(uint32_t address);

	void setPageStatus(uint8_t status);
	bool isSetPageStatus(uint8_t status);

	StorageStatus load(bool startPage = false);
	StorageStatus save();
	StorageStatus deletePage();
	bool isEmpty();

	StorageStatus loadNext();
	bool validateNextAddress();

protected:
	bool     validate();
	uint16_t getCRC16(uint8_t* buf, uint16_t len);
};

class HeaderPage: public Page
{
public:
	typedef enum _PageHeaderStatus {
		PAGE_OK      = static_cast<uint8_t>(0b00000001),
		PAGE_EMPTY   = static_cast<uint8_t>(0b00000010),
		PAGE_BLOCKED = static_cast<uint8_t>(0b00000100),
	} PageHeaderStatus;

	PACK(typedef struct, _PageHeader {
		uint8_t  prefix[STORAGE_PAGE_PREFIX_SIZE];
		uint32_t id;
		uint8_t  status;
	} PageHeader);

	/* Pages in block that header page contains */
	static const uint32_t PAGE_HEADERS_COUNT = STORAGE_PAGE_PAYLOAD_SIZE / sizeof(struct _PageHeader);

	PACK(typedef struct, _HeaderPageStruct {
		PageHeader pages[PAGE_HEADERS_COUNT];
	} HeaderPageStruct);


	uint32_t          sectorIndex;
	HeaderPageStruct* data;

	HeaderPage(uint32_t address);

	void setHeaderStatus(uint32_t pageIndex, uint8_t status);
	bool isSetHeaderStatus(uint32_t pageIndex, uint8_t status);

	StorageStatus createHeader();
	StorageStatus load();
	StorageStatus save();
	StorageStatus deletePage() { return STORAGE_ERROR; }
};


#endif
