/* Copyright © 2025 Georgy E. All rights reserved. */

#include "StorageService.hpp"

#include "StorageAT.h"

#include "gutils.h"
#include "fsm_gc.h"


#define STORAGE_DELAY_MS (100)


using namespace prvt_st_at;
using AT = StorageAT;
using SV = StorageService;
using SM = StorageMacroblock;


utl::GQueue<16, SV::route_t> SV::m_queue;
uint32_t                     SV::m_addrs[Header::PAGES_COUNT] = {};
Page                         SV::m_page(0);
Header                       SV::m_header(0);
utl::Timer                   SV::m_timer(STORAGE_DELAY_MS);
StorageStatus                SV::m_result = STORAGE_OK;
AT::callback_t               SV::m_callback = nullptr;


FSM_GC_CREATE(st_at_fsm)
FSM_GC_CREATE(st_at_find_fsm)
FSM_GC_CREATE(st_at_read_fsm)
FSM_GC_CREATE(st_at_write_fsm)
FSM_GC_CREATE(st_at_rewrite_fsm)
FSM_GC_CREATE(st_at_delete_fsm)
FSM_GC_CREATE(st_at_header_fsm)

FSM_GC_CREATE_EVENT(done_e,    0)
FSM_GC_CREATE_EVENT(router_e,  0)
FSM_GC_CREATE_EVENT(find_e,    0)
FSM_GC_CREATE_EVENT(read_e,    0)
FSM_GC_CREATE_EVENT(write_e,   0)
FSM_GC_CREATE_EVENT(rewrite_e, 0)
FSM_GC_CREATE_EVENT(delete_e,  0)
FSM_GC_CREATE_EVENT(header_e,  0)
FSM_GC_CREATE_EVENT(next_e,    0)
FSM_GC_CREATE_EVENT(block_e,   0)
FSM_GC_CREATE_EVENT(end_e,     0)
FSM_GC_CREATE_EVENT(success_e, 1)
FSM_GC_CREATE_EVENT(timeout_e, 2)
FSM_GC_CREATE_EVENT(error_e,   3)

FSM_GC_CREATE_ACTION(router_a,   _router_a)
FSM_GC_CREATE_ACTION(callback_a, _callback_a)
FSM_GC_CREATE_ACTION(find_a,     _find_a)
FSM_GC_CREATE_ACTION(read_a,     _read_a)
FSM_GC_CREATE_ACTION(write_a,    _write_a)
FSM_GC_CREATE_ACTION(rewrite_a,  _rewrite_a)
FSM_GC_CREATE_ACTION(delete_a,   _delete_a)
FSM_GC_CREATE_ACTION(header_a,   _header_a)

FSM_GC_CREATE_STATE(init_s,      _init_s)
FSM_GC_CREATE_STATE(router_s,    _router_s)
FSM_GC_CREATE_STATE(find_s,      _find_s)
FSM_GC_CREATE_STATE(read_s,      _read_s)
FSM_GC_CREATE_STATE(write_s,     _write_s)
FSM_GC_CREATE_STATE(rewrite_s,   _rewrite_s)
FSM_GC_CREATE_STATE(delete_s,    _delete_s)
FSM_GC_CREATE_STATE(header_s,    _header)

FSM_GC_CREATE_TABLE(
    st_at_fsm_table,
    {&init_s,    &done_e,   &router_s,  &router_a},

    {&router_s,  &find_e,    &find_s,   &find_a},
    {&router_s,  &read_e,    &read_s,   &read_a},
    {&router_s,  &write_e,   &write_s,  &write_a},
    {&router_s,  &rewrite_e, &write_s,  &write_a},
    {&router_s,  &delete_e,  &delete_s, &delete_a},
    {&router_s,  &header_e,  &header_s, &header_a},
    {&router_s,  &error_e,   &router_s, &router_a},

    {&find_s,    &done_e,    &router_s, &callback_a},
    {&find_s,    &router_e,  &router_s, &router_a},

    {&read_s,    &done_e,    &router_s, &callback_a},
    {&read_s,    &router_e,  &router_s, &router_a},

    {&write_s,   &done_e,    &router_s, &callback_a},
    {&write_s,   &router_e,  &router_s, &router_a},

    {&rewrite_s, &done_e,    &router_s, &callback_a},
    {&rewrite_s, &router_e,  &router_s, &router_a},

    {&delete_s,  &done_e,    &router_s, &callback_a},
    {&delete_s,  &router_e,  &router_s, &router_a},

    {&header_s,  &done_e,    &router_s, &callback_a},
    {&header_s,  &router_e,  &router_s, &router_a},
)


void SV::init()
{
    fsm_gc_init(&st_at_fsm, st_at_fsm_table, __arr_len(st_at_fsm_table));
    fsm_gc_disable_messages(&st_at_fsm); // TODO: uncomment 
    reset();
}

void SV::tick()
{
    fsm_gc_process(&st_at_fsm);
}

void SV::callback(StorageStatus status)
{
    if (m_queue.empty()) {
        return;
    }
    fsm_gc_event_t* event = &done_e;
    if (status != STORAGE_OK) {
        event = &error_e;
    }
    switch (m_queue.back().status)
    {
    case ST_FIND:
        fsm_gc_push_event(&st_at_find_fsm, event);
        break;
    case ST_READ:
        fsm_gc_push_event(&st_at_read_fsm, event);
        break;
    case ST_WRITE:
        fsm_gc_push_event(&st_at_write_fsm, event);
        break;
    case ST_REWRITE:
        fsm_gc_push_event(&st_at_rewrite_fsm, event);
        break;
    case ST_DELETE:
        fsm_gc_push_event(&st_at_delete_fsm, event);
        break;
    case ST_HEADER:
        fsm_gc_push_event(&st_at_header_fsm, event);
        break;
    case ST_READY:
    default:
        fsm_gc_push_event(&st_at_fsm, event);
        break;
    }
}

bool SV::ready()
{
    return m_queue.empty();
}

void SV::reset()
{
    fsm_gc_reset(&st_at_fsm);
    fsm_gc_reset(&st_at_find_fsm);
    fsm_gc_reset(&st_at_read_fsm);
    fsm_gc_reset(&st_at_write_fsm);
    fsm_gc_reset(&st_at_rewrite_fsm);
    fsm_gc_reset(&st_at_delete_fsm);
    fsm_gc_reset(&st_at_header_fsm);
    tick();
}

