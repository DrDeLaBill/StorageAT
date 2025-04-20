/* Copyright © 2025 Georgy E. All rights reserved. */

#ifndef _STORAGE_SERVICE_H_
#define _STORAGE_SERVICE_H_


#include "StorageType.h"


class StorageService 
{
private:
    enum process_status {
        READY = 0,
        WAIT,
        SUCCESS,
        ERROR
    };

    static process_status status;
    
    static StorageFindMode mode;
    static uint32_t        address;
    static uint8_t         prefix[STORAGE_PAGE_PREFIX_SIZE];
    static uint8_t*        dst;
    static uint32_t        id;
    static uint32_t        len;

public:
    static void init();

    static void tick();

    static void reset();
    static _st_at::process_status getStatus();

    static StorageStatus asyncFind(
		StorageFindMode mode,
		uint32_t*       address,
		const char*     prefix = "",
		uint32_t        id = 0
    );
};


#endif