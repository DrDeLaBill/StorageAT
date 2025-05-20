/* Copyright © 2025 Georgy E. All rights reserved. */

#include "StorageService.hpp"

#include "StorageAT.h"
#include "StorageSearch.h"

#include "gutils.h"
#include "fsm_gc.h"


#define STORAGE_DELAY_MS (500)


using namespace prvt_st_at;
using AT = StorageAT;
using SV = StorageService;
using SM = StorageMacroblock;


utl::GStack<16, SV::route_t> SV::m_queue;
uint32_t                     SV::m_addrs[Header::PAGES_COUNT] = {};
Page                         SV::m_page(0);
Header                       SV::m_header(0);
StorageStatus                SV::m_result = STORAGE_OK;
AT::callback_t               SV::m_callback = nullptr;

static const char TAG[] = "STSV";


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
FSM_GC_CREATE_EVENT(hd_err_e,  0)
FSM_GC_CREATE_EVENT(success_e, 1)
FSM_GC_CREATE_EVENT(timeout_e, 2)
FSM_GC_CREATE_EVENT(error_e,   3)
FSM_GC_CREATE_EVENT(oom_e,     4)
FSM_GC_CREATE_EVENT(busy_e,    5)

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
FSM_GC_CREATE_STATE(header_s,    _header_s)

FSM_GC_CREATE_TABLE(
    st_at_fsm_table,
    {&init_s,    &done_e,   &router_s,   &router_a},

    {&router_s,  &find_e,    &find_s,    &find_a},
    {&router_s,  &read_e,    &read_s,    &read_a},
    {&router_s,  &write_e,   &write_s,   &write_a},
    {&router_s,  &rewrite_e, &rewrite_s, &rewrite_a},
    {&router_s,  &delete_e,  &delete_s,  &delete_a},
    {&router_s,  &header_e,  &header_s,  &header_a},
    {&router_s,  &error_e,   &router_s,  &router_a},

    {&find_s,    &done_e,    &router_s,  &callback_a},
    {&find_s,    &router_e,  &router_s,  &router_a},

    {&read_s,    &done_e,    &router_s,  &callback_a},
    {&read_s,    &router_e,  &router_s,  &router_a},

    {&write_s,   &done_e,    &router_s,  &callback_a},
    {&write_s,   &router_e,  &router_s,  &router_a},

    {&rewrite_s, &done_e,    &router_s,  &callback_a},
    {&rewrite_s, &router_e,  &router_s,  &router_a},

    {&delete_s,  &done_e,    &router_s,  &callback_a},
    {&delete_s,  &router_e,  &router_s,  &router_a},

    {&header_s,  &done_e,    &router_s,  &callback_a},
    {&header_s,  &router_e,  &router_s,  &router_a},
)


void SV::init()
{
    // fsm_gc_disable_messages(&st_at_fsm); TODO
    // fsm_gc_disable_messages(&st_at_find_fsm);
    // fsm_gc_disable_messages(&st_at_read_fsm);
    // fsm_gc_disable_messages(&st_at_write_fsm);
    // fsm_gc_disable_messages(&st_at_rewrite_fsm);
    // fsm_gc_disable_messages(&st_at_delete_fsm);
    // fsm_gc_disable_messages(&st_at_header_fsm);
    fsm_gc_init(&st_at_fsm, st_at_fsm_table, __arr_len(st_at_fsm_table));
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
    switch (m_queue.peek().status)
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
    if (st_at_fsm._initialized) {
        fsm_gc_reset(&st_at_fsm);
    }
    if (st_at_find_fsm._initialized) {
        fsm_gc_reset(&st_at_find_fsm);
    }
    if (st_at_read_fsm._initialized) {
        fsm_gc_reset(&st_at_read_fsm);
    }
    if (st_at_write_fsm._initialized) {
        fsm_gc_reset(&st_at_write_fsm);
    }
    if (st_at_rewrite_fsm._initialized) {
        fsm_gc_reset(&st_at_rewrite_fsm);
    }
    if (st_at_delete_fsm._initialized) {
        fsm_gc_reset(&st_at_delete_fsm);
    }
    if (st_at_header_fsm._initialized) {
        fsm_gc_reset(&st_at_header_fsm);
    }
    m_queue.clear();
    tick();
}

void SV::route(route_t& route)
{
    if (m_queue.full()) {
        m_result = STORAGE_ERROR;
        fsm_gc_push_event(&st_at_fsm, &done_e);
    } else {

        switch (route.status) {
        case SV::ST_FIND:
            printTagLog(TAG, "%02lu r: find    - %lu", SV::m_queue.count(), route.addr);
            break;
        case SV::ST_READ:
            printTagLog(TAG, "%02lu r: read    - %lu", SV::m_queue.count(), route.addr);
            break;
        case SV::ST_WRITE:
            printTagLog(TAG, "%02lu r: write   - %lu", SV::m_queue.count(), route.addr);
            break;
        case SV::ST_REWRITE:
            printTagLog(TAG, "%02lu r: rewrite - %lu", SV::m_queue.count(), route.addr);
            break;
        case SV::ST_DELETE:
            printTagLog(TAG, "%02lu r: delete  - %lu", SV::m_queue.count(), route.addr);
            break;
        case SV::ST_HEADER:
            printTagLog(TAG, "%02lu r: header  - %lu", SV::m_queue.count(), route.addr);
            break;
        case SV::ST_READY:
        default:
            printTagLog(TAG, "%02lu r: defalt  - %lu", SV::m_queue.count(), route.addr);
            break;
        }

        m_queue.push(route);
        if (!fsm_gc_is_state(&st_at_fsm, &router_s)) {
            fsm_gc_push_event(&st_at_fsm, &router_e);
        }
    }
}

void SV::routeRes(fsm_gc_t* fsm)
{
    if (m_result == STORAGE_OK) {
        fsm_gc_push_event(fsm, &done_e);
    } else {
        fsm_gc_push_event(fsm, &error_e);
    }
}

StorageStatus SV::asyncFind(
    StorageFindMode mode,
    uint32_t*       address,
    AT::callback_t  callback,
    const char*     prefix,
    uint32_t        id
) {
    if (!ready()) {
        return STORAGE_BUSY;
    }
    if (!address || !callback) {
        return STORAGE_ERROR;
    }
    if (
        mode != FIND_MODE_EMPTY &&
        mode != FIND_MODE_NEXT &&
        (!prefix || !prefix[0])
    ) {
        return STORAGE_ERROR;
    }
    if (mode > FIND_MODE_EMPTY || mode == 0) {
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
		0,
        0,
        {}
    };
    if (prefix) {
        memcpy(route.prefix, prefix, __min(STORAGE_PAGE_PREFIX_SIZE, strlen(prefix)));
    } else {
        memset(route.prefix, 0, STORAGE_PAGE_PREFIX_SIZE);
    }
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
		0,
        0,
        {}
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
		0,
        0,
        {}
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
		0,
        0,
        {}
    };
    memcpy(route.prefix, prefix, __min(STORAGE_PAGE_PREFIX_SIZE, strlen(prefix)));
    SV::route(route);
    return STORAGE_OK;
}

void _init_s()
{
    fsm_gc_push_event(&st_at_fsm, &done_e);
}

void _router_a() 
{
    fsm_gc_clear(&st_at_fsm);
}