void SV::route(route_t& route)
{
    if (SV::m_queue.full()) {
        SV::m_result = STORAGE_ERROR;
        fsm_gc_push_event(&st_at_fsm, &done_e);
    } else {
        SV::m_queue.push(route);
        fsm_gc_push_event(&st_at_fsm, &router_e);
    }
}

StorageStatus SV::asyncFind(
    StorageFindMode       mode,
    uint32_t*             address,
    AT::callback_t callback,
    const char*           prefix,
    uint32_t              id
) {
    if (!ready()) {
        return STORAGE_BUSY;
    }
    if (!address || !callback) {
        return STORAGE_ERROR;
    }
    if (mode != FIND_MODE_EMPTY && !prefix) {
        return STORAGE_ERROR;
    }
    if (mode > FIND_MODE_EMPTY) {
        return STORAGE_ERROR;
    }
    m_callback = callback;
    m_result = STORAGE_OK;
    route_t route{
        ST_FIND,
        *address,
        0,
        0,
        {},
        id,
        (uint8_t*)address,
        nullptr,
        mode,
		0
    };
    memcpy(route.prefix, prefix, __min(STORAGE_PAGE_PREFIX_SIZE, strlen(prefix)));
    SV::route(route);
    return STORAGE_OK;
}

StorageStatus SV::asyncLoad(uint32_t address, uint8_t* data, uint32_t len, AT::callback_t callback)
{
    if (!ready()) {
        return STORAGE_BUSY;
    }
    if (address % STORAGE_PAGE_SIZE > 0) {
        return STORAGE_ERROR;
    }
    if (!data || !callback || !len) {
        return STORAGE_ERROR;
    }
    if (address + len >= AT::getStorageSize()) {
        return STORAGE_OOM;
    }
    m_callback = callback;
    m_result = STORAGE_OK;
    route_t route{
        ST_READ,
        address,
        len,
        0,
        {},
        0,
        data,
        nullptr,
        (StorageFindMode)0,
		0
    };
    SV::route(route);
    return STORAGE_OK;
}

StorageStatus SV::asyncSave(
    uint32_t       address,
    const char*    prefix,
    uint32_t       id,
    uint8_t*       data,
    uint32_t       len,
    AT::callback_t callback
) {
    if (!ready()) {
        return STORAGE_BUSY;
    }
    if (address % STORAGE_PAGE_SIZE > 0) {
        return STORAGE_ERROR;
    }
    if (!data || !callback || !len || !prefix) {
        return STORAGE_ERROR;
    }
    if (address + len >= AT::getStorageSize()) {
        return STORAGE_OOM;
    }
    if (SM::isMacroblockAddress(address)) {
        return STORAGE_ERROR;
    }
    m_callback = callback;
    m_result = STORAGE_OK;
    route_t route{
        ST_WRITE,
        address,
        len,
        0,
        {},
        id,
        nullptr,
        data,
        (StorageFindMode)0,
		0
    };
    memcpy(route.prefix, prefix, __min(STORAGE_PAGE_PREFIX_SIZE, strlen(prefix)));
    SV::route(route);
    return STORAGE_OK;
}

StorageStatus SV::asyncRewrite(
    uint32_t              address,
    const char*           prefix,
    uint32_t              id,
    uint8_t*              data,
    uint32_t              len,
	AT::callback_t callback
) {
    if (!ready()) {
        return STORAGE_BUSY;
    }
    if (address % STORAGE_PAGE_SIZE > 0) {
        return STORAGE_ERROR;
    }
    if (!data || !callback || !len || !prefix) {
        return STORAGE_ERROR;
    }
    if (address + len >= AT::getStorageSize()) {
        return STORAGE_OOM;
    }
    if (SM::isMacroblockAddress(address)) {
        return STORAGE_ERROR;
    }
    m_callback = callback;
    m_result = STORAGE_OK;
    route_t route{
        ST_REWRITE,
        address,
        len,
        0,
        {},
        id,
        nullptr,
        data,
        (StorageFindMode)0,
		0
    };
    memcpy(route.prefix, prefix, __min(STORAGE_PAGE_PREFIX_SIZE, strlen(prefix)));
    SV::route(route);
    return STORAGE_OK;
}

void _init_s()
{
    fsm_gc_push_event(&st_at_fsm, &done_e);
}

void _router_a() {}

void _router_s()
{
    if (SV::m_queue.empty()) {
        return;
    }
    switch (SV::m_queue.back().status) {
    case SV::ST_FIND:
        fsm_gc_push_event(&st_at_fsm, &find_e);
        break;
    case SV::ST_READ:
        fsm_gc_push_event(&st_at_fsm, &read_e);
        break;
    case SV::ST_WRITE:
        fsm_gc_push_event(&st_at_fsm, &write_e);
        break;
    case SV::ST_REWRITE:
        fsm_gc_push_event(&st_at_fsm, &rewrite_e);
        break;
    case SV::ST_DELETE:
        fsm_gc_push_event(&st_at_fsm, &delete_e);
        break;
    case SV::ST_HEADER:
        fsm_gc_push_event(&st_at_fsm, &header_e);
        break;
    case SV::ST_READY:
    default:
        break;
    }
}

void _callback_a()
{
    SV::route_t route = SV::m_queue.pop();
    if (SV::m_result != STORAGE_OK && SV::m_queue.count() > 1) {
        while (SV::m_queue.count() > 1) {
            SV::m_queue.pop();
        }
        route = SV::m_queue.pop();
        SV::reset();
    }
    switch (route.status) {
    case SV::ST_FIND:
    case SV::ST_READ:
    case SV::ST_WRITE:
    case SV::ST_REWRITE:
    case SV::ST_DELETE:
    case SV::ST_HEADER:
        if (SV::m_queue.empty()) {
            SV::m_callback(SV::m_result);
        }
        break;
    case SV::ST_READY:
    default:
        SV::reset();
        break;
    }
}

FSM_GC_CREATE_ACTION(find_read_a, _find_read_a)

FSM_GC_CREATE_STATE(find_init_s,  _find_init_s)
FSM_GC_CREATE_STATE(find_read_s,  _find_init_s)

FSM_GC_CREATE_TABLE(
    st_at_find_fsm_table,
    {&find_init_s, &success_e, &find_read_s, &find_read_a}
)

void _find_a() {}

