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
    return (length + sizeof(Slot)) <= free_space();
}
/* ==========================IGNORE,  may  need in future ======================*/
// void Page::compact_page()
// {
//     uint8_t new_data[Config::PAGE_SIZE];
//     uint16_t new_row_offset = Config::PAGE_SIZE;
    
//     for(int slot_id =0; slot_id < header.num_slots; slot_id++)
//     {
//         // get offset
//         uint16_t current_slot_offset = sizeof(PageHeader) + slot_id * sizeof(Slot);
//         uint16_t current_row_offset = read_u16(current_slot_offset);
//         uint16_t current_row_len = read_u16(current_slot_offset + sizeof(Slot::offset));
        
//         if(current_row_len == 0){
//             memcpy(new_data + current_slot_offset, data+current_slot_offset, sizeof(Slot));
//             continue;
//         }

//         new_row_offset -= current_row_offset;
//         memcpy(new_data + new_row_offset, data + current_row_offset, current_row_len);
//         memcpy(new_data + current_slot_offset, &new_row_offset, sizeof(new_row_offset));
//         memcpy(new_data + current_slot_offset + sizeof(Slot::offset), &current_row_len, sizof(current_row_len));

//     }
//     // update header
//     header.row_offset = new_row_offset;
//     update_header();

//     // copy from slots and record
//     for(int i = sizeof(header); i < Config::PAGE_SIZE; i++)
//     {
//         data[i] = new_data[i];
//     }
//     return ;
// }

/* =============================================================*/

void Page::serialize(char* dest)
{
    update_header();
    memcpy(dest, data, Config::PAGE_SIZE);
}

void Page::deserialize(const char* src)
{
    memcpy(data, src, Config::PAGE_SIZE);
    header.page_id = read_u16(0);
    header.num_slots = read_u16(4);
    header.slot_offset = read_u16(8);
    header.row_offset = read_u16(12);
}

void Page::init(uint16_t page_id){
    header.page_id =page_id;
    header.num_slots = 0;
    header.slot_offset = sizeof(PageHeader);
    header.row_offset = sizeof(Config::PAGE_SIZE);
    update_header();
}

int Page::free_space()
{
    return header.row_offset - header.slot_offset -1;
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

void Page::delete_record(uint16_t slot_id)
{
        int offset = sizeof(PageHeader) + slot_id * sizeof(Slot);
        int len = read_u16(offset + sizeof(Slot::offset));
        // try to delete deleted tuple
        if(len == 0) return ;
        write_u16(offset + sizeof(Slot::offset), 0);
        return ;
}

bool Page::update_record(uint16_t slot_id, const char* record, uint16_t new_len)
{
    uint16_t slot_offset = sizeof(header) + slot_id * sizeof(Slot);
    uint16_t row_offset = read_u16(slot_offset);
    uint16_t row_len = read_u16(slot_offset + sizeof(Slot::offset));

    if(row_len >= new_len)
    {
        memcpy(data + row_offset, record, new_len);
        //update len
        memcpy(data + slot_offset + sizeof(Slot::offset), &new_len, sizeof(Slot::length));
        return true;
    }
    // need for compact
   
    

    
    if(!has_space_for(new_len)){
        return false;
    }  // no sufficent space 
    
    header.row_offset -= new_len;
    memcpy(data + header.row_offset, record, new_len);
    write_u16(slot_offset, row_offset);
    write_u16(slot_offset + sizof(Slot::offset),  new_len);
    return true;
}






