#pragma once

#include <cstdint>
#include <vector>
#include <optional>

#include "include/storage/page.h"
#include "include/storage/buffer_pool_manager.h"
struct RecordID{
    page_id_t page_id =  INVALID_PAGE_ID;
    uint16_t slot_id = 0;

    bool operator==(const RecordID& other) const{
        return page_id == other.page_id && slot_id == other.slot_id;
    }
};

class HeapFile{
    public:
    HeapFile(BufferPoolManager* bpm, page_id_t first_page_id);

    void initializeNewHeapPage(Page* page);

    std::optional<RecordID> insertRecord(const char* data, const uint16_t len);
    std::optional<std::vector<char>> getRecord(const RecordID& rid);
    bool deleteRecord(const RecordID& rid);

    std::optional<RecordID> updateRecord(const RecordID& rid, const char* new_data, const uint16_t new_len);
    
    page_id_t getFirstPageId();
    
    class Iterator{
        public:
        Iterator(HeapFile* file, page_id_t start_page_id);

        bool hasNext() const;
        std::pair<RecordID, std::vector<char>> next();

        private:
        void advanceToNextValidSlot();

        HeapFile* file_;
        page_id_t current_page_id_;
        uint16_t current_slot_;
        bool has_cached_next_ = false;
    };

    Iterator begin();

    private:
    struct PageHeader{
        page_id_t next_page_id;
        uint16_t num_slots;
        uint16_t free_space_offset;
    };
    struct Slot{
        uint16_t offset;
        uint16_t length;
    };
    static PageHeader readHeader(const Page* page);
    static void writeHeader(Page* page, PageHeader& header);
    static Slot readSlot(const Page* page, uint16_t slot_num);
    static void writeSlot(Page* page, uint16_t slot_num, const Slot& slot);
    static void initializeEmptyPage(Page* page, page_id_t next_page_id);
    static uint16_t freeSpaceOnPage(const PageHeader& header);

    static constexpr size_t HEADER_SIZE = sizeof(page_id_t) + sizeof(uint16_t) * 2;
    static constexpr size_t SLOT_SIZE = sizeof(uint16_t)*2;

    BufferPoolManager* bpm_; // non-owning
    page_id_t first_page_id_;
    page_id_t last_page_id_;


};