void _find_s()
{
    static bool initialized = false;
    if (!initialized) {
        fsm_gc_init(&st_at_find_fsm, st_at_find_fsm_table, __arr_len(st_at_find_fsm_table));
        initialized = true;
    }
    fsm_gc_process(&st_at_find_fsm);
}

void _find_init_s()
{
    SV::route_t& route = SV::m_queue.back();
    route.cnt = 0;
    fsm_gc_clear(&st_at_find_fsm);
    fsm_gc_push_event(&st_at_find_fsm, &success_e);
}

void _find_read_a()
{
    SV::route_t& route = SV::m_queue.back();
}

void _find_read_s()
{

}


FSM_GC_CREATE_ACTION(read_read_a,    _read_read_a)
FSM_GC_CREATE_ACTION(read_check_a,   _read_check_a)
FSM_GC_CREATE_ACTION(read_delete_a,  _read_delete_a)
FSM_GC_CREATE_ACTION(read_success_a, _read_success_a)
FSM_GC_CREATE_ACTION(read_error_a,   _read_error_a)
 
FSM_GC_CREATE_STATE(read_init_s,     _read_init_s)
FSM_GC_CREATE_STATE(read_read_s,     _read_read_s)
FSM_GC_CREATE_STATE(read_delete_s,   _read_delete_s)

FSM_GC_CREATE_TABLE(
    st_at_read_fsm_table,
    {&read_init_s,   &success_e, &read_read_s,   &read_read_a},
 
    {&read_read_s,   &success_e, &read_init_s,   &read_success_a},
    {&read_read_s,   &done_e,    &read_read_s,   &read_check_a},
    {&read_read_s,   &next_e,    &read_read_s,   &read_read_a},
    {&read_read_s,   &delete_e,  &read_delete_s, &read_delete_a},
    {&read_read_s,   &error_e,   &read_init_s,   &read_error_a},

    {&read_delete_s, &done_e,    &read_init_s,   &read_error_a},
)


void _read_a() {}

void _read_s()
{
    static bool initialized = false;
    if (!initialized) {
        fsm_gc_init(&st_at_read_fsm, st_at_read_fsm_table, __arr_len(st_at_read_fsm_table));
        initialized = true;
    }
    fsm_gc_process(&st_at_read_fsm);
}

void _read_init_s() 
{
    SV::route_t& route = SV::m_queue.back();
    route.cnt = 0;
    fsm_gc_push_event(&st_at_read_fsm, &success_e);
}    

void _read_read_a() 
{
    SV::route_t& route = SV::m_queue.back();
    if (SM::isMacroblockAddress(route.addr)) {
        fsm_gc_push_event(&st_at_read_fsm, &error_e);
    } else if (route.addr + sizeof(SV::m_page.page) > AT::getStorageSize()) {
        SV::m_result = STORAGE_OOM;
        fsm_gc_push_event(&st_at_read_fsm, &error_e);
    } else {
        StorageStatus status = AT::driverCallback()->asyncRead(route.addr, (uint8_t*)&SV::m_page.page, sizeof(SV::m_page.page));
        if (status == STORAGE_OK) {
            SV::m_timer.start();
        } else {
            SV::m_result = status;
            fsm_gc_push_event(&st_at_read_fsm, &error_e);
        } 
    }
    route.sub_cnt = route.addr;
}

void _read_check_a()
{
    SV::route_t& route = SV::m_queue.back();
    if (!SV::m_page.validate()) {
        // TODO: SV::m_page.repair();
    }
    if (!SV::m_page.validate()) {
        SV::m_result = STORAGE_NOTVALID_ERROR;
        fsm_gc_push_event(&st_at_read_fsm, &delete_e);
        return;
    }
    if (!route.cnt && !SV::m_page.isStart()) {
        fsm_gc_push_event(&st_at_read_fsm, &delete_e);
        return;
    }
    if (!route.cnt) {
        memcpy(route.prefix, SV::m_page.page.header.prefix, sizeof(route.prefix));
        route.id = SV::m_page.page.header.id;
    }
    if (memcmp(route.prefix, SV::m_page.page.header.prefix, sizeof(route.prefix))) {
        fsm_gc_push_event(&st_at_read_fsm, &delete_e);
        return;
    }
    if (route.id != SV::m_page.page.header.id) {
        fsm_gc_push_event(&st_at_read_fsm, &delete_e);
        return;
    }
    uint32_t cnt = __min(sizeof(SV::m_page), route.len - route.cnt);
    memcpy(route.dst + route.cnt, (uint8_t*)(&SV::m_page), cnt);
    route.cnt += cnt;
    if (route.cnt == route.len && SV::m_page.isEnd()) {
        fsm_gc_push_event(&st_at_read_fsm, &success_e);
    } else if (route.cnt >= route.len) {
        fsm_gc_push_event(&st_at_read_fsm, &delete_e);
        return;
    }
    route.addr = SV::m_page.page.header.next_addr;
    fsm_gc_push_event(&st_at_read_fsm, &next_e);
}

void _read_read_s()
{
    if (SV::m_timer.wait()) {
        return;
    }
    fsm_gc_push_event(&st_at_read_fsm, &error_e);
}

void _read_delete_a()
{
    SV::route_t& route = SV::m_queue.back();
    SV::route_t _delete{
        SV::ST_DELETE,
        route.addr,
        0,
        0,
        {},
        0,
        nullptr,
        nullptr,
        (StorageFindMode)0,
		0
    };
    SV::route(_delete);
    SV::m_timer.start();
}

void _read_delete_s()
{
    if (SV::m_timer.wait()) {
        return;
    }
    fsm_gc_push_event(&st_at_header_fsm, &done_e);
}

void _read_success_a()
{
    SV::m_result = STORAGE_OK;
    fsm_gc_clear(&st_at_read_fsm);
    fsm_gc_push_event(&st_at_fsm, &done_e);
}

void _read_error_a()
{
    if (SV::m_result == STORAGE_OK) {
        SV::m_result = STORAGE_ERROR;
    }
    fsm_gc_clear(&st_at_read_fsm);
    fsm_gc_push_event(&st_at_fsm, &done_e);
}


FSM_GC_CREATE_ACTION(write_read_a,    _write_read_a)
FSM_GC_CREATE_ACTION(write_setup_a,   _write_setup_a)
FSM_GC_CREATE_ACTION(write_rewrite_a, _write_rewrite_a)
FSM_GC_CREATE_ACTION(write_success_a, _write_success_a)
FSM_GC_CREATE_ACTION(write_error_a,   _write_error_a)

