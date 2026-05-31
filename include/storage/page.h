#pragma once

#include <cstdint>
#include <array>
#include <cstring>
#include <span>
#include <optional>
#include<stdexcept>
#include <vector>
#include<string>
#include "constants.h"


struct RecordID {
    uint32_t page_id {INVALID_PAGE_ID};
    uint16_t slot_id {INVALID_SLOT_ID};
    bool valid() const { return page_id != INVALID_PAGE_ID && slot_id != INVALID_SLOT_ID; }
    bool operator==(const RecordID& other) const {return page_id  == other.page_id && slot_id == other.slot_id;}
};

#pragma pack(push, 1)
struct PageHeader {
    uint32_t page_id        {INVALID_PAGE_ID};
    uint16_t num_slots      {0};
    uint16_t free_space_end {0};
    uint32_t free_space     {0};
    uint8_t  _pad[12]       {};
};
#pragma pack(pop)


#pragma pack(push, 1)
struct SlotEntry {
    uint16_t offset {0};
    uint16_t length {0};
    uint32_t flags  {0};
};
#pragma pack(pop)

//    header | slots[0...num_slots] | free space | row record[0...num_slots]
class Page{
    using RawData = std::array<std::byte, Config::PAGE_SIZE>;

    explicit Page(uint32_t page_id);
    static Page from_bytes(const RawData& raw);
    RawData to_bytes() const;

    uint16_t insert_record(std::vector<const std::byte> data);
    std::optional<std::vector<std::byte>> get_record(uint16_t slot_id) const;
    bool delete_record(uint16_t slot_id);
    void compact();

    uint32_t page_id()    const { return header_.page_id; }
    uint16_t num_slots()  const { return header_.num_slots; }
    uint32_t free_space() const { return header_.free_space; }
    bool     is_dirty()   const { return dirty_; }
    void     set_dirty(bool d)  { dirty_ = d; }

private:
    PageHeader header_ {};
    RawData    data_   {};
    bool       dirty_  {false};

    SlotEntry& slot_at(uint16_t i);
    const SlotEntry& slot_at(uint16_t i) const;
    static constexpr std::size_t slot_offset(uint16_t i) {
        return Config::PAGE_HEADER_SIZE + i * Config::SLOT_SIZE;
    }
    // sync header_ into data_ before returning bytes
    void sync_header() const;
    // load header_ from data_
    void load_header();
    
};