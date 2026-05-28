#pragma once

#include <cstdint>
#include <array>
#include "constants.h"



struct PageHeader{
    uint16_t page_id;
    uint16_t num_slots;
    uint16_t slot_offset;
    uint16_t row_offset;
};

struct Slot{
    uint16_t offset;
    uint16_t length;
};

//    header | slots[0...num_slots] | free space | row record[0...num_slots]
class Page{
public:
    PageHeader header;
    uint8_t data[Config::PAGE_SIZE];
    
    
    void init(uint16_t page_id);
    uint16_t insert_record(const char* record, uint16_t size);       // returns slot_id
    bool get_record(uint16_t slot_id, uint8_t& out_buf, uint16_t& out_len);
    void delete_record(uint16_t slot_id);    
    bool update_record(uint16_t slot_id, const char* record, uint16_t size);

    
    // void compact_page();                // remove delete row(tuple) and shift right record. remove spaces
    void serialize(char* dest);         // write page to raw bytes
    void deserialize(const char* src);  // read page from raw bytes
    int free_space();
    
private:
    
    //utility
    void update_header();
    uint16_t read_u16(uint16_t offset);
    void write_u16(uint16_t offset, uint16_t value);
    bool has_space_for(uint16_t length);
};