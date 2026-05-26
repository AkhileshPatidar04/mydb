#include "include/storage/page.h"
#include <cstring>


void Page::update_header(){
    memcpy(data, &header, sizeof(PageHeader));
}
uint16_t Page::read_u16(uint16_t offset){
    uint16_t value;
    memcpy(&value, data + offset, 4);
} 


void Page::write_u16(uint16_t offset, uint16_t value){
    memcpy(data + offset, &value, 4);
}


bool Page::has_space_for(uint16_t length)
{
    return (length + sizeof(Slot)) < header.row_offset - header.slot_offset -1;
}

void Page::init(uint16_t page_id){
    header.page_id =page_id;
    header.num_slots = 0;
    header.slot_offset = sizeof(PageHeader);
    header.row_offset = sizeof(Config::PAGE_SIZE);
    update_header();
}

uint16_t Page::insert_record(const char* record, uint16_t size)
{
    if(!has_space_for(size)){
        return 0xFF;
    }

    //append new row
    uint16_t slot_id = header.num_slots;

    header.row_offset = header.row_offset - size;
    memcpy(data + header.row_offset, record, size);

    //append new slot
    write_u16(header.slot_offset, header.row_offset);
    header.slot_offset += sizeof(Slot::offset);
    write_u16(header.slot_offset, size);
    header.slot_offset += sizeof(Slot::length);

    header.num_slots += 1;

    update_header();


}

bool Page::get_record(uint16_t slot_id, uint8_t  &out_buf, uint16_t &out_len)
{
    int offset = sizeof(PageHeader) + slot_id*4;

    // read slot
    uint16_t row_offset = read_u16(offset);
    uint16_t len = read_u16(offset+4);
    
    // copy row(or tuple)
    memcpy(&out_buf, data + row_offset, len);
    out_len =len;
    return true;
}








