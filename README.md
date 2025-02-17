<p align="center">
  <h1 align="center">StorageAT</h1>
</p>

<p align="right">
  <img alt="Test passing" src="https://github.com/DrDeLaBill/StorageAT/actions/workflows/build-test.yml/badge.svg?branch=main">
</p>


## Language
- [English](README.md)
- [Русский](README.ru.md)

## Description

StorageAT is an allocation table (file system) library for memory of embedded devices.

Navigation occurs through the use of a prefix and an identifier that each record has. The prefix is used to determine the purpose of the data and the identifier is used to number them.

## Allocation table structure

The memory is divided into macroblocks of equal size (Figure 1). Macroblocks ares navigated using indexes starting from 0.

<img src="https://github.com/DrDeLaBill/StorageAT/assets/40359652/da24a984-cd57-42ee-b1b4-8728b4f9a375">
<p align="center">Figure 1</p>

Each macroblock has a table of contents (Header) and a data storage area (User data), their sizes are fixed. The table of contents and the data storage area are divided into a fixed number of pages of 256 bytes. The data storage area is navigated using indexes starting from 0. The markup is shown in Figure 2.

<img src="https://github.com/DrDeLaBill/StorageAT/assets/40359652/7c6230a3-a5bb-4c65-8a0a-4714986a6588">
<p align="center">Figure 2</p>

The Page is the minimum operaded unit in the data allocation table, it has a meta-data area and a user data area. 
Each page (including macroblock header) saves with specially generated meta-data. Page code - defines the beginning of the page, Version - the library version that was used when writing the page, Prev page address and Next page address - the linked pages addresses, if the previously saved data exceeded the size of the area of the page, prefix and identifier - special data set by user (also stored in the macroblock header) that is uses to search for specific data in memory, CRC16 - checksum of the page (Figure 3).

<img src="https://github.com/DrDeLaBill/StorageAT/assets/40359652/8636232b-68e4-49b3-bb6f-1a5a3f945f14">
<p align="center">Figure 3</p>

In the header of macroblock is reserved 4 pages for the storing its table of contents. The structure of the header is identical to the page structure (Figure 3), but header user data area is uses by allocation table library and divides into two parts that are used for quick navigation. First part - array of prefix-identifier pairs, the index of each element in the header corresponds to the index of the page in the macroblock; second part - array of 2-bit values of the current page status (0b01 - data exists, 0b10 - page empty, 0b11 - page blocked), the index of each element in the header corresponds to the index of the page in the macroblock. The markup is shown in Figure 4.

<img src="https://github.com/DrDeLaBill/StorageAT/assets/40359652/ce123792-f612-47a1-8380-78ef3ffe1973">
<p align="center">Figure 4</p>

## API

The entire API, after processing the request, returns the execution status:
* STORAGE_OK - Successful exit code
* STORAGE_ERROR - Interanal error
* STORAGE_BUSY - Physical drive is busy
* STORAGE_OOM - Out of memory
* STORAGE_NOT_FOUND - Data was not found on physical drive
* STORAGE_DATA_EXISTS - Data already exists on current address
* STORAGE_HEADER_ERROR - Error with header data saving

For any manipulation of the memory uses a 32-bit address, so to save or/and load data you must perform a search in the appropriate mode.

Find function - finds data in storage
* mode - search mode
    * FIND_MODE_EQUAL - searches for data with equal prefix and identifier
    * FIND_MODE_NEXT - searches for data that has an identifier next to the current, with a prefix identical current
    * FIND_MODE_MIN - searches for data that has an minimum identifier, with a prefix identical current
    * FIND_MODE_MAX - searches for data that has an maximum identifier, with a prefix identical current
    * FIND_MODE_EMPTY - searches for empty page
* address - the variable in which the result will be written
* prefix - data prefix
* id - data identifier
```c++
StorageStatus find(
    StorageFindMode mode,
    uint32_t*       address,
    const char*     prefix = "",
    uint32_t        id = 0
);
```

Load function - loads the data from the storage by address
* address - storage page address to load
* data - the variable in which the result will be written
* len - the result variable length in bytes
```c++
StorageStatus load(uint32_t address, uint8_t* data, uint32_t len);
```

