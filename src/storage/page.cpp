#include "include/storage/page.h"
#include <algorithm>
#include <cassert>
#include <stdexcept>

void Page::sync_header() const {
    std::memcpy(const_cast<Page*>(this)->data_.data(), &header_, sizeof(PageHeader));
}

void Page::load_header() 
{
    std::memcpy(&header_, data_.data(), sizeof(PageHeader));
}

void Page::compact()
{
    struct Entry { uint16_t slot_id, offset, length; };
    std::vector<Entry> live;
    for (uint16_t i = 0; i < header_.num_slots; ++i) {
        const SlotEntry& s = slot_at(i);
        if (s.length > 0) live.push_back({i, s.offset, s.length});
    }
    std::sort(live.begin(), live.end(), [](const Entry& a, const Entry& b){
        return a.offset > b.offset;
    });

    uint16_t write_end = static_cast<uint16_t>(Config::PAGE_SIZE);
    for (auto& e : live) {
        write_end -= e.length;
        if (e.offset != write_end)
            std::memmove(data_.data() + write_end, data_.data() + e.offset, e.length);
        slot_at(e.slot_id).offset = write_end;
    }
    header_.free_space_end = write_end;
    const std::size_t used = Config::PAGE_HEADER_SIZE + header_.num_slots * Config::SLOT_SIZE;
    header_.free_space = static_cast<uint32_t>(write_end - used);
    dirty_ = true;
    
    sync_header();

    
}


Page::RawData Page::to_bytes() const
{
    sync_header();
    return data_;
}

Page Page::from_bytes(const RawData& raw)
{
    Page p(INVALID_PAGE_ID);
        p.data_ = raw;
        p.load_header();
        return p;
}

Page::Page(uint32_t page_id) : header_{}, data_{}, dirty_(false){
    header_.page_id =page_id;
    header_.num_slots = 0;
    header_.free_space_end = static_cast<uint16_t>(Config::PAGE_SIZE);
    header_.free_space =  static_cast<uint32_t>(Config::PAGE_SIZE - Config::PAGE_HEADER_SIZE);
    sync_header();
}



uint16_t Page::insert_record(std::vector<const std::byte> data)
{
    if(data.empty()){
        throw std::invalid_argument("cannot insert empty record");
    }

    const std::size_t record_len =data.size();
    //append new row
    uint16_t slot_id = INVALID_SLOT_ID;
    bool new_slot = false;
    for(uint16_t i=0; i < header_.num_slots; i++)
    {
        if(slot_at(i).length == 0)
        {
            slot_id =i; break;
        }
    }
    if(slot_id == INVALID_PAGE_ID)
    {
        new_slot = true;
        slot_id = header_.num_slots;
    }

    const std::size_t needed = record_len + (new_slot ? Config::SLOT_SIZE : 0);
    if(header_.free_space < needed)
        throw std::runtime_error("not enough space on page");

    const uint16_t new_end = static_cast<uint16_t>(header_.free_space_end - record_len);
    
    memcpy(data_.data() + new_end, data.data(), record_len);
    header_.free_space_end = new_end;

    if(new_slot) header_.num_slots++;
    SlotEntry& slot = slot_at(slot_id);
    slot.offset = new_end;
    slot.length = static_cast<uint16_t>(record_len);
    slot.flags = 0;
    
    header_.free_space -= static_cast<uint32_t>(record_len);
    if(new_slot) header_.free_space -= static_cast<uint32_t>(Config::SLOT_SIZE);

    dirty_ = true;
    sync_header();
    return slot_id;


}

std::optional<std::vector<std::byte>> Page::get_record(uint16_t slot_id) const
{
    if (slot_id >= header_.num_slots) return std::nullopt;
    const SlotEntry& slot = slot_at(slot_id);
    if (slot.length == 0) return std::nullopt;
    const std::byte* src = data_.data() + slot.offset;
    return std::vector<std::byte>(src, src + slot.length);
}

bool Page::delete_record(uint16_t slot_id)
{
    if (slot_id >= header_.num_slots) return false;
    SlotEntry& slot = slot_at(slot_id);
    if (slot.length == 0) return false;
    header_.free_space += static_cast<uint32_t>(slot.length);
    slot.offset = 0;
    slot.length = 0;
    dirty_      = true;
    sync_header();
    return true;
}