FSM_GC_CREATE_STATE(write_init_s,     _write_init_s)
FSM_GC_CREATE_STATE(write_read_s,     _write_read_s)
FSM_GC_CREATE_STATE(write_setup_s,    _write_setup_s)
FSM_GC_CREATE_STATE(write_rewrite_s,  _write_rewrite_s)

FSM_GC_CREATE_TABLE(
    st_at_write_fsm_table,
    {&write_init_s,    &success_e, &write_read_s,    &write_read_a},

    {&write_read_s,    &done_e,    &write_setup_s,   &write_setup_a},
    {&write_read_s,    &error_e,   &write_init_s,    &write_error_a},

    {&write_setup_s,   &success_e, &write_rewrite_s, &write_rewrite_a},
    {&write_setup_s,   &error_e,   &write_init_s,    &write_error_a},

    {&write_rewrite_s, &done_e,    &write_init_s,    &write_success_a},
    {&write_rewrite_s, &error_e,   &write_init_s,    &write_error_a},
)

void _write_a() {}

void _write_s() 
{
    static bool initialized = false;
    if (!initialized) {
        fsm_gc_init(&st_at_write_fsm, st_at_write_fsm_table, __arr_len(st_at_write_fsm_table));
        initialized = true;
    }
    fsm_gc_process(&st_at_write_fsm);
}

void _write_init_s() 
{
    fsm_gc_push_event(&st_at_write_fsm, &success_e);
}

void _write_read_a()
{
    SV::route_t& route = SV::m_queue.back();
    SV::route_t header{
        SV::ST_HEADER,
        route.addr,
        STORAGE_PAGE_SIZE,
        0,
        {},
        0,
        nullptr,
        nullptr,
        (StorageFindMode)0,
		0
    };
    SV::route(header);
    SV::m_timer.start();
}

void _write_read_s()
{
    if (SV::m_timer.wait()) {
        return;
    }
    fsm_gc_push_event(&st_at_write_fsm, &error_e);
}

void _write_setup()
{
    SV::route_t& route = SV::m_queue.back();
    Header::MetaUnit* metaUnitPtr = SV::m_header.data->metaUnits[SM::getPageIndexByAddress(route.addr)];
    if ((*metaUnitPtr).prefix[0] > 0 || (*metaUnitPtr).id) {
        fsm_gc_push_event(&st_at_write_fsm, &error_e);
    } else {
        fsm_gc_push_event(&st_at_write_fsm, &success_e);
    }
}

void _write_setup_s() {}

void _write_rewrite_a()
{
    SV::route_t& route = SV::m_queue.back();
    SV::route_t rewrite{
        SV::ST_REWRITE,
        route.addr,
        STORAGE_PAGE_SIZE,
        0,
        {},
        0,
        nullptr,
        route.src,
        (StorageFindMode)0,
		0
    };
    SV::route(rewrite);
    SV::m_timer.start();
}

void _write_rewrite_s()
{
    if (SV::m_timer.wait()) {
        return;
    }
    fsm_gc_push_event(&st_at_write_fsm, &error_e);
}

void _write_success_a()
{
    SV::m_result = STORAGE_OK;
    fsm_gc_clear(&st_at_write_fsm);
    fsm_gc_push_event(&st_at_fsm, &done_e);
}

void _write_error_a()
{
    if (SV::m_result == STORAGE_OK) {
        SV::m_result = STORAGE_ERROR;
    }
    fsm_gc_clear(&st_at_write_fsm);
    fsm_gc_push_event(&st_at_fsm, &done_e);
}


FSM_GC_CREATE_ACTION(rewrite_delete_a,     _rewrite_delete_a)
FSM_GC_CREATE_ACTION(rewrite_header_a,     _rewrite_header_a)
FSM_GC_CREATE_ACTION(rewrite_find_a,       _rewrite_find_a)
FSM_GC_CREATE_ACTION(rewrite_find_next_a,  _rewrite_find_next_a)
FSM_GC_CREATE_ACTION(rewrite_find_block_a, _rewrite_find_block_a)
FSM_GC_CREATE_ACTION(rewrite_save_a,       _rewrite_save_a)
FSM_GC_CREATE_ACTION(rewrite_setup_a,      _rewrite_setup_a)
FSM_GC_CREATE_ACTION(rewrite_block_a,      _rewrite_block_a)
FSM_GC_CREATE_ACTION(rewrite_hd_save_a,    _rewrite_hd_save_a)
FSM_GC_CREATE_ACTION(rewrite_back_a,       _rewrite_back_a)
FSM_GC_CREATE_ACTION(rewrite_success_a,    _rewrite_success_a)
FSM_GC_CREATE_ACTION(rewrite_error_a,      _rewrite_error_a)

FSM_GC_CREATE_STATE(rewrite_init_s,        _rewrite_init_s)
FSM_GC_CREATE_STATE(rewrite_delete_s,      _rewrite_delete_s)
FSM_GC_CREATE_STATE(rewrite_header_s,      _rewrite_header_s)
FSM_GC_CREATE_STATE(rewrite_find_s,        _rewrite_find_s)
FSM_GC_CREATE_STATE(rewrite_save_s,        _rewrite_save_s)
FSM_GC_CREATE_STATE(rewrite_setup_s,       _rewrite_setup_s)
FSM_GC_CREATE_STATE(rewrite_hd_save_s,     _rewrite_hd_save_s)
FSM_GC_CREATE_STATE(rewrite_back_s,        -rewrite_back_s)