Save function - saves data to memory at the specified address if the memory area is not occupied or has an identical prefix and identifier
* address - storage page address to save
* prefix - data prefix
* id - data identifier
* data - data to save
* len - the result variable length in bytes
```c++
StorageStatus save(
    uint32_t    address,
    const char* prefix,
    uint32_t    id,
    uint8_t*    data,
    uint32_t    len
);
```

Rewrite function - saves data to memory at the specified address in any case
* address - storage page address to save
* prefix - data prefix
* id - data identifier
* data - data to save
* len - the result variable length in bytes
```c++
StorageStatus rewrite(
    uint32_t    address,
    const char* prefix,
    uint32_t    id,
    uint8_t*    data,
    uint32_t    len
);
```

Format function - formats all the memory
```c++
StorageStatus format();
```

Delete function - deletes data from the header
* address - storage page address to delete
```c++
StorageStatus deleteData(uint32_t address);
```

Returns page count in the memory
```c++
static uint32_t getStoragePagesCount();
```

Returns page size in bytes
```c++
static uint32_t getStorageSize();
```

<p align="center">
  <h2 align="center">How to work with the data allocation table</h2>
</p>

### 1. Create driver

To interact with the table, you need to create a physical memory read-write driver, with extending from the IStorageDriver interface
```c++
class StorageDriver: public IStorageDriver
{
public: 
    StorageDriver() {}
	StorageStatus read(uint32_t address, uint8_t* data, uint32_t len) override
    {
        DriverStatus status = memory_read(address, data, len);
        if (status == DRIVER_BUSY) {
            return STORAGE_BUSY;
        }
        if (status == DRIVER_OOM) {
            return STORAGE_OOM;
        }
        if (status == DRIVER_ERROR) {
            return STORAGE_ERROR;
        }
        return STORAGE_OK;
    }
	StorageStatus write(uint32_t address, uint8_t* data, uint32_t len) override
    {
        DriverStatus status = memory_write(address, data, len);
        if (status == DRIVER_BUSY) {
            return STORAGE_BUSY;
        }
        if (status == DRIVER_OOM) {
            return STORAGE_OOM;
        }
        if (status == DRIVER_ERROR) {
            return STORAGE_ERROR;
        }
        return STORAGE_OK;
    }
    StorageStatus erase(const uint32_t* addresses, const uint32_t len) override
    {
        DriverStatus status = memory_erase(addresses, len);
        if (status == DRIVER_BUSY) {
            return STORAGE_BUSY;
        }
        if (status == DRIVER_OOM) {
            return STORAGE_OOM;
        }
        if (status == DRIVER_ERROR) {
            return STORAGE_ERROR;
        }
        return STORAGE_OK;
    }
};
```

### 2. Create allocation table object

```c++
StorageAT storage(
    memory.getPagesCount(), // Physical memory pages count
    (new StorageDriver())   // Storage driver
);
```

### 3. Allocation table usage

Memory formatting:
```c++
storage.format();
```

Saving data to the memory:
```c++
uint32_t address = 0;
uint8_t data[1000] = { 1, 2, 3, 4, 5 };

StorageStatus status = storage.find(FIND_MODE_EMPTY, &address);
if (status == STORAGE_OK) {
    storage.save(address, "DAT", 1, data, sizeof(data));
}
```

Loading data from the memory:
```c++
uint32_t address = 0;
uint8_t data[1000] = { };

StorageStatus status = storage.find(FIND_MODE_EQUAL, &address, "DAT", 1);
if (status == STORAGE_OK) {
    storage.load(address, data, sizeof(data));
}
```

Rewrite data to the memory:
```c++
uint32_t address = 0;
uint8_t data1[1000] = { 1, 2, 3, 4, 5 };

StorageStatus status = storage.find(FIND_MODE_EMPTY, &address);
if (status == STORAGE_OK) {
    storage.save(address, "DAT", 1, data1, sizeof(data1));
    uint8_t data2[1000] = { 6, 7, 8, 9, 10 }; 
    storage.rewrite(address, "DAT", 2, data2, sizeof(data2));
}
```

