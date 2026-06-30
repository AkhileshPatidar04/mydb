#pragma once


#include "include/storage/page.h"
#include "include/storage/buffer_pool_manager.h"
struct RecordID{
    page_id_t page_id =  INVALID_PAGE_ID;
    uint16_t slot_id = 0;

    bool operator==(const RecordID& other) const{
        return page_id == other.page_id && slot_id == other.slot_id;
    }
};