void _router_s()
{
    if (SV::m_queue.empty()) {
        return;
    }
    SV::route_t route = SV::m_queue.peek();
    switch (SV::m_queue.peek().status) {
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

    switch (route.status) {
    case SV::ST_FIND:
        printTagLog(TAG, "%02lu c: find    - %lu, res=%u", SV::m_queue.count(), route.sub_addr, SV::m_result);
        break;
    case SV::ST_READ:
        printTagLog(TAG, "%02lu c: read    - %lu, res=%u", SV::m_queue.count(), route.addr, SV::m_result);
        break;
    case SV::ST_WRITE:
        printTagLog(TAG, "%02lu c: write   - %lu, res=%u", SV::m_queue.count(), route.addr, SV::m_result);
        break;
    case SV::ST_REWRITE:
        printTagLog(TAG, "%02lu c: rewrite - %lu, res=%u", SV::m_queue.count(), route.addr, SV::m_result);
        break;
    case SV::ST_DELETE:
        printTagLog(TAG, "%02lu c: delete  - %lu, res=%u", SV::m_queue.count(), route.addr, SV::m_result);
        break;
    case SV::ST_HEADER:
        printTagLog(TAG, "%02lu c: header  - %lu, res=%u", SV::m_queue.count(), route.addr, SV::m_result);
        break;
    case SV::ST_READY:
    default:
        printTagLog(TAG, "%02lu c: defalt  - %lu, res=%u", SV::m_queue.count(), route.addr, SV::m_result);
        break;
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


FSM_GC_CREATE_ACTION(find_header_a,   _find_header_a)
FSM_GC_CREATE_ACTION(find_hd_next_a,  _find_hd_next_a)
FSM_GC_CREATE_ACTION(find_check_a,    _find_check_a)
FSM_GC_CREATE_ACTION(find_rd_check_a, _find_rd_check_a)
FSM_GC_CREATE_ACTION(find_read_a,     _find_read_a)
FSM_GC_CREATE_ACTION(find_success_a,  _find_success_a)
FSM_GC_CREATE_ACTION(find_error_a,    _find_error_a)

FSM_GC_CREATE_STATE(find_init_s,      _find_init_s)
FSM_GC_CREATE_STATE(find_header_s,    _find_header_s)
FSM_GC_CREATE_STATE(find_check_s,     _find_check_s)
FSM_GC_CREATE_STATE(find_read_s,      _find_read_s)
FSM_GC_CREATE_STATE(find_rd_check_s,  _find_rd_check_s)

FSM_GC_CREATE_TABLE(
    st_at_find_fsm_table,
    {&find_init_s,     &success_e, &find_header_s,   &find_header_a},

    {&find_header_s,   &end_e,     &find_init_s,     &find_error_a},
    {&find_header_s,   &done_e,    &find_check_s,    &find_check_a},
    {&find_header_s,   &error_e,   &find_header_s,   &find_hd_next_a},

    {&find_check_s,    &end_e,     &find_init_s,     &find_success_a},
    {&find_check_s,    &done_e,    &find_read_s,     &find_read_a},
    {&find_check_s,    &next_e,    &find_header_s,   &find_hd_next_a},
    {&find_check_s,    &error_e,   &find_init_s,     &find_error_a},

    {&find_read_s,     &done_e,    &find_rd_check_s, &find_rd_check_a},
    {&find_read_s,     &error_e,   &find_header_s,   &find_hd_next_a},

    {&find_rd_check_s, &success_e, &find_init_s,     &find_success_a},
    {&find_rd_check_s, &error_e,   &find_header_s,   &find_hd_next_a},
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
    SV::route_t& route = SV::m_queue.peek();
    if (route.mode != FIND_MODE_EMPTY) {
        route.addr = 0;
    }
    SV::m_header.setAddress(route.addr);
    route.sub_cnt  = 0;
    route.sub_addr = route.addr;
    route.cnt      = SM::getPageIndexByAddress(route.addr);
    route.addr     = __rm_mod(route.addr, SM::getMacroblockSize());
    StorageSearchBase::reset();
    fsm_gc_push_event(&st_at_find_fsm, &success_e);
}

void _find_header_a()
{
    SV::route_t& route = SV::m_queue.peek();
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
		0,
        0,
        {}
    };
    SV::route(header);
}

void _find_hd_next_a()
{
    SV::route_t& route = SV::m_queue.peek();
    route.timer.reset();
    route.addr += SM::getMacroblockSize();
    if (route.addr >= AT::getStorageSize()) {
        if (SV::m_result == STORAGE_OK) {
            SV::m_result = STORAGE_NOT_FOUND;
        }
        fsm_gc_push_event(&st_at_find_fsm, &end_e);
        route.timer.start(STORAGE_DELAY_MS);
        return;
    }
    route.cnt = 0;
    _find_header_a();
}

void _find_header_s()
{
    SV::route_t& route = SV::m_queue.peek();
    if (route.timer.wait()) {
        return;
    }
    SV::routeRes(&st_at_find_fsm);
}

void _find_check_a()
{
    SV::route_t& route = SV::m_queue.peek();
    StorageSearchBase* search;    
    switch (route.mode) {
    case FIND_MODE_EQUAL:
        search = new StorageSearchEqual(0);
        break;
    case FIND_MODE_NEXT:
        search = new StorageSearchNext(0);
        break;
    case FIND_MODE_MIN:
        search = new StorageSearchMin(0);
        break;
    case FIND_MODE_MAX:
        search = new StorageSearchMax(0);
        break;
    case FIND_MODE_EMPTY:
        search = new StorageSearchEmpty(route.sub_addr);
        break;
    default:
        fsm_gc_push_event(&st_at_find_fsm, &error_e);
        return;
    }
    if (route.addr <= SM::RESERVED_PAGES_COUNT * STORAGE_PAGE_SIZE) {
        search->setStartId(search->getStartCmpId());
    }
    StorageStatus status = search->searchPageAddressInMacroblock(
        &SV::m_header, 
        (uint8_t*)route.prefix, 
        route.id, 
        false
    );
    if (search->found()) {
        route.sub_cnt  = 1;
        route.sub_addr = search->getAddress();
        if (route.mode == FIND_MODE_EMPTY) {
            memcpy(route.dst, (uint8_t*)&route.sub_addr, sizeof(route.sub_addr));
            fsm_gc_push_event(&st_at_find_fsm, &end_e);
            return;
        }
    }
    if (route.addr + SM::getMacroblockSize() < AT::getStorageSize()) {
        if (!search->isNeededFirstResult((uint8_t*)route.prefix) || status != STORAGE_OK) {
            fsm_gc_push_event(&st_at_find_fsm, &next_e);
            delete search;
            return;
        }
    }
    if (route.sub_cnt) {
        fsm_gc_push_event(&st_at_find_fsm, &done_e);
    } else {
        SV::m_result = STORAGE_NOT_FOUND;
        fsm_gc_push_event(&st_at_find_fsm, &error_e);
    }
    delete search;
}

void _find_check_s() {}

void _find_read_a()
{
    SV::route_t& route = SV::m_queue.peek();
    StorageStatus status = AT::driverCallback()->asyncRead(route.sub_addr, (uint8_t*)&SV::m_page.page, sizeof(SV::m_page.page));
    if (status == STORAGE_OK) {
        route.timer.start(STORAGE_DELAY_MS);
        SV::m_page.setAddress(route.sub_addr);
    } else {
        SV::m_result = status;
        fsm_gc_push_event(&st_at_find_fsm, &error_e);
    } 
}

void _find_read_s()
{

    SV::route_t& route = SV::m_queue.peek();
    if (route.timer.wait()) {
        return;
    }
    fsm_gc_push_event(&st_at_find_fsm, &error_e);
}

void _find_rd_check_a()
{
    SV::route_t& route = SV::m_queue.peek();
    if (!SV::m_page.validate()) {
        // TODO: SV::m_page.repair();
    }
    if (SV::m_page.validate() && SV::m_page.isStart()) {
        memcpy(route.dst, (uint8_t*)&route.sub_addr, sizeof(route.sub_addr));
        fsm_gc_push_event(&st_at_find_fsm, &success_e);
        return;
    }
    fsm_gc_push_event(&st_at_find_fsm, &error_e);
}

void _find_rd_check_s() {}

void _find_success_a()
{
    SV::m_result = STORAGE_OK;
    fsm_gc_clear(&st_at_find_fsm);
    fsm_gc_push_event(&st_at_fsm, &done_e);
}

void _find_error_a()
{
    if (SV::m_result == STORAGE_OK) {
        SV::m_result = STORAGE_ERROR;
    }
    fsm_gc_clear(&st_at_find_fsm);
    fsm_gc_push_event(&st_at_fsm, &done_e);
}


FSM_GC_CREATE_ACTION(read_read_a,    _read_read_a)
FSM_GC_CREATE_ACTION(read_check_a,   _read_check_a)
FSM_GC_CREATE_ACTION(read_success_a, _read_success_a)
FSM_GC_CREATE_ACTION(read_error_a,   _read_error_a)
 
FSM_GC_CREATE_STATE(read_init_s,     _read_init_s)
FSM_GC_CREATE_STATE(read_read_s,     _read_read_s)

FSM_GC_CREATE_TABLE(
    st_at_read_fsm_table,
    {&read_init_s,   &success_e, &read_read_s,   &read_read_a},
 
    {&read_read_s,   &success_e, &read_init_s,   &read_success_a},
    {&read_read_s,   &done_e,    &read_read_s,   &read_check_a},
    {&read_read_s,   &next_e,    &read_read_s,   &read_read_a},
    {&read_read_s,   &error_e,   &read_init_s,   &read_error_a},
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
    SV::route_t& route = SV::m_queue.peek();
    route.cnt = 0;
    fsm_gc_push_event(&st_at_read_fsm, &success_e);
}    

void _read_read_a() 
{
    SV::route_t& route = SV::m_queue.peek();
    if (!route.sub_addr && SM::isMacroblockAddress(route.addr)) {
        fsm_gc_push_event(&st_at_read_fsm, &error_e);
    } else if (route.addr + sizeof(SV::m_page.page) > AT::getStorageSize()) {
        SV::m_result = STORAGE_OOM;
        fsm_gc_push_event(&st_at_read_fsm, &error_e);
    } else {
        StorageStatus status = AT::driverCallback()->asyncRead(route.addr, (uint8_t*)&SV::m_page.page, sizeof(SV::m_page.page));
        if (status == STORAGE_OK) {
            route.timer.start(STORAGE_DELAY_MS);
            SV::m_page.setAddress(route.addr);
        } else {
            SV::m_result = status;
            fsm_gc_push_event(&st_at_read_fsm, &error_e);
        } 
    }
    route.sub_cnt = route.addr;
}

void _read_check_a()
{
    SV::route_t& route = SV::m_queue.peek();
    if (!SV::m_page.validate()) {
        // TODO: SV::m_page.repair();
    }
    if (!SV::m_page.validate()) {
        fsm_gc_push_event(&st_at_read_fsm, &error_e);
        return;
    }
    if (!route.sub_addr && !route.cnt && !SV::m_page.isStart()) {
        fsm_gc_push_event(&st_at_read_fsm, &error_e);
        return;
    }
    uint32_t cnt = STORAGE_PAGE_PAYLOAD_SIZE;
    uint8_t* ptr = SV::m_page.page.payload;
    if (SM::isMacroblockAddress(route.addr)) {
        cnt = STORAGE_HEADER_PAYLOAD_SIZE;
        ptr = ((HeaderStruct*)&SV::m_page.page)->payload;
    }
    if (!SM::isMacroblockAddress(route.addr)) {
        if (!route.cnt) {
            memcpy(route.prefix, SV::m_page.page.header.prefix, sizeof(route.prefix));
            route.id = SV::m_page.page.header.id;
        }
        if (memcmp(route.prefix, SV::m_page.page.header.prefix, sizeof(route.prefix))) {
            fsm_gc_push_event(&st_at_read_fsm, &error_e);
            return;
        }
        if (route.id != SV::m_page.page.header.id) {
            fsm_gc_push_event(&st_at_read_fsm, &error_e);
            return;
        }
    }
    cnt = __min(cnt, route.len - route.cnt);
    memcpy(route.dst + route.cnt, ptr, cnt);
    route.cnt += cnt;
    if (route.sub_addr || route.cnt == route.len && SV::m_page.isEnd()) {
        fsm_gc_push_event(&st_at_read_fsm, &success_e);
        return;
    } else if (route.cnt >= route.len) {
        fsm_gc_push_event(&st_at_read_fsm, &error_e);
        return;
    }
    route.addr = SV::m_page.page.header.next_addr;
    fsm_gc_push_event(&st_at_read_fsm, &next_e);
}

void _read_read_s()
{
    SV::route_t& route = SV::m_queue.peek();
    if (route.timer.wait()) {
        return;
    }
    fsm_gc_push_event(&st_at_read_fsm, &error_e);
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
    SV::route_t& route = SV::m_queue.peek();
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
		0,
        0,
        {}
    };
    SV::route(header);
}

void _write_read_s()
{
    SV::routeRes(&st_at_write_fsm);
}

void _write_setup_a()
{
    SV::route_t& route = SV::m_queue.peek();
    if (SV::m_header.isAddressEmpty(route.addr)) {
        fsm_gc_push_event(&st_at_write_fsm, &success_e);
    } else {
        SV::m_result = STORAGE_DATA_EXISTS;
        fsm_gc_push_event(&st_at_write_fsm, &error_e);
    }
}

void _write_setup_s() {}

void _write_rewrite_a()
{
    SV::route_t& route = SV::m_queue.peek();
    SV::route_t rewrite{
        SV::ST_REWRITE,
        route.addr,
        route.len,
        0,
        {},
        route.id,
        nullptr,
        route.src,
        (StorageFindMode)0,
		0,
        0,
        {}
    };
    memcpy(rewrite.prefix, route.prefix, STORAGE_PAGE_PREFIX_SIZE);
    SV::route(rewrite);
}

void _write_rewrite_s()
{
    SV::routeRes(&st_at_write_fsm);
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


FSM_GC_CREATE_ACTION(rewrite_delete_a,      _rewrite_delete_a)
FSM_GC_CREATE_ACTION(rewrite_header_a,      _rewrite_header_a)
FSM_GC_CREATE_ACTION(rewrite_find_a,        _rewrite_find_a)
FSM_GC_CREATE_ACTION(rewrite_find_next_a,   _rewrite_find_next_a)
FSM_GC_CREATE_ACTION(rewrite_find_prev_a,   _rewrite_find_prev_a)
FSM_GC_CREATE_ACTION(rewrite_save_a,        _rewrite_save_a)
FSM_GC_CREATE_ACTION(rewrite_read_a,        _rewrite_read_a)
FSM_GC_CREATE_ACTION(rewrite_setup_a,       _rewrite_setup_a)
FSM_GC_CREATE_ACTION(rewrite_setup_block_a, _rewrite_setup_block_a)
FSM_GC_CREATE_ACTION(rewrite_fd_save_a,     _rewrite_fd_save_a)
FSM_GC_CREATE_ACTION(rewrite_fd_page_a,     _rewrite_fd_page_a)
FSM_GC_CREATE_ACTION(rewrite_fd_read_a,     _rewrite_fd_read_a)
FSM_GC_CREATE_ACTION(rewrite_hd_save_a,     _rewrite_hd_save_a)
FSM_GC_CREATE_ACTION(rewrite_hd_read_a,     _rewrite_hd_read_a)
FSM_GC_CREATE_ACTION(rewrite_hd_block_a,    _rewrite_hd_block_a)
FSM_GC_CREATE_ACTION(rewrite_prev_a,        _rewrite_prev_a)
FSM_GC_CREATE_ACTION(rewrite_prev_hd_a,     _rewrite_prev_hd_a)
FSM_GC_CREATE_ACTION(rewrite_back_a,        _rewrite_back_a)
FSM_GC_CREATE_ACTION(rewrite_success_a,     _rewrite_success_a)
FSM_GC_CREATE_ACTION(rewrite_error_a,       _rewrite_error_a)

FSM_GC_CREATE_STATE(rewrite_init_s,         _rewrite_init_s)
FSM_GC_CREATE_STATE(rewrite_delete_s,       _rewrite_delete_s)
FSM_GC_CREATE_STATE(rewrite_header_s,       _rewrite_header_s)
FSM_GC_CREATE_STATE(rewrite_fd_curr_s,      _rewrite_fd_curr_s)
FSM_GC_CREATE_STATE(rewrite_fd_save_s,      _rewrite_fd_save_s)
FSM_GC_CREATE_STATE(rewrite_fd_read_s,      _rewrite_fd_read_s)
FSM_GC_CREATE_STATE(rewrite_fd_page_s,      _rewrite_fd_page_s)
FSM_GC_CREATE_STATE(rewrite_save_s,         _rewrite_save_s)
FSM_GC_CREATE_STATE(rewrite_read_s,         _rewrite_read_s)
FSM_GC_CREATE_STATE(rewrite_setup_s,        _rewrite_setup_s)
FSM_GC_CREATE_STATE(rewrite_hd_save_s,      _rewrite_hd_save_s)
FSM_GC_CREATE_STATE(rewrite_hd_block_s,     _rewrite_hd_block_s)
FSM_GC_CREATE_STATE(rewrite_hd_read_s,      _rewrite_hd_read_s)
FSM_GC_CREATE_STATE(rewrite_prev_s,         _rewrite_prev_s)
FSM_GC_CREATE_STATE(rewrite_prev_hd_s,      _rewrite_prev_hd_s)
FSM_GC_CREATE_STATE(rewrite_find_prev_s,    _rewrite_find_prev_s)
FSM_GC_CREATE_STATE(rewrite_back_s,         _rewrite_back_s)

FSM_GC_CREATE_TABLE(
    st_at_rewrite_fsm_table,  
    {&rewrite_init_s,      &success_e, &rewrite_delete_s,    &rewrite_delete_a},

    {&rewrite_delete_s,    &done_e,    &rewrite_header_s,    &rewrite_header_a},
    {&rewrite_delete_s,    &error_e,   &rewrite_init_s,      &rewrite_error_a},

    {&rewrite_header_s,    &header_e,  &rewrite_save_s,      &rewrite_save_a},
    {&rewrite_header_s,    &success_e, &rewrite_fd_curr_s,   &rewrite_find_a},

    {&rewrite_fd_curr_s,   &header_e,  &rewrite_fd_save_s,   &rewrite_fd_save_a},
    {&rewrite_fd_curr_s,   &end_e,     &rewrite_init_s,      &rewrite_success_a},
    {&rewrite_fd_curr_s,   &success_e, &rewrite_save_s,      &rewrite_save_a},
    {&rewrite_fd_curr_s,   &done_e,    &rewrite_fd_page_s,   &rewrite_fd_page_a},
    {&rewrite_fd_curr_s,   &error_e,   &rewrite_back_s,      &rewrite_back_a},

    {&rewrite_fd_page_s,   &done_e,    &rewrite_fd_read_s,   &rewrite_fd_read_a},
    {&rewrite_fd_page_s,   &error_e,   &rewrite_setup_s,     &rewrite_setup_block_a},

    {&rewrite_fd_read_s,   &done_e,    &rewrite_setup_s,     &rewrite_setup_a},
    {&rewrite_fd_read_s,   &error_e,   &rewrite_setup_s,     &rewrite_setup_block_a},

    {&rewrite_fd_save_s,   &done_e,    &rewrite_fd_curr_s,   &rewrite_find_a},
    {&rewrite_fd_save_s,   &error_e,   &rewrite_fd_curr_s,   &rewrite_find_a},

    {&rewrite_save_s,      &done_e,    &rewrite_read_s,      &rewrite_read_a},
    {&rewrite_save_s,      &error_e,   &rewrite_setup_s,     &rewrite_setup_block_a},

    {&rewrite_read_s,      &done_e,    &rewrite_setup_s,     &rewrite_setup_a},
    {&rewrite_read_s,      &error_e,   &rewrite_setup_s,     &rewrite_setup_block_a},

    {&rewrite_setup_s,     &next_e,    &rewrite_fd_curr_s,   &rewrite_find_next_a},
    {&rewrite_setup_s,     &header_e,  &rewrite_hd_save_s,   &rewrite_hd_save_a},
    {&rewrite_setup_s,     &hd_err_e,  &rewrite_save_s,      &rewrite_save_a},
    {&rewrite_setup_s,     &write_e,   &rewrite_hd_block_s,  &rewrite_hd_block_a},
    {&rewrite_setup_s,     &read_e,    &rewrite_hd_read_s,   &rewrite_hd_read_a},
    {&rewrite_setup_s,     &error_e,   &rewrite_back_s,      &rewrite_back_a},

    {&rewrite_hd_save_s,   &done_e,    &rewrite_fd_curr_s,   &rewrite_find_a},
    {&rewrite_hd_save_s,   &error_e,   &rewrite_fd_curr_s,   &rewrite_find_a},

    {&rewrite_hd_read_s,   &done_e,    &rewrite_hd_block_s,  &rewrite_hd_block_a},
    {&rewrite_hd_read_s,   &error_e,   &rewrite_back_s,      &rewrite_back_a},

    {&rewrite_hd_block_s,  &done_e,    &rewrite_prev_s,      &rewrite_prev_a},
    {&rewrite_hd_block_s,  &error_e,   &rewrite_prev_s,      &rewrite_prev_a},

    {&rewrite_prev_s,      &success_e, &rewrite_fd_curr_s,   &rewrite_find_a},
    {&rewrite_prev_s,      &done_e,    &rewrite_prev_hd_s,   &rewrite_prev_hd_a},
    {&rewrite_prev_s,      &error_e,   &rewrite_back_s,      &rewrite_back_a},

    {&rewrite_prev_hd_s,   &done_e,    &rewrite_find_prev_s, &rewrite_find_prev_a},
    {&rewrite_prev_hd_s,   &error_e,   &rewrite_back_s,      &rewrite_back_a},

    {&rewrite_find_prev_s, &done_e,    &rewrite_fd_page_s,   &rewrite_fd_page_a},
    {&rewrite_find_prev_s, &error_e,   &rewrite_back_s,      &rewrite_back_a},

    {&rewrite_back_s,      &done_e,    &rewrite_init_s,      &rewrite_error_a},
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
    SV::route_t& route = SV::m_queue.peek();
    route.sub_cnt  = route.addr;
    route.sub_addr = route.addr;
    fsm_gc_push_event(&st_at_rewrite_fsm, &success_e);
}

void _rewrite_delete_a()
{
    SV::route_t& route = SV::m_queue.peek();
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
		0,
        0,
        {}
    };
    SV::route(_delete);
}

void _rewrite_delete_s()
{
    SV::routeRes(&st_at_rewrite_fsm);
}

void _rewrite_header_a()
{
    SV::route_t& route = SV::m_queue.peek();
    if (SM::isMacroblockAddress(route.addr)) {
        route.sub_addr = route.addr;
        fsm_gc_push_event(&st_at_rewrite_fsm, &header_e);
        return;
    }
    fsm_gc_push_event(&st_at_rewrite_fsm, &success_e);
}

void _rewrite_header_s() {}

void _rewrite_find_a()
{
    SV::route_t& route = SV::m_queue.peek();
    if (route.cnt && route.cnt + STORAGE_PAGE_PAYLOAD_SIZE >= route.len) {
        fsm_gc_push_event(&st_at_rewrite_fsm, &end_e);
        route.timer.start(STORAGE_DELAY_MS);
        return;
    }
    SV::m_page.setPrevAddress(route.addr);
    SV::route_t find{
        SV::ST_FIND,
        route.sub_addr,
        STORAGE_PAGE_SIZE,
        0,
        {},
        0,
        (uint8_t*)&route.sub_addr,
        nullptr,
        FIND_MODE_EMPTY,
		0,
        0,
        {}
    };
    SV::route(find);
    route.timer.reset();
}

static void _rewrite_find()
{
    SV::route_t& route = SV::m_queue.peek();
    bool found = false;
    if (route.cnt + STORAGE_PAGE_PAYLOAD_SIZE >= route.len) {
        fsm_gc_push_event(&st_at_rewrite_fsm, &success_e);
        route.timer.start(STORAGE_DELAY_MS);
        return;
    }
    for (unsigned i = 0; i < Header::PAGES_COUNT; i++) {
        uint32_t addr = SM::getPageAddressByIndex(SM::getMacroblockIndex(SV::m_header.getAddress()), i);
        if (SV::m_header.isAddressEmpty(addr)) {
            route.sub_addr = addr;
            if (route.addr == route.sub_addr && route.sub_addr == addr) {
                continue;
            }
            uint32_t idx = SM::getPageIndexByAddress(addr);
            memcpy(SV::m_header.data->metaUnits[idx].prefix, route.prefix, STORAGE_PAGE_PREFIX_SIZE);
            SV::m_header.data->metaUnits[idx].id = route.id;
            found = true;
            break;
        }
    }
    if (!found && route.cnt + STORAGE_PAGE_PAYLOAD_SIZE < route.len) {
        uint32_t idx   = SM::getMacroblockIndex(route.addr);
        route.sub_addr = SM::getMacroblockAddress(idx + 1);
        fsm_gc_push_event(&st_at_rewrite_fsm, &header_e);
        route.timer.start(STORAGE_DELAY_MS);
        return;
    }
    fsm_gc_push_event(&st_at_rewrite_fsm, &success_e);
    route.timer.start(STORAGE_DELAY_MS);
}

void _rewrite_find_next_a()
{
    SV::route_t& route = SV::m_queue.peek();
    if (route.cnt >= route.len) {
        fsm_gc_push_event(&st_at_rewrite_fsm, &end_e);
        route.timer.start(STORAGE_DELAY_MS);
        return;
    }
    route.sub_cnt  = route.addr;
    route.addr     = route.sub_addr;
    route.sub_addr = route.sub_addr + STORAGE_PAGE_SIZE;
    _rewrite_find();
}

void _rewrite_fd_curr_s()
{
    SV::route_t& route = SV::m_queue.peek();
    if (route.timer.wait()) {
        return;
    }
    if (route.len <= STORAGE_PAGE_PAYLOAD_SIZE) {
        fsm_gc_push_event(&st_at_rewrite_fsm, &done_e);
        return;
    }
    if (SV::m_result == STORAGE_OK) {
        route.sub_addr = SM::getPageAddressByIndex(SM::getMacroblockIndex(SV::m_header.getAddress()), 0);
        for (unsigned i = 0; i < Header::PAGES_COUNT; i++) {
            uint32_t addr = SM::getPageAddressByIndex(SM::getMacroblockIndex(route.sub_addr), i);
            if (SV::m_header.isAddressEmpty(addr)) {
                uint32_t idx = SM::getPageIndexByAddress(addr);
                memcpy(SV::m_header.data->metaUnits[idx].prefix, route.prefix, STORAGE_PAGE_PREFIX_SIZE);
                SV::m_header.data->metaUnits[idx].id = route.id;
                route.sub_addr = addr;
                break;
            }
        }
        fsm_gc_push_event(&st_at_rewrite_fsm, &done_e);
    } else {
        fsm_gc_push_event(&st_at_rewrite_fsm, &error_e);
    }
}

void _rewrite_fd_page_a()
{
    SV::route_t& route = SV::m_queue.peek();
    uint32_t idx1 = SM::getMacroblockIndex(route.addr);
    uint32_t idx2 = SM::getMacroblockIndex(route.sub_addr);
    if (route.len > STORAGE_PAGE_PAYLOAD_SIZE && idx1 == idx2) {
        // route.sub_addr = SM::getPageAddressByIndex(SM::getMacroblockIndex(SV::m_header.getAddress()), 0); TODO
        for (unsigned i = SM::getPageIndexByAddress(route.sub_addr); i < Header::PAGES_COUNT; i++) {
            uint32_t addr = SM::getPageAddressByIndex(SM::getMacroblockIndex(route.sub_addr), i);
            if (SV::m_header.isAddressEmpty(addr)) {
                uint32_t idx = SM::getPageIndexByAddress(addr);
                memcpy(SV::m_header.data->metaUnits[idx].prefix, route.prefix, STORAGE_PAGE_PREFIX_SIZE); // TODO: add header.setMeta(index, prefix, id)
                SV::m_header.data->metaUnits[idx].id = route.id;
                route.sub_addr = addr;
                break;
            }
        }
    }
    _rewrite_save_a();
}

void _rewrite_fd_page_s()
{
    _rewrite_save_s();
}

void _rewrite_fd_read_a()
{
    _rewrite_read_a();
}

void _rewrite_fd_read_s()
{
    _rewrite_read_s();
}

void _rewrite_fd_save_a()
{
    _rewrite_hd_save_a();
}

void _rewrite_fd_save_s()
{
    _rewrite_hd_save_s();
}

void _rewrite_fd_load_a()
{
    SV::route_t& route = SV::m_queue.peek();
    SV::route_t header{
        SV::ST_HEADER,
        route.sub_addr,
        STORAGE_PAGE_SIZE,
        0,
        {},
        0,
        nullptr,
        nullptr,
        (StorageFindMode)0,
		0,
        0,
        {}
    };
    SV::route(header);
}

void _rewrite_fd_load_s()
{
    SV::routeRes(&st_at_rewrite_fsm);
}

void _rewrite_save_a()
{
    SV::route_t& route = SV::m_queue.peek();
    uint32_t itr_len = STORAGE_PAGE_PAYLOAD_SIZE;
    uint8_t* ptr = SV::m_page.page.payload;
    if (SM::isMacroblockAddress(route.addr)) {
        itr_len = STORAGE_HEADER_PAYLOAD_SIZE;
        ptr = ((HeaderStruct*)&SV::m_page.page)->payload;
    }
    uint32_t target_addr = 0;
    if (route.cnt) {
        SV::m_page.setPrevAddress(route.sub_cnt);
    } else {
        SV::m_page.setPrevAddress(route.addr);
    }
    SV::m_page.setAddress(route.addr);
    if (route.cnt + itr_len < route.len) {
        SV::m_page.setNextAddress(route.sub_addr);
    } else {
        SV::m_page.setNextAddress(route.addr);
    }
    if (!SM::isMacroblockAddress(route.addr)) {
        memcpy(SV::m_page.page.header.prefix, route.prefix, STORAGE_PAGE_PREFIX_SIZE);
        SV::m_page.page.header.id = route.id;
    }
    memset(ptr, 0xFF, itr_len);
    memcpy(ptr, route.src + route.cnt, __min(itr_len, route.len - route.cnt));
    SV::m_page.prepareSave();

    printTagLog(TAG, "write %lu %lu %lu", route.sub_cnt, route.addr, route.sub_addr);
    StorageStatus status = AT::driverCallback()->asyncWrite(route.addr, (uint8_t*)&SV::m_page.page, STORAGE_PAGE_SIZE);
    if (status == STORAGE_OK) {
        route.timer.start(STORAGE_DELAY_MS);
    } else {
        SV::m_result = status;
        fsm_gc_push_event(&st_at_read_fsm, &error_e);
    } 
}

void _rewrite_save_s()
{
    SV::route_t& route = SV::m_queue.peek();
    if (route.timer.wait()) {
        return;
    }
    fsm_gc_push_event(&st_at_rewrite_fsm, &error_e);
}

void _rewrite_read_a()
{   
    SV::route_t& route = SV::m_queue.peek();
    uint32_t itr_len = STORAGE_PAGE_PAYLOAD_SIZE;
    uint8_t* ptr = SV::m_page.page.payload;
    if (SM::isMacroblockAddress(route.addr)) {
        itr_len = STORAGE_HEADER_PAYLOAD_SIZE;
        ptr = ((HeaderStruct*)&SV::m_page.page)->payload;
    }
    SV::route_t read{
        SV::ST_READ,
        route.addr,
        itr_len,
        0,
        {},
        0,
        ptr,
        nullptr,
        (StorageFindMode)0,
		0,
        1,
        {}
    };
    SV::route(read);
}

void _rewrite_read_s()
{
    SV::route_t& route = SV::m_queue.peek();
    uint32_t itr_len = STORAGE_PAGE_PAYLOAD_SIZE;
    uint8_t* ptr = SV::m_page.page.payload;
    if (SM::isMacroblockAddress(route.addr)) {
        itr_len = STORAGE_HEADER_PAYLOAD_SIZE;
        ptr = ((HeaderStruct*)&SV::m_page.page)->payload;
    }
    if (SV::m_result != STORAGE_OK) {
        fsm_gc_push_event(&st_at_rewrite_fsm, &error_e);
        return;
    }
    uint32_t cnt = itr_len;
    if (route.len - route.cnt < itr_len) {
        cnt = route.len - route.cnt;
    }
    if (memcmp(route.src + route.cnt, ptr, cnt)) {
        fsm_gc_push_event(&st_at_rewrite_fsm, &error_e);
        return;
    }
    route.cnt += itr_len;
    fsm_gc_push_event(&st_at_rewrite_fsm, &done_e);
}

void _rewrite_setup_a()
{
    SV::route_t& route = SV::m_queue.peek();
    if (SM::isMacroblockAddress(route.addr)) {
        route.cnt += STORAGE_HEADER_PAYLOAD_SIZE;
    }
    if (route.cnt < route.len) {
        fsm_gc_push_event(&st_at_rewrite_fsm, &next_e);
        return;
    }
    uint32_t idx = SM::getPageIndexByAddress(route.addr);
    memcpy(SV::m_header.data->metaUnits[idx].prefix, route.prefix, STORAGE_PAGE_PREFIX_SIZE);
    SV::m_header.data->metaUnits[idx].id = route.id;
    fsm_gc_push_event(&st_at_rewrite_fsm, &header_e);
}

void _rewrite_setup_block_a()
{
    SV::route_t& route = SV::m_queue.peek();
    if (!SM::isMacroblockAddress(route.addr)) {
        uint32_t idx1 = SM::getMacroblockIndex(route.addr);
        uint32_t idx2 = SM::getMacroblockIndex(route.sub_addr);
        if (idx1 == idx2) {
            SV::m_header.setAddressBlocked(route.addr);
            SV::m_header.setAddressEmpty(route.sub_addr);
            fsm_gc_push_event(&st_at_rewrite_fsm, &write_e);
        } else {
            fsm_gc_push_event(&st_at_rewrite_fsm, &read_e);
        }
        return;
    }
    route.addr += STORAGE_PAGE_SIZE;
    if ((route.addr / STORAGE_PAGE_SIZE) % SM::PAGES_COUNT < SM::RESERVED_PAGES_COUNT) {
        fsm_gc_push_event(&st_at_rewrite_fsm, &hd_err_e);
    } else {
        fsm_gc_push_event(&st_at_rewrite_fsm, &error_e);
    }
}

void _rewrite_setup_s() {}

void _rewrite_hd_save_a()
{
    SV::route_t& route = SV::m_queue.peek();
    SV::route_t header{
        SV::ST_HEADER,
        route.addr,
        STORAGE_PAGE_SIZE,
        0,
        {},
        0,
        nullptr,
        (uint8_t*)SV::m_header.header,
        (StorageFindMode)0,
		0,
        0,
        {}
    };
    SV::route(header);
}

void _rewrite_hd_save_s()
{
    SV::routeRes(&st_at_rewrite_fsm);
}

void _rewrite_hd_read_a()
{
    SV::route_t& route = SV::m_queue.peek();
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
		0,
        0,
        {}
    };
    SV::route(header);
}