FSM_GC_CREATE_TABLE(
    st_at_rewrite_fsm_table,  
    {&rewrite_init_s,      &success_e, &rewrite_delete_s,    &rewrite_delete_a},

    {&rewrite_delete_s,    &done_e,    &rewrite_header_s,    &rewrite_header_a},
    {&rewrite_delete_s,    &error_e,   &rewrite_init_s,      &rewrite_error_a},

    {&rewrite_header_s,    &header_e,  &rewrite_save_s,      &rewrite_save_a},
    {&rewrite_header_s,    &done_e,    &rewrite_find_s,      &rewrite_find_a},
    {&rewrite_header_s,    &error_e,   &rewrite_back_s,      &rewrite_error_a},

    {&rewrite_find_s,      &end_e,     &rewrite_init_s,      &rewrite_success_a},
    {&rewrite_find_s,      &done_e,    &rewrite_save_s,      &rewrite_save_a},
    {&rewrite_find_s,      &error_e,   &rewrite_back_s,      &rewrite_error_a},

    {&rewrite_save_s,      &done_e,    &rewrite_setup_s,     &rewrite_setup_a},
    {&rewrite_save_s,      &error_e,   &rewrite_setup_s,     &rewrite_block_a},

    {&rewrite_setup_s,     &next_e,    &rewrite_find_s,      &rewrite_find_next_a},
    {&rewrite_setup_s,     &block_e,   &rewrite_find_s,      &rewrite_find_block_a},
    {&rewrite_setup_s,     &header_e,  &rewrite_hd_save_s,   &rewrite_hd_save_a},

    {&rewrite_hd_save_s,   &done_e,    &rewrite_find_s,      &rewrite_find_next_a},
    {&rewrite_hd_save_s,   &error_e,   &rewrite_find_s,      &rewrite_find_next_a},

    {&rewrite_back_s,      &done_e,    &rewrite_init_s,      &rewrite_error_a},
    {&rewrite_back_s,      &error_e,   &rewrite_init_s,      &rewrite_error_a},
)

void _rewrite_a() {}

void _rewrite_s()
{
    static bool initialized = false;
    if (!initialized) {
        fsm_gc_init(&st_at_rewrite_fsm, st_at_rewrite_fsm_table, __arr_len(st_at_rewrite_fsm_table));
        initialized = true;
    }
    fsm_gc_process(&st_at_rewrite_fsm);
}

void _rewrite_init_s()
{
    SV::route_t& route = SV::m_queue.back();
    route.cnt = 0;
    route.sub_cnt = route.addr;
    fsm_gc_push_event(&st_at_rewrite_fsm, &success_e);
}

void _rewrite_delete_a()
{
    SV::route_t& route = SV::m_queue.back();
    SV::route_t _delete{
        SV::ST_DELETE,
        route.addr,
        0,
        0,
        {},
        0,
        nullptr,
        nullptr,
        (StorageFindMode)0,
		0
    };
    SV::route(_delete);
    SV::m_timer.start();
}

void _rewrite_delete_s()
{
    if (SV::m_timer.wait()) {
        return;
    }
    fsm_gc_push_event(&st_at_rewrite_fsm, &error_e);
}

void _rewrite_header_a()
{
    SV::route_t& route = SV::m_queue.back();
    if (SM::isMacroblockAddress(route.addr)) {
        route.sub_cnt = route.addr;
        fsm_gc_push_event(&st_at_fsm, &header_e);
        return;
    }
    SV::route_t header{
        SV::ST_HEADER,
        route.addr,
        STORAGE_PAGE_SIZE,
        0,
        {},
        0,
        nullptr,
        nullptr,
        (StorageFindMode)0,
		0
    };
    SV::route(header);
    SV::m_timer.start();
}

void _rewrite_header_s()
{
    if (SV::m_timer.wait()) {
        return;
    }
    fsm_gc_push_event(&st_at_rewrite_fsm, &error_e);
}

void _rewrite_find_a()
{
    SV::route_t& route = SV::m_queue.back();
    SV::m_page.setPrevAddress(route.addr);
    SV::route_t find{
        SV::ST_FIND,
        route.sub_cnt + STORAGE_PAGE_SIZE,
        STORAGE_PAGE_SIZE,
        0,
        {},
        0,
        nullptr,
        (uint8_t*)&route.sub_cnt,
        FIND_MODE_EMPTY,
		0
    };
    SV::route(find);
    SV::m_timer.start();
}

void _rewrite_find_next_a()
{
    if (route.cnt >= route.len) {
        fsm_gc_push_event(&st_at_rewrite_fsm, &end_e);
        SV::m_timer.start();
        return;
    }
    SV::route_t& route = SV::m_queue.back();
    route.sub_addr = route.addr;
    _rewrite_find_a();
    route.addr = route.sub_cnt;
}

void _rewrite_find_block_a()
{
    SV::route_t& route = SV::m_queue.back();
    route.addr = route.sub_addr;
    if (route.cnt) {
        route.cnt -= STORAGE_PAGE_SIZE;
    }
    _rewrite_find_next_a();
}

void _rewrite_find_s()
{
    if (SV::m_timer.wait()) {
        return;
    }
    fsm_gc_push_event(&st_at_rewrite_fsm, &error_e);
}

void _rewrite_save_a()
{
    SV::route_t& route = SV::m_queue.back();

    if (route.cnt + STORAGE_PAGE_PAYLOAD_SIZE < route.len) {
        SV::m_page.setNextAddress(route.sub_cnt);
    } else {
        SV::m_page.setNextAddress(route.addr);
    }
    memcpy(SV::m_page.page.header.prefix, route.prefix, STORAGE_PAGE_PREFIX_SIZE);
    SV::m_page.page.header.id = route.id;
    memset(SV::m_page.page.payload, 0, STORAGE_PAGE_PAYLOAD_SIZE);
    memcpy(SV::m_page.page.payload, route.src + route.cnt, __min(STORAGE_PAGE_PAYLOAD_SIZE, route.len - route.cnt));
    SV::m_page.prepareSave();
    
    StorageStatus status = AT::driverCallback()->asyncWrite(route.addr, (uint8_t*)&SV::m_page.page, STORAGE_PAGE_SIZE);
    if (status == STORAGE_OK) {
        SV::m_timer.start();
    } else {
        SV::m_result = status;
        fsm_gc_push_event(&st_at_read_fsm, &error_e);
    } 
}

void _rewrite_save_s()
{
    if (SV::m_timer.wait()) {
        return;
    }
    fsm_gc_push_event(&st_at_rewrite_fsm, &error_e);
}

void _rewrite_setup_a()
{
    SV::route_t& route = SV::m_queue.back();
    route.cnt += __min(STORAGE_PAGE_PAYLOAD_SIZE, route.len - route.cnt);
    if (SM::isMacroblockAddress(route.addr)) {
        fsm_gc_push_event(&st_at_rewrite_fsm, &end_e);
        return;
    }
    SV::m_header.setPageStatus(Header::PAGE_OK);
    uint32_t addr1 = __rm_mod(route.addr, SM::getMacroblocksSize());
    uint32_t addr2 = __rm_mod(route.sub_cnt, SM::getMacroblocksSize());
    if (addr1 != addr2) {
        fsm_gc_push_event(&st_at_rewrite_fsm, &header_e);
        return;
    }
    if (route.cnt < route.len) {
        fsm_gc_push_event(&st_at_rewrite_fsm, &next_e);
        return;
    }
    fsm_gc_push_event(&st_at_rewrite_fsm, &header_e);
}

