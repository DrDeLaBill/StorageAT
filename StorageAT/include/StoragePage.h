/* Copyright © 2023 Georgy E. All rights reserved. */

#pragma once


#include <stdint.h>

#include "StorageType.h"


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
	static const uint8_t PREFIX_SIZE     = 4;


	/* Page header meta data structure */
	PACK(typedef struct, _PageMeta {
		uint32_t magic;
		uint8_t  version;
		uint32_t prev_addr;
		uint32_t next_addr;
		uint8_t  prefix[PREFIX_SIZE];
		uint32_t id;
	} PageMeta);


	/* Available payload bytes in page structure */
	static const uint16_t PAYLOAD_SIZE =
		PAGE_SIZE -
		sizeof(struct _PageMeta) -
		sizeof(uint16_t);

	/* Page structure */
	PACK(typedef struct, _PageStruct {
		PageMeta header;
		uint8_t  payload[PAYLOAD_SIZE];
		uint16_t crc;
	} PageStruct);

	PageStruct page;

	Page(uint32_t address);

	Page& operator=(Page* other);

	virtual StorageStatus load(bool startPage = false);
	virtual StorageStatus save();
	virtual StorageStatus deletePage();
	StorageStatus loadPrev();
	StorageStatus loadNext();

	bool isStart();
	bool isMiddle();
	bool isEnd();
	bool validatePrevAddress();
	bool validateNextAddress();
	bool isEmpty();
	uint32_t getAddress();

	void setPrevAddress(uint32_t address);
	void setNextAddress(uint32_t address);

protected:
	uint32_t address;

protected:
	virtual bool validate();
	uint16_t getCRC16(uint8_t* buf, uint16_t len);
};

class Header: public Page
{
public:
	typedef enum _PageHeaderStatus {
		PAGE_OK      = static_cast<uint8_t>(0b00000001),
		PAGE_EMPTY   = static_cast<uint8_t>(0b00000010),
		PAGE_BLOCKED = static_cast<uint8_t>(0b00000100),
	} PageHeaderStatus;

	PACK(typedef struct, _PageHeader {
		uint8_t  prefix[PREFIX_SIZE];
		uint32_t id;
		uint8_t  status;
	} PageHeader);

	/* Pages in block that header page contains */
	static const uint32_t PAGES_COUNT = PAYLOAD_SIZE / sizeof(struct _PageHeader);

	PACK(typedef struct, _HeaderPageStruct {
		PageHeader pages[PAGES_COUNT];
	} HeaderPageStruct);

	HeaderPageStruct* data;

	Header(uint32_t address);

	Header& operator=(Header* other);

	StorageStatus load();
	StorageStatus load(bool) override { return this->load(); };
	StorageStatus save() override;
	StorageStatus create();
	StorageStatus deletePage() override { return STORAGE_ERROR; }

	void setHeaderStatus(uint32_t pageIndex, uint8_t status);
	bool isSetHeaderStatus(uint32_t pageIndex, uint8_t status);
	void setPageBlocked(uint32_t pageIndex);
	uint32_t getSectorIndex();
	bool isAddressEmpty(uint32_t targetAddress);
	bool isSameMeta(uint32_t pageIndex, uint8_t prefix[Page::PREFIX_SIZE], uint32_t id);

	static uint32_t getSectorStartAddress(uint32_t address);

protected:
	bool validate() override;

private:
	uint32_t m_sectorIndex;

};