void _rewrite_hd_read_s()
{
    SV::routeRes(&st_at_rewrite_fsm);
}

void _rewrite_hd_block_a()
{
    SV::route_t& route = SV::m_queue.peek();
    SV::m_header.setAddressBlocked(route.addr);
    _rewrite_hd_save_a();
}

void _rewrite_hd_block_s()
{
    _rewrite_hd_save_s();
}

void _rewrite_prev_a()
{
    SV::route_t& route = SV::m_queue.peek();
    route.timer.reset();
    SV::route_t req = {};
    if (route.cnt) {
        req = SV::route_t{
            SV::ST_READ,
            route.sub_cnt,
            STORAGE_PAGE_PAYLOAD_SIZE,
            0,
            {},
            0,
            SV::m_page.page.payload,
            nullptr,
            (StorageFindMode)0,
            0,
            1,
            {}
        };
    } else {
        route.addr     = route.addr + STORAGE_PAGE_SIZE;
        route.sub_cnt  = route.addr;
        route.sub_addr = route.addr;
        req = SV::route_t{
            SV::ST_FIND,
            route.addr,
            STORAGE_PAGE_SIZE,
            0,
            {},
            0,
            (uint8_t*)&route.addr,
            nullptr,
            FIND_MODE_EMPTY,
            0,
            0,
            {}
        };
    }
    SV::route(req);
}