void _rewrite_setup_s() {}

void _rewrite_hd_save_a()
{
    SV::route_t& route = SV::m_queue.back();
    SV::route_t header{
        SV::ST_HEADER,
        route.addr,
        STORAGE_PAGE_SIZE,
        0,
        {},
        0,
        nullptr,
        (uint8_t*)&SV::m_header.page,
        (StorageFindMode)0,
		0
    };
    SV::route(header);
    SV::m_timer.start();
}

void _rewrite_hd_save_s()
{
    if (SV::m_timer.wait()) {
        return;
    }
    fsm_gc_push_event(&st_at_rewrite_fsm, &error_e);
}

void _rewrite_back_a()
{
    SV::route_t& route = SV::m_queue.back();
    SV::route_t _delete{
        SV::ST_DELETE,
        0,
        0,
        0,
        {},
        route.id,
        nullptr,
        nullptr,
        (StorageFindMode)0,
		0
    };
    memcpy(_delete.prefix, route.prefix, sizeof(route.prefix));
    SV::route(_delete);
    SV::m_timer.start();
}

void _rewrite_back_s()
{
    if (SV::m_timer.wait()) {
        return;
    }
    fsm_gc_push_event(&st_at_rewrite_fsm, &done_e);
}

void _rewrite_success_a()
{
    SV::m_result = STORAGE_OK;
    fsm_gc_clear(&st_at_rewrite_fsm);
    fsm_gc_push_event(&st_at_fsm, &done_e);
}

void _rewrite_error_a()
{
    if (SV::m_result == STORAGE_OK) {
        SV::m_result = STORAGE_ERROR;
    }
    fsm_gc_clear(&st_at_rewrite_fsm);
    fsm_gc_push_event(&st_at_fsm, &done_e);
}


FSM_GC_CREATE_ACTION(delete_find_a,     _delete_find_a)
FSM_GC_CREATE_ACTION(delete_found_a,    _delete_found_a)
FSM_GC_CREATE_ACTION(delete_header_a,   _delete_header_a)
FSM_GC_CREATE_ACTION(delete_hd_next_a,  _delete_hd_next_a)
FSM_GC_CREATE_ACTION(delete_save_a,     _delete_save_a)
FSM_GC_CREATE_ACTION(delete_rd_erase_a, _delete_rd_erase_a)
FSM_GC_CREATE_ACTION(delete_rd_next_a,  _delete_rd_next_a)
FSM_GC_CREATE_ACTION(delete_sv_erase_a, _delete_sv_erase_a)
FSM_GC_CREATE_ACTION(delete_success_a,  _delete_success_a)
FSM_GC_CREATE_ACTION(delate_error_a,    _delete_error_a)

FSM_GC_CREATE_STATE(delete_init_s,     _delete_init_s)
FSM_GC_CREATE_STATE(delete_find_s,     _delete_find_s)
FSM_GC_CREATE_STATE(delete_header_s,   _delete_header_s)
FSM_GC_CREATE_STATE(delete_save_s,     _delete_save_s)
FSM_GC_CREATE_STATE(delete_rd_erase_s, _delete_rd_erase_s)
FSM_GC_CREATE_STATE(delete_sv_erase_s, _delete_sv_erase_s)

FSM_GC_CREATE_TABLE(
    st_at_delete_fsm_table,
    {&delete_init_s,     &success_e, &delete_find_s,     &delete_find_a},
    {&delete_init_s,     &find_e,    &delete_header_s,   &delete_header_a},

    {&delete_find_s,     &done_e,    &delete_header_s,   &delete_found_a},
    {&delete_find_s,     &error_e,   &delete_sv_erase_s, &delete_sv_erase_a},

    {&delete_header_s,   &success_e, &delete_init_s,     &delete_success_a},
    {&delete_header_s,   &done_e,    &delete_save_s,     &delete_save_a},
    {&delete_header_s,   &error_e,   &delete_rd_erase_s, &delete_rd_erase_a},

    {&delete_save_s,     &done_e,    &delete_header_s,   &delete_hd_next_a},
    {&delete_save_s,     &error_e,   &delete_rd_erase_s, &delete_rd_erase_a},

    {&delete_rd_erase_s, &success_e, &delete_init_s,     &delete_success_a},
    {&delete_rd_erase_s, &next_e,    &delete_sv_erase_s, &delete_sv_erase_a},
    {&delete_rd_erase_s, &done_e,    &delete_rd_erase_s, &delete_rd_next_a},
    {&delete_rd_erase_s, &error_e,   &delete_rd_erase_s, &delete_rd_erase_a},

    {&delete_sv_erase_s, &done_e,    &delete_header_s,   &delete_hd_next_a},
    {&delete_sv_erase_s, &error_e,   &delete_init_s,     &delate_error_a},
)

void _delete_a() {}

void _delete_s()
{
    static bool initialized = false;
    if (!initialized) {
        fsm_gc_init(&st_at_delete_fsm, st_at_delete_fsm_table, __arr_len(st_at_delete_fsm_table));
        initialized = true;
    }
    fsm_gc_process(&st_at_delete_fsm);
}

void _delete_init_s()
{
    SV::route_t& route = SV::m_queue.back();
    route.cnt = 0;
    route.sub_cnt = 0;
    if (route.prefix[0] == 0) {
        fsm_gc_push_event(&st_at_header_fsm, &success_e);
    } else {
        route.addr = 0;
        fsm_gc_push_event(&st_at_header_fsm, &find_e);
    }
}

void _delete_find_a()
{
    SV::route_t& route = SV::m_queue.back();
    SV::route_t read{
        SV::ST_READ,
        route.addr,
        STORAGE_PAGE_SIZE,
        0,
        {},
        0,
        (uint8_t*)&SV::m_page.page,
        nullptr,
        (StorageFindMode)0,
		0
    };
    SV::route(read);
    SV::m_timer.start();
}

void _delete_find_s()
{
    if (SV::m_timer.wait()) {
        return;
    }
    fsm_gc_push_event(&st_at_delete_fsm, &error_e);
}

