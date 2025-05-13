/* Copyright © 2025 Georgy E. All rights reserved. */

#ifndef _STORAGE_PAGE_H_
#define _STORAGE_PAGE_H_


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
     * Prepares the page for saving
     */
    void prepareSave();

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
     * Sets page address of the data
     *
     * @param address page address
     */
    void setAddress(uint32_t address);

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

    /*
     * @return Returns previously page address
     */
    uint32_t getPrevAddress();

    /*
     * @param Returns next page address
     */
    uint32_t getNextAddress();

    /*
     * Validates the page data
     *
     * @return Returns true if the page data is correct
     */
    bool validate();

    // TODO: docs
    bool empty();

protected:
    /* Page address */
    uint32_t address;

    /*
     * Calculates the page data CRC16
     *
     * @return Returns CRC16 of the page data
     */
    uint16_t getCRC16(uint8_t* buf, uint16_t len);

    // TODO: docs
    Page() {}

private:
    /*
     * Tries to repair the page
     */
    void repair();

};

/*
 * Header is a table of contents of the storage macroblock
 */
class Header: protected Page
{
private:
    // TODO: docs
    static constexpr uint8_t EMPTY_PREFIX[] = { 0xFF, 0xFF, 0xFF };
    static constexpr uint8_t BLOCK_PREFIX[] = { 0x00, 0x00, 0x00 };

    /* Header macroblock index in memory */
    uint32_t m_macroblockIndex;

public:
    /* Single page meta data structure */
#ifdef __GNUC__
    typedef struct __attribute__((__packed__)) _MetaUnit {
        // String page prefix for searching
        uint8_t  prefix[STORAGE_PAGE_PREFIX_SIZE];
        // ID for searching// ID for searching
        uint32_t id;
    } MetaUnit;
#elif _MSC_VER
#   pragma pack(push, 1)
    typedef struct _MetaUnit {
        // String page prefix for searching
        uint8_t  prefix[STORAGE_PAGE_PREFIX_SIZE];
        // ID for searching// ID for searching
        uint32_t id;
    } MetaUnit;
#   pragma pack(pop)
#endif

    /* Pages in block that header page contains */
    static const uint32_t PAGES_COUNT = STORAGE_HEADER_PAYLOAD_SIZE / sizeof(struct _MetaUnit);

    /* Header page payload data */
#ifdef __GNUC__
    typedef struct __attribute__((__packed__)) _HeaderMeta {
        // Header block flag
        uint8_t  block;
        // Macroblock page meta units
        MetaUnit metaUnits[PAGES_COUNT];
    } HeaderMeta;
#elif _MSC_VER
#   pragma pack(push, 1)
    typedef struct _HeaderMeta {
        // Header block flag
        uint8_t  block;
        // Macroblock page meta units
        MetaUnit metaUnits[PAGES_COUNT];
    } HeaderMeta;
#   pragma pack(pop)
#endif

    // TODO: docs
    HeaderStruct* header;

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
    // StorageStatus load(bool) override { return this->load(); };

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
     * Sets the page blocked status in the header
     *
     * @param targetAddress Page address
     */
    void setAddressBlocked(uint32_t targetAddress);

    /*
     * Checks that the target page is blocked
     *
     * @param targetAddress The page address
     * @return              Returns true if the target page is blocked
     */
    bool isAddressBlocked(uint32_t targetAddress);

    /*
     * Sets the page empty status in the header
     *
     * @param targetAddress Page address
     */
    void setAddressEmpty(uint32_t targetAddress);

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

    /*
     * Validates the header data
     *
     * @return Returns true if the page data is correct
     */
    bool validate();

    /*
     * @return Returns the header address
     */
    uint32_t getAddress();

    /*
     * Sets header address
     *
     * @param address header address
     */
    void setAddress(uint32_t address);

    // TODO: docs
    void prepareSave();
    bool exists(uint32_t address);
};


#endif