void _rewrite_prev_s()
{
    SV::route_t& route = SV::m_queue.peek();
    if (route.cnt && SV::m_result == STORAGE_OK) {
        route.sub_cnt   = SV::m_page.getPrevAddress();
        route.addr      = SV::m_page.getAddress();
        // route.sub_addr += STORAGE_PAGE_SIZE;
        uint32_t cnt = STORAGE_PAGE_PAYLOAD_SIZE;
        if (route.len - route.cnt < cnt) {
            cnt = route.len - route.cnt;
        }
        if (route.cnt) {
            route.cnt -= cnt;
        }
        fsm_gc_push_event(&st_at_rewrite_fsm, &done_e);
    } else if (SV::m_result == STORAGE_OK) {
        route.sub_cnt  = route.addr;
        route.sub_addr = route.addr + STORAGE_PAGE_SIZE;
        fsm_gc_push_event(&st_at_rewrite_fsm, &success_e);
    } else {
        fsm_gc_push_event(&st_at_rewrite_fsm, &error_e);
    }
}

void _rewrite_prev_hd_a()
{
    SV::route_t& route = SV::m_queue.peek();
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
		0,
        0,
        {}
    };
    SV::route(header);
}

void _rewrite_prev_hd_s()
{
    SV::routeRes(&st_at_rewrite_fsm);
}