void _delete_found_a()
{
    SV::route_t& route = SV::m_queue.back();
    memcpy(route.prefix, SV::m_header.page.header.prefix, STORAGE_PAGE_PREFIX_SIZE);
    route.id = SV::m_header.page.header.id;
    _delete_header_a();
}

void _delete_header_a()
{
    SV::m_timer.start();
    SV::route_t& route = SV::m_queue.back();
    SV::route_t header{
        SV::ST_HEADER,
        route.addr,
        STORAGE_PAGE_SIZE,
        0,
        {},
        0,
        nullptr,
        (uint8_t*)&SV::m_header.page,
        (StorageFindMode)0,
		0
    };
    SV::route(header);
}

void _delete_hd_next_a()
{
    SV::m_timer.start();
    SV::route_t& route = SV::m_queue.back();
    route.addr += SM::PAGES_COUNT * STORAGE_PAGE_SIZE;
    if (route.addr >= SM::getMacroblocksCount() * SM::PAGES_COUNT * STORAGE_PAGE_SIZE) {
        fsm_gc_push_event(&st_at_delete_fsm, &success_e);
        return;
    }
    _delete_header_a();
}

void _delete_header_s()
{
    if (SV::m_timer.wait()) {
        return;
    }
    fsm_gc_push_event(&st_at_delete_fsm, &error_e);
}

void _delete_save_a()
{
    SV::route_t& route = SV::m_queue.back();
    for (unsigned i = 0; i < Header::PAGES_COUNT; i++) {
	    Header::MetaUnit* metaUnitPtr = SV::m_header.data->metaUnits;
        if (!memcmp((*metaUnitPtr).prefix, route.prefix, STORAGE_PAGE_PREFIX_SIZE) &&
            (*metaUnitPtr).id == route.id
        ) {
	        SV::m_header.setPageStatus(i, Header::PAGE_EMPTY);
	        (*metaUnitPtr).id = 0;
        }
    }
    SV::route_t save{
        SV::ST_REWRITE,
        SV::m_header.getAddress(),
        STORAGE_PAGE_SIZE,
        0,
        {},
        0,
        (uint8_t*)&SV::m_header.page,
        nullptr,
        (StorageFindMode)0,
		0
    };
    SV::route(save);
    SV::m_timer.start();
}

void _delete_save_s()
{
    if (SV::m_timer.wait()) {
        return;
    }
    fsm_gc_push_event(&st_at_delete_fsm, &error_e);
}

void _delete_rd_erase_a()
{
    SV::route_t& route = SV::m_queue.back();
    route.cnt = __rm_mod(route.addr, SM::PAGES_COUNT * STORAGE_PAGE_SIZE) + SM::RESERVED_PAGES_COUNT * STORAGE_PAGE_SIZE;
    SV::route_t read{
        SV::ST_READ,
        route.cnt,
        STORAGE_PAGE_SIZE,
        0,
        {},
        0,
		(uint8_t*)&SV::m_page.page,
        nullptr,
        (StorageFindMode)0,
		0
    };
    SV::route(read);
}

void _delete_rd_next_a()
{
    SV::route_t& route = SV::m_queue.back();
    route.cnt += STORAGE_PAGE_SIZE;
    if (!memcmp(SV::m_page.page.header.prefix, route.prefix, STORAGE_PAGE_PREFIX_SIZE) &&
        SV::m_page.page.header.id == route.id
    ) {
        SV::m_addrs[route.sub_cnt++] = route.cnt;
    }
    if (SM::isMacroblockAddress(route.cnt)) {
        fsm_gc_push_event(&st_at_delete_fsm, &next_e);
        return;
    }
    SV::route_t read{
        SV::ST_READ,
        route.cnt,
        STORAGE_PAGE_SIZE,
        0,
        {},
        0,
		(uint8_t*)&SV::m_page.page,
        nullptr,
        (StorageFindMode)0,
		0
    };
    SV::route(read);
}

void _delete_rd_erase_s()
{
    if (SV::m_timer.wait()) {
        return;
    }
    fsm_gc_push_event(&st_at_delete_fsm, &error_e);
}

void _delete_sv_erase_a()
{
    SV::route_t& route = SV::m_queue.back();
    StorageStatus status = AT::driverCallback()->asyncErase(SV::m_addrs, route.sub_cnt);
    if (status == STORAGE_OK) {
        SV::m_timer.start();
    } else {
        SV::m_result = status;
        fsm_gc_push_event(&st_at_read_fsm, &error_e);
    } 
}

void _delete_sv_erase_s()
{
    if (SV::m_timer.wait()) {
        return;
    }
    fsm_gc_push_event(&st_at_delete_fsm, &error_e);
}

void _delete_success_a()
{
    SV::m_result = STORAGE_OK;
    fsm_gc_clear(&st_at_delete_fsm);
    fsm_gc_push_event(&st_at_fsm, &done_e);
}

void _delete_error_a()
{
    if (SV::m_result == STORAGE_OK) {
        SV::m_result = STORAGE_ERROR;
    }
    fsm_gc_clear(&st_at_delete_fsm);
    fsm_gc_push_event(&st_at_fsm, &done_e);
}


FSM_GC_CREATE_ACTION(header_read_a,    _header_read_a)
FSM_GC_CREATE_ACTION(header_check_a,   _header_check_a)
FSM_GC_CREATE_ACTION(header_next_a,    _header_next_a)
FSM_GC_CREATE_ACTION(header_save_a,    _header_save_a)
FSM_GC_CREATE_ACTION(header_create_a,  _header_create_a)
FSM_GC_CREATE_ACTION(header_cr_next_a, _header_cr_next_a)
FSM_GC_CREATE_ACTION(header_sv_next_a, _header_sv_next_a)
FSM_GC_CREATE_ACTION(header_success_a, _header_success_a)
FSM_GC_CREATE_ACTION(header_error_a,   _header_error_a)

FSM_GC_CREATE_STATE(header_init_s,     _header_init_s)
FSM_GC_CREATE_STATE(header_read_s,     _header_read_s)
FSM_GC_CREATE_STATE(header_create_s,   _header_create_s)
FSM_GC_CREATE_STATE(header_save_s,     _header_save_s)

