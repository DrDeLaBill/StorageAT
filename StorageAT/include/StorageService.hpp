/* Copyright © 2025 Georgy E. All rights reserved. */

#ifndef _STORAGE_SERVICE_H_
#define _STORAGE_SERVICE_H_


#include "StorageAT.h"
#include "StorageType.h"

#include "Timer.h"
#include "GQueue.hpp"

#include "fsm_gc.h"


namespace prvt_st_at {

class StorageService 
{
public:
	using AT = StorageAT;

    enum status_t {
        ST_READY = 0,
        ST_FIND,
        ST_READ,
        ST_WRITE,
        ST_REWRITE,
        ST_DELETE,
        ST_HEADER
    };

    struct route_t {
        status_t        status;
        uint32_t        addr;
        uint32_t        len;
        uint32_t        cnt;
        char            prefix[STORAGE_PAGE_PREFIX_SIZE];
        uint32_t        id;
        uint8_t*        dst;
        uint8_t*        src;
        StorageFindMode mode;
        uint32_t        sub_cnt;
    };

    static utl::GQueue<16, route_t> m_queue;
    static uint32_t                 m_addrs[Header::PAGES_COUNT];
    static Page                     m_page;
    static Header                   m_header;
    static utl::Timer               m_timer;
    static StorageStatus            m_result;
	static StorageAT::callback_t    m_callback;
    
    static void init();

    static void tick();

    static void callback(StorageStatus status);

    static bool ready();

    static void reset();

    static void route(route_t& route);

    static StorageStatus asyncFind(
		StorageFindMode       mode,
		uint32_t*             address,
		StorageAT::callback_t callback,
		const char*           prefix = "",
		uint32_t              id = 0
    );

    static StorageStatus asyncLoad(uint32_t address, uint8_t* data, uint32_t len, StorageAT::callback_t callback);
    
    StorageStatus asyncSave(
        uint32_t       address,
        const char*    prefix,
        uint32_t       id,
        uint8_t*       data,
        uint32_t       len,
		AT::callback_t callback
    );
    
    StorageStatus asyncRewrite(
        uint32_t       address,
        const char*    prefix,
        uint32_t       id,
        uint8_t*       data,
        uint32_t       len,
		AT::callback_t callback
    );
};

}


#endif
