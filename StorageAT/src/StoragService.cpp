/* Copyright © 2025 Georgy E. All rights reserved. */

#include "StorageService.hpp"

#include "fsm_gc.h"


StorageService::process_status StorageService::status = StorageService::READY;

static StorageFindMode StorageService::mode = FIND_MODE_EQUAL;
static uint32_t        StorageService::address = 0;
static uint8_t         StorageService::prefix[STORAGE_PAGE_PREFIX_SIZE] = "";
static uint8_t*        StorageService::dst = nullptr;
static uint32_t        StorageService::id = 0;
static uint32_t        StorageService::len = 0;


FSM_GC_CREATE(st_at_fsm)

FSM_GC_CREATE_EVENT(done_e,    0)
FSM_GC_CREATE_EVENT(find_e,    0)
FSM_GC_CREATE_EVENT(read_e,    0)
FSM_GC_CREATE_EVENT(write_e,   0)
FSM_GC_CREATE_EVENT(erase_e,   0)
FSM_GC_CREATE_EVENT(success_e, 0)
FSM_GC_CREATE_EVENT(error_e,   1)

FSM_GC_CREATE_STATE(init_s,   _init_s)
FSM_GC_CREATE_STATE(router_s, _router_s)

FSM_GC_CREATE_TABLE(
    st_at_fsm_table,
    {&init_s, &done_e, &router_s, NULL}
)


void StorageService::init()
{
    fsm_gc_init(&st_at_fsm, st_at_fsm_table, __arr_len(st_at_fsm_table));
    reset();
}

void StorageService::tick()
{
    fsm_gc_process(&st_at_fsm);
}

void StorageService::reset()
{
    fsm_gc_reset(&st_at_fsm);
    status = READY;
}

StorageService::process_status StorageService::getStatus()
{
    return status;
}

StorageStatus StorageService::asyncFind(
    StorageFindMode mode,
    uint32_t*       address,
    const char*     prefix = "",
    uint32_t        id = 0
) {
    if (status != READY) {
        return STORAGE_BUSY;
    }
    StorageService::status  = WAIT;
    StorageService::mode    = mode;
    StorageService::address = *address;
    StorageService::dst     = address;
    StorageService::id      = id;
    
    memcpy(StorageService::prefix, prefix, STORAGE_PAGE_PREFIX_SIZE);

    return STORAGE_OK;
}