FSM_GC_CREATE_TABLE(
    st_at_header_fsm_table,
    {&header_init_s,   &success_e, &header_read_s,   &header_read_a},
    {&header_init_s,   &write_e,   &header_save_s,   &header_save_a},

    {&header_read_s,   &success_e, &header_init_s,   &header_success_a},
    {&header_read_s,   &done_e,    &header_read_s,   &header_check_a},
    {&header_read_s,   &end_e,     &header_create_s, &header_create_a},
    {&header_read_s,   &error_e,   &header_read_s,   &header_next_a},

    {&header_create_s, &success_e, &header_save_s,   &header_save_a},
    {&header_create_s, &done_e,    &header_create_s, &header_cr_next_a},
    {&header_create_s, &error_e,   &header_create_s, &header_cr_next_a},

    {&header_save_s,   &success_e, &header_init_s,   &header_success_a},
    {&header_save_s,   &error_e,   &header_save_s,   &header_sv_next_a},
    {&header_save_s,   &end_e,     &header_init_s,   &header_error_a},
)


void _header_a() {}

void _header_s()
{
    static bool initialized = false;
    if (!initialized) {
        fsm_gc_init(&st_at_header_fsm, st_at_header_fsm_table, __arr_len(st_at_header_fsm_table));
        initialized = true;
    }
    fsm_gc_process(&st_at_header_fsm);
}

void _header_init_s()
{
    SV::route_t& route = SV::m_queue.back();
    route.cnt = 0;
    route.addr = SM::getMacroblockAddress(SM::getMacroblockIndex(route.addr));
    if (route.src) {
        fsm_gc_push_event(&st_at_header_fsm, &write_e);
    } else {
        fsm_gc_push_event(&st_at_header_fsm, &success_e);
    }
}

static void _header_read(const uint32_t address, uint8_t* dst)
{
    if (address + STORAGE_PAGE_SIZE > AT::getStorageSize()) {
        SV::m_result = STORAGE_OOM;
        fsm_gc_push_event(&st_at_header_fsm, &error_e);
    } else {
        StorageStatus status = AT::driverCallback()->asyncRead(address, dst, sizeof(SV::m_page.page));
        if (status != STORAGE_OK) {
            SV::m_result = status;
            fsm_gc_push_event(&st_at_header_fsm, &error_e);
        } 
    }
    SV::m_timer.start();
}

void _header_read_a()
{
    SV::route_t& route = SV::m_queue.back();
    if (!SM::isMacroblockAddress(route.addr)) {
        fsm_gc_push_event(&st_at_header_fsm, &end_e);
        return;
    } 
    _header_read(route.addr, (uint8_t*)&SV::m_header.page);
}

void _header_next_a()
{
    SV::route_t& route = SV::m_queue.back();
    route.cnt++;
    if (route.cnt >= SM::RESERVED_PAGES_COUNT) {
        fsm_gc_push_event(&st_at_header_fsm, &timeout_e);
        return;
    }
    if (!SM::isMacroblockAddress(route.addr)) {
        fsm_gc_push_event(&st_at_header_fsm, &end_e);
        return;
    } 
    _header_read(route.addr + route.cnt * STORAGE_PAGE_SIZE, (uint8_t*)&SV::m_header.page);
}

void _header_check_a()
{
    if (!SV::m_header.validate()) {
        SV::m_result = STORAGE_NOTVALID_ERROR;
        fsm_gc_push_event(&st_at_header_fsm, &error_e);
        return;
    }
    fsm_gc_push_event(&st_at_header_fsm, &success_e);
}

void _header_read_s()
{
    if (SV::m_timer.wait()) {
        return;
    }
    fsm_gc_push_event(&st_at_header_fsm, &done_e);
}

void _header_create_a()
{
    for (unsigned i = 0; i < Header::STATUSES_COUNT; i++) {
        SV::m_header.setPageStatus(i, Header::PAGE_EMPTY);
    }
    SV::route_t& route = SV::m_queue.back();
    if (SM::isMacroblockAddress(route.addr)) {
        fsm_gc_push_event(&st_at_header_fsm, &error_e);
        return;
    } 
    _header_read(
        SM::getPageAddressByIndex(SM::getPageIndexByAddress(route.addr), 0), 
        (uint8_t*)&SV::m_page.page
    );
}

void _header_cr_next_a()
{
    SV::route_t& route = SV::m_queue.back();
    route.cnt++;
    if (route.cnt >= Header::PAGES_COUNT) {
        fsm_gc_push_event(&st_at_header_fsm, &success_e);
        return;
    }
    if (SM::isMacroblockAddress(route.addr)) {
        fsm_gc_push_event(&st_at_header_fsm, &error_e);
        return;
    } 
    _header_read(
        SM::getPageAddressByIndex(SM::getPageIndexByAddress(route.addr), route.cnt), 
        (uint8_t*)&SV::m_page.page
    );
}

void _header_create_s()
{
    if (SV::m_timer.wait()) {
        return;
    }
    fsm_gc_push_event(&st_at_header_fsm, &done_e);
}

static void _header_save(const uint32_t address)
{
    SV::route_t save{
        SV::ST_REWRITE,
        address,
        STORAGE_PAGE_SIZE,
        0,
        {},
        0,
        nullptr,
        (uint8_t*)&SV::m_header.page,
        (StorageFindMode)0,
		0
    };
    SV::route(save);
    SV::m_timer.start();
}

void _header_save_a()
{
    SV::route_t& route = SV::m_queue.back();
    route.cnt = 0;
    _header_save(route.addr);
}

void _header_sv_next_a()
{
    SV::route_t& route = SV::m_queue.back();
    route.cnt++;
    if (route.cnt >= SM::RESERVED_PAGES_COUNT) {
        fsm_gc_push_event(&st_at_header_fsm, &error_e);
        return;
    }
    _header_save(route.addr + route.cnt * STORAGE_PAGE_SIZE);
}

void _header_save_s()
{
    if (SV::m_timer.wait()) {
        return;
    }
    fsm_gc_push_event(&st_at_header_fsm, &timeout_e);
}

void _header_success_a()
{
    SV::m_result = STORAGE_OK;
    fsm_gc_clear(&st_at_header_fsm);
    fsm_gc_push_event(&st_at_fsm, &done_e);
}

void _header_error_a()
{
    if (SV::m_result == STORAGE_OK) {
        SV::m_result = STORAGE_ERROR;
    }
    fsm_gc_clear(&st_at_header_fsm);
    fsm_gc_push_event(&st_at_fsm, &done_e);
}