void _rewrite_find_prev_a()
{
    SV::route_t& route = SV::m_queue.peek();
    SV::route_t find{
        SV::ST_FIND,
        route.sub_addr,
        STORAGE_PAGE_SIZE,
        0,
        {},
        0,
        (uint8_t*)&route.sub_addr,
        nullptr,
        FIND_MODE_EMPTY,
        0,
        0,
        {}
    };
    SV::route(find);
    route.timer.reset();
}

void _rewrite_find_prev_s()
{
    SV::routeRes(&st_at_rewrite_fsm);
}

void _rewrite_back_a()
{
    SV::route_t& route = SV::m_queue.peek();
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
		0,
        0,
        {}
    };
    memcpy(_delete.prefix, route.prefix, sizeof(route.prefix));
    SV::route(_delete);
    route.timer.start(STORAGE_DELAY_MS);
}

void _rewrite_back_s()
{
    SV::route_t& route = SV::m_queue.peek();
    fsm_gc_clear(&st_at_rewrite_fsm);
    if (route.addr >= AT::getStorageSize()) {
        SV::m_result = STORAGE_OOM;
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


FSM_GC_CREATE_ACTION(delete_find_a,      _delete_find_a)
FSM_GC_CREATE_ACTION(delete_found_a,     _delete_found_a)
FSM_GC_CREATE_ACTION(delete_header_a,    _delete_header_a)
FSM_GC_CREATE_ACTION(delete_empty_a,     _delete_empty_a)
FSM_GC_CREATE_ACTION(delete_hd_next_a,   _delete_hd_next_a)
FSM_GC_CREATE_ACTION(delete_save_a,      _delete_save_a)
FSM_GC_CREATE_ACTION(delete_rd_erase_a,  _delete_rd_erase_a)
FSM_GC_CREATE_ACTION(delete_rd_next_a,   _delete_rd_next_a)
FSM_GC_CREATE_ACTION(delete_sv_erase_a,  _delete_sv_erase_a)
FSM_GC_CREATE_ACTION(delete_sv_header_a, _delete_sv_header_a)
FSM_GC_CREATE_ACTION(delete_sv_check_a,  _delete_sv_check_a)
FSM_GC_CREATE_ACTION(delete_success_a,   _delete_success_a)
FSM_GC_CREATE_ACTION(delate_error_a,     _delete_error_a)

FSM_GC_CREATE_STATE(delete_init_s,     _delete_init_s)
FSM_GC_CREATE_STATE(delete_find_s,     _delete_find_s)
FSM_GC_CREATE_STATE(delete_header_s,   _delete_header_s)
FSM_GC_CREATE_STATE(delete_empty_s,    _delete_empty_s)
FSM_GC_CREATE_STATE(delete_save_s,     _delete_save_s)
FSM_GC_CREATE_STATE(delete_rd_erase_s, _delete_rd_erase_s)
FSM_GC_CREATE_STATE(delete_sv_erase_s, _delete_sv_erase_s)
FSM_GC_CREATE_STATE(delete_sv_check_s, _delete_sv_check_s)

FSM_GC_CREATE_TABLE(
    st_at_delete_fsm_table,
    {&delete_init_s,     &success_e, &delete_find_s,     &delete_find_a},
    {&delete_init_s,     &header_e,  &delete_sv_erase_s, &delete_sv_header_a},
    {&delete_init_s,     &find_e,    &delete_header_s,   &delete_header_a},

    {&delete_find_s,     &done_e,    &delete_empty_s,    &delete_empty_a},
    {&delete_find_s,     &error_e,   &delete_sv_erase_s, &delete_sv_erase_a},
    
    {&delete_empty_s,    &success_e, &delete_init_s,     &delete_success_a},
    {&delete_empty_s,    &done_e,    &delete_header_s,   &delete_found_a},

    {&delete_header_s,   &success_e, &delete_init_s,     &delete_success_a},
    {&delete_header_s,   &done_e,    &delete_save_s,     &delete_save_a},
    {&delete_header_s,   &error_e,   &delete_rd_erase_s, &delete_rd_erase_a},

    {&delete_save_s,     &done_e,    &delete_header_s,   &delete_hd_next_a},
    {&delete_save_s,     &error_e,   &delete_rd_erase_s, &delete_rd_erase_a},

    {&delete_rd_erase_s, &success_e, &delete_init_s,     &delete_success_a},
    {&delete_rd_erase_s, &next_e,    &delete_sv_erase_s, &delete_sv_erase_a},
    {&delete_rd_erase_s, &done_e,    &delete_rd_erase_s, &delete_rd_next_a},
    {&delete_rd_erase_s, &error_e,   &delete_rd_erase_s, &delete_rd_erase_a},

    {&delete_sv_erase_s, &done_e,    &delete_sv_check_s, &delete_sv_check_a},
    {&delete_sv_erase_s, &error_e,   &delete_init_s,     &delate_error_a},

    {&delete_sv_check_s, &success_e, &delete_header_s,   &delete_hd_next_a},
    {&delete_sv_check_s, &header_e,  &delete_init_s,     &delete_success_a},
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

static void _delete_read(uint32_t addr)
{
    SV::route_t& route = SV::m_queue.peek();
    StorageStatus status = AT::driverCallback()->asyncRead(addr, (uint8_t*)&SV::m_page.page, sizeof(SV::m_page.page));
    if (status == STORAGE_OK) {
        route.timer.start(STORAGE_DELAY_MS);
    }
    else {
        SV::m_result = status;
        fsm_gc_push_event(&st_at_read_fsm, &error_e);
    }
}

void _delete_init_s()
{
    SV::route_t& route = SV::m_queue.peek();
    route.cnt = 0;
    route.sub_cnt = 0;
    if (route.prefix[0] == 0) {
        if (SM::isMacroblockAddress(route.addr)) {
            fsm_gc_push_event(&st_at_delete_fsm, &header_e);
        }
        else {
            fsm_gc_push_event(&st_at_delete_fsm, &success_e);
        }
    } else {
        route.addr = 0;
        fsm_gc_push_event(&st_at_delete_fsm, &find_e);
    }
}

void _delete_find_a()
{
    SV::route_t& route = SV::m_queue.peek();
    _delete_read(route.addr);
}

void _delete_find_s()
{
    SV::route_t& route = SV::m_queue.peek();
    if (route.timer.wait()) {
        return;
    }
    fsm_gc_push_event(&st_at_delete_fsm, &error_e);
}

void _delete_empty_a()
{
    SV::route_t& route = SV::m_queue.peek();
    if (SV::m_page.empty()) {
        fsm_gc_push_event(&st_at_delete_fsm, &success_e);
    } else {
        fsm_gc_push_event(&st_at_delete_fsm, &done_e);
    }
}

void _delete_empty_s() {}

void _delete_found_a()
{
    SV::route_t& route = SV::m_queue.peek();
    memcpy(route.prefix, SV::m_page.page.header.prefix, STORAGE_PAGE_PREFIX_SIZE);
    route.id = SV::m_page.page.header.id;
    _delete_header_a();
}

void _delete_header_a()
{
    SV::route_t& route = SV::m_queue.peek();
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
		0,
        0,
        {}
    };
    SV::route(header);
}

void _delete_hd_next_a()
{
    SV::route_t& route = SV::m_queue.peek();
    route.addr += SM::getMacroblockSize();
    if (route.addr >= AT::getStorageSize()) {
        fsm_gc_push_event(&st_at_delete_fsm, &success_e);
        return;
    }
    _delete_header_a();
}

void _delete_header_s()
{
    SV::route_t& route = SV::m_queue.peek();
    if (route.addr >= AT::getStorageSize()) {
        return;
    }
    SV::routeRes(&st_at_delete_fsm);
}

void _delete_save_a()
{
    SV::route_t& route = SV::m_queue.peek();
    for (unsigned i = 0; i < Header::PAGES_COUNT; i++) {
	    Header::MetaUnit* metaUnitPtr = SV::m_header.data->metaUnits;
        if (!memcmp((*metaUnitPtr).prefix, route.prefix, STORAGE_PAGE_PREFIX_SIZE) &&
            (*metaUnitPtr).id == route.id
        ) {
	        SV::m_header.setAddressEmpty((i + SM::RESERVED_PAGES_COUNT) * STORAGE_PAGE_SIZE);
	        (*metaUnitPtr).id = 0;
        }
    }
    SV::route_t save{
        SV::ST_HEADER,
        SV::m_header.getAddress(),
        STORAGE_PAGE_SIZE,
        0,
        {},
        0,
        nullptr,
        (uint8_t*)SV::m_header.header,
        (StorageFindMode)0,
		0,
        0,
        {}
    };
    SV::route(save);
    route.timer.start(STORAGE_DELAY_MS);
}

void _delete_save_s()
{
    SV::routeRes(&st_at_delete_fsm);
}

void _delete_rd_erase_a()
{
    SV::route_t& route = SV::m_queue.peek();
    route.cnt = __rm_mod(route.addr, SM::PAGES_COUNT * STORAGE_PAGE_SIZE) + SM::RESERVED_PAGES_COUNT * STORAGE_PAGE_SIZE;
    _delete_read(route.cnt);
}

void _delete_rd_next_a()
{
    SV::route_t& route = SV::m_queue.peek();
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
    _delete_read(route.cnt);
}

void _delete_rd_erase_s()
{
    SV::route_t& route = SV::m_queue.peek();
    if (route.timer.wait()) {
        return;
    }
    fsm_gc_push_event(&st_at_delete_fsm, &error_e);
}

void _delete_sv_erase_a()
{
    SV::route_t& route = SV::m_queue.peek();
    StorageStatus status = AT::driverCallback()->asyncErase(SV::m_addrs, route.sub_cnt);
    if (status == STORAGE_OK) {
        route.timer.start(STORAGE_DELAY_MS);
    } else {
        SV::m_result = status;
        fsm_gc_push_event(&st_at_delete_fsm, &error_e);
    } 
}

void _delete_sv_header_a()
{
    SV::route_t& route = SV::m_queue.peek();
    uint32_t addr = __rm_mod(route.addr, SM::getMacroblockSize());
    for (unsigned i = 0; i < SM::RESERVED_PAGES_COUNT; i++) {
        SV::m_addrs[route.sub_cnt++] = addr + i * STORAGE_PAGE_SIZE;
    }
    _delete_sv_erase_a();
}

void _delete_sv_erase_s()
{
    SV::route_t& route = SV::m_queue.peek();
    if (route.timer.wait()) {
        return;
    }
    fsm_gc_push_event(&st_at_delete_fsm, &error_e);
}

void _delete_sv_check_a()
{
    if (SM::isMacroblockAddress(SV::m_queue.peek().addr)) {
        fsm_gc_push_event(&st_at_delete_fsm, &header_e);
    } else {
        fsm_gc_push_event(&st_at_delete_fsm, &success_e);
    }
}

void _delete_sv_check_s() {}

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


FSM_GC_CREATE_ACTION(header_read_a,      _header_read_a)
FSM_GC_CREATE_ACTION(header_next_a,      _header_next_a)
FSM_GC_CREATE_ACTION(header_check_a,     _header_check_a)
FSM_GC_CREATE_ACTION(header_check_err_a, _header_check_err_a)
FSM_GC_CREATE_ACTION(header_save_a,      _header_save_a)
FSM_GC_CREATE_ACTION(header_create_a,    _header_create_a)
FSM_GC_CREATE_ACTION(header_cr_next_a,   _header_cr_next_a)
FSM_GC_CREATE_ACTION(header_cr_err_a,    _header_cr_err_a)
FSM_GC_CREATE_ACTION(header_sv_next_a,   _header_sv_next_a)
FSM_GC_CREATE_ACTION(header_rd_crc_a,    _header_rd_crc_a)
FSM_GC_CREATE_ACTION(header_crc_a,       _header_crc_a)
FSM_GC_CREATE_ACTION(header_success_a,   _header_success_a)
FSM_GC_CREATE_ACTION(header_error_a,     _header_error_a)

FSM_GC_CREATE_STATE(header_init_s,       _header_init_s)
FSM_GC_CREATE_STATE(header_read_s,       _header_read_s)
FSM_GC_CREATE_STATE(header_check_s,      _header_check_s)
FSM_GC_CREATE_STATE(header_create_s,     _header_create_s)
FSM_GC_CREATE_STATE(header_save_s,       _header_save_s)
FSM_GC_CREATE_STATE(header_rd_crc_s,     _header_rd_crc_s)
FSM_GC_CREATE_STATE(header_crc_s,        _header_crc_s)

FSM_GC_CREATE_TABLE(
    st_at_header_fsm_table,
    {&header_init_s,   &success_e, &header_read_s,   &header_read_a},
    {&header_init_s,   &write_e,   &header_save_s,   &header_save_a},

    {&header_read_s,   &done_e,    &header_check_s,  &header_check_a},
    {&header_read_s,   &error_e,   &header_check_s,  &header_check_err_a},
    {&header_read_s,   &busy_e,    &header_init_s,   &header_error_a},
    
    {&header_check_s,  &success_e, &header_init_s,   &header_success_a},
    {&header_check_s,  &end_e,     &header_create_s, &header_create_a},
    {&header_check_s,  &error_e,   &header_read_s,   &header_next_a},

    {&header_create_s, &success_e, &header_save_s,   &header_save_a},
    {&header_create_s, &done_e,    &header_create_s, &header_cr_next_a},
    {&header_create_s, &error_e,   &header_create_s, &header_cr_err_a},

    {&header_save_s,   &done_e,    &header_rd_crc_s, &header_rd_crc_a},
    {&header_save_s,   &error_e,   &header_save_s,   &header_sv_next_a},
    {&header_save_s,   &end_e,     &header_init_s,   &header_success_a},

    {&header_rd_crc_s, &done_e,   &header_crc_s,     &header_crc_a},
    {&header_rd_crc_s, &error_e,  &header_save_s,    &header_sv_next_a},

    {&header_crc_s,    &done_e,   &header_init_s,    &header_success_a},
    {&header_crc_s,    &error_e,  &header_save_s,    &header_sv_next_a},
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
    SV::route_t& route = SV::m_queue.peek();
    route.addr = __rm_mod(route.addr, SM::getMacroblockSize());
    SV::m_header.setAddress(route.addr);
    if (route.src) {
        fsm_gc_push_event(&st_at_header_fsm, &write_e);
    } else {
        fsm_gc_push_event(&st_at_header_fsm, &success_e);
    }
}

static void _header_read(const uint32_t address, uint8_t* dst)
{
    fsm_gc_clear(&st_at_header_fsm);
    SV::route_t& route = SV::m_queue.peek();
    if (route.addr + STORAGE_PAGE_SIZE > AT::getStorageSize()) {
        SV::m_result = STORAGE_OOM;
        fsm_gc_push_event(&st_at_header_fsm, &error_e);
    } else {
        StorageStatus status = AT::driverCallback()->asyncRead(address, dst, sizeof(SV::m_page.page));
        if (status == STORAGE_BUSY) {
            SV::m_result = status;
            fsm_gc_push_event(&st_at_header_fsm, &busy_e);
        }
        if (status != STORAGE_OK) {
            SV::m_result = status;
            fsm_gc_push_event(&st_at_header_fsm, &error_e);
        } 
    }
    route.timer.start(STORAGE_DELAY_MS);
}

void _header_read_a()
{
    SV::route_t& route = SV::m_queue.peek();
    route.cnt = 0;
    uint32_t addr = SM::getMacroblockAddress(SM::getMacroblockIndex(route.addr));
    SV::m_header.setAddress(addr);
    _header_read(route.addr, (uint8_t*)SV::m_header.header);
}

void _header_next_a()
{
    SV::route_t& route = SV::m_queue.peek();
    route.cnt++;
    uint32_t addr = SM::getMacroblockAddress(SM::getMacroblockIndex(route.addr)) + route.cnt * STORAGE_PAGE_SIZE;
    SV::m_header.setAddress(addr);
    _header_read(addr, (uint8_t*)SV::m_header.header);
    if (route.cnt >= SM::RESERVED_PAGES_COUNT) {
        fsm_gc_push_event(&st_at_header_fsm, &error_e);
    }
}

void _header_read_s()
{
    SV::route_t& route = SV::m_queue.peek();
    uint32_t addr = route.addr + route.cnt * STORAGE_PAGE_SIZE;
    if (route.timer.wait()) {
        return;
    }
    fsm_gc_push_event(&st_at_header_fsm, &error_e);
}

void _header_check_a()
{
    SV::route_t& route = SV::m_queue.peek();
    if (!SV::m_header.validate()) {
        SV::m_result = STORAGE_HEADER_ERROR;
        fsm_gc_push_event(&st_at_header_fsm, &error_e);
        return;
    }
    fsm_gc_push_event(&st_at_header_fsm, &success_e);
}

void _header_check_err_a()
{
    fsm_gc_clear(&st_at_header_fsm);
    SV::route_t& route = SV::m_queue.peek();
    uint32_t addr = route.addr + (route.cnt + 1) * STORAGE_PAGE_SIZE;
    if (!SM::isMacroblockAddress(addr)) {
        fsm_gc_push_event(&st_at_header_fsm, &end_e);
        return;
    }
    fsm_gc_push_event(&st_at_header_fsm, &error_e);
}

void _header_check_s() {}

void _header_create_a()
{
    for (unsigned i = 0; i < Header::PAGES_COUNT; i++) {
        SV::m_header.setAddressEmpty((i + SM::RESERVED_PAGES_COUNT) * STORAGE_PAGE_SIZE);
    }
    SV::route_t& route = SV::m_queue.peek();
    route.cnt = 0;
    _header_read(
        SM::getPageAddressByIndex(SM::getMacroblockIndex(route.addr), 0), 
        (uint8_t*)&SV::m_page.page
    );
}

void _header_cr_next_a()
{
    SV::route_t& route = SV::m_queue.peek();
    memcpy(SV::m_header.data->metaUnits[route.cnt].prefix, SV::m_page.page.header.prefix, STORAGE_PAGE_PREFIX_SIZE);
    SV::m_header.data->metaUnits[route.cnt].id = SV::m_page.page.header.id;
    _header_cr_err_a();
}

void _header_cr_err_a()
{
    SV::route_t& route = SV::m_queue.peek();
    route.cnt++;
    uint32_t addr = route.addr + SM::RESERVED_PAGES_COUNT * STORAGE_PAGE_SIZE + route.cnt * STORAGE_PAGE_SIZE;
    if (route.cnt >= Header::PAGES_COUNT) {
        fsm_gc_push_event(&st_at_header_fsm, &success_e);
        return;
    }
    _header_read(
        SM::getPageAddressByIndex(SM::getMacroblockIndex(route.addr), route.cnt),
        (uint8_t*)&SV::m_page.page
    );
}

void _header_create_s()
{
    SV::route_t& route = SV::m_queue.peek();
    if (route.timer.wait()) {
        return;
    }
    fsm_gc_push_event(&st_at_header_fsm, &done_e);
}

static void _header_save(const uint32_t address)
{
    SV::m_header.prepareSave();
    SV::route_t& route = SV::m_queue.peek();
    StorageStatus status = AT::driverCallback()->asyncWrite(address,  (uint8_t*)SV::m_header.header, STORAGE_PAGE_SIZE);
    if (status == STORAGE_OK) {
        route.timer.start(STORAGE_DELAY_MS);
    } else {
        SV::m_result = status;
        fsm_gc_push_event(&st_at_header_fsm, &error_e);
    }
}

void _header_save_a()
{
    SV::route_t& route = SV::m_queue.peek();
    route.cnt = 0;
    _header_save(route.addr);
}

void _header_sv_next_a()
{
    fsm_gc_clear(&st_at_header_fsm);
    SV::route_t& route = SV::m_queue.peek();
    route.cnt++;
    if (route.cnt >= SM::RESERVED_PAGES_COUNT) {
        fsm_gc_push_event(&st_at_header_fsm, &end_e);
        return;
    }
    fsm_gc_clear(&st_at_header_fsm);
    _header_save(route.addr + route.cnt * STORAGE_PAGE_SIZE);
}

void _header_save_s()
{
    SV::route_t& route = SV::m_queue.peek();
    if (route.cnt >= SM::RESERVED_PAGES_COUNT) {
        return;
    }
    if (route.timer.wait()) {
        return;
    }
    fsm_gc_push_event(&st_at_header_fsm, &error_e);
}

void _header_rd_crc_a()
{
    SV::route_t& route = SV::m_queue.peek();
    StorageStatus status = AT::driverCallback()->asyncRead(route.addr + route.cnt * STORAGE_PAGE_SIZE, (uint8_t*)&SV::m_page.page, STORAGE_PAGE_SIZE);
    if (status != STORAGE_OK) {
        SV::m_result = status;
        fsm_gc_push_event(&st_at_header_fsm, &error_e);
    }
    route.timer.start(STORAGE_DELAY_MS);
}

void _header_rd_crc_s()
{
    SV::route_t& route = SV::m_queue.peek();
    if (route.timer.wait()) {
        return;
    }
    fsm_gc_push_event(&st_at_header_fsm, &error_e);
}

void _header_crc_a()
{
    SV::route_t& route = SV::m_queue.peek();
    if (SV::m_header.header->crc == SV::m_page.page.crc) {
        fsm_gc_push_event(&st_at_header_fsm, &done_e);
    }
    else {
        fsm_gc_push_event(&st_at_header_fsm, &error_e);
    }
}

void _header_crc_s() {}

void _header_success_a()
{
    SV::m_result = STORAGE_OK;
    fsm_gc_clear(&st_at_header_fsm);
    fsm_gc_push_event(&st_at_fsm, &done_e);
}

void _header_error_a()
{
    if (SV::m_result == STORAGE_OK) {
        SV::m_result = STORAGE_HEADER_ERROR;
    }
    fsm_gc_clear(&st_at_header_fsm);
    fsm_gc_push_event(&st_at_fsm, &done_e);
}

