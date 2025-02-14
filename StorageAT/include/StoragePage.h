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
    /* Current page */
    PageStruct page;

    /*
     * Page constructor
     *
     * @param address Page address in memory
     */
    Page(uint32_t address);

    /*
     * Page copy constructor
     *
     * @param other The page for copy
     */
    Page(const Page& other);

    /*
     * Destructor
     */
    ~Page() {}

    /*
     * Assignment operator
     *
     * @param other The header for copy
     */
    Page& operator=(const Page& other);

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

private:
    /*
     * Tries to repair the page
     */
    void repair();

};

/*
 * Header is a table of contents of the storage macroblock
 */
class Header: public Page
{
private:
    /* Header macroblock index in memory */
    uint32_t m_macroblockIndex;

    /* Single page meta status bits count */
    static const uint8_t STATUS_BITS_COUNT = 2;

protected:
    /*
     * Validates the header data
     *
     * @return Returns true if the header data is correct
     */
    bool validate() override;

public:
    /* Header page statuses */
    typedef enum _PageHeaderStatus {
        PAGE_OK      = static_cast<uint8_t>(0b01), // Data on page exists
        PAGE_EMPTY   = static_cast<uint8_t>(0b10), // Page is empty
        PAGE_BLOCKED = static_cast<uint8_t>(0b11), // Page is blocked for load and save by StorageAT library
    } PageStatus;

    /* Single page meta data structure */
    STORAGE_PACK(typedef struct, _MetaUnit {
    	// String page prefix for searching
        uint8_t  prefix[STORAGE_PAGE_PREFIX_SIZE];
        // ID for searching// ID for searching
        uint32_t id;
    } MetaUnit);

    /* Single page meta status structure */
    STORAGE_PACK(typedef struct, _MetaStatus {
        uint8_t  status; // Page status (PageStatus)

        /*
        * Checks that the target page status is set in the MetaStatus
        *
        * @param pageIndex The page index in current macroblock
        * @param status    Target status
        * @return          Returns true if the target page status is set
        */
        bool isStatus(uint32_t pageIndex, PageStatus targetStatus);

        /*
        * Sets the page status in the MetaStatus
        *
        * @param pageIndex The page index in current macroblock
        * @param status    Target status
        */
        void setStatus(uint32_t pageIndex, PageStatus targetStatus);


        /*
        * Returns the page status in the MetaStatus
        *
        * @param pageIndex The page index in current macroblock
        * @return          Returns page status
        */
        uint8_t getStatus(uint32_t pageIndex);
    } MetaStatus);

    /* Pages in block that header page contains */
    static const uint32_t PAGES_COUNT = (STORAGE_PAGE_PAYLOAD_SIZE * 8) / (sizeof(struct _MetaUnit) * 8 + STATUS_BITS_COUNT);

    /* Statuses count in byte */
    static const uint32_t BYTE_STATUSES_COUNT = 8 / STATUS_BITS_COUNT;

    /* Header page payload data statuses count */
    static const uint32_t STATUSES_COUNT = PAGES_COUNT / BYTE_STATUSES_COUNT + (PAGES_COUNT % BYTE_STATUSES_COUNT ? 1 : 0);

    /* Header page payload data */
    STORAGE_PACK(typedef struct, _HeaderMeta {
        MetaUnit   metaUnits[PAGES_COUNT];       // Macroblock page meta units
        MetaStatus metaStatuses[STATUSES_COUNT]; // Macroblock page meta statuses
    } HeaderMeta);

    /* Pointer to payload header data */
    HeaderMeta* data;

    /*
     * Header constructor
     *
     * @param address Header address in memory
     */
    Header(uint32_t address);

    /*
     * Header copy constructor
     *
     * @param other The header for copy
     */
    Header(const Header& other);

    /*
     * Header destructor
     */
    ~Header();

    /*
     * Header assignment operator
     *
     * @param other The header for copy
     */
    Header& operator=(const Header& other);

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
     * Sets the page status in the header
     *
     * @param pageIndex The page index in current macroblock
     * @param status    Target status
     */
    void setPageStatus(uint32_t pageIndex, PageStatus status);

    /*
     * Checks that the target page status is set in the header
     *
     * @param pageIndex The page index in current macroblock
     * @param status    Target status
     * @return          Returns true if the target page status is set
     */
    bool isPageStatus(uint32_t pageIndex, PageStatus status);

    /*
     * Sets the page blocked status in the header
     *
     * @param targetAddress Page address
     */
    void setAddressBlocked(uint32_t targetAddress);

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

};
