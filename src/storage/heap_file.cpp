
#include <cstring>
#include "include/storage/heap_file.h"

//   ----------- byte-level / slot helper -----------
HeapFile::PageHeader HeapFile::readHeader(const Page* page){
    PageHeader header;
    const char* data = page->getData();
    std::memcpy(&header.next_page_id, data, sizeof(page_id_t));
    std::memcpy(&header.num_slots, data + sizeof(page_id_t), sizeof(uint16_t));
    std::memcpy(&header.free_space_offset, data + sizeof(page_id_t) + sizeof(uint16_t), sizeof(uint16_t));
    return header;
}

void HeapFile::writeHeader(Page* page, PageHeader& header){
    char* data= page->getData();
    std::memcpy(data, &header.next_page_id, sizeof(page_id_t));
    std::memcpy(data + sizeof(page_id_t), &header.num_slots, sizeof(uint16_t));
    std::memcpy(data + sizeof(page_id_t) + sizeof(uint16_t), &header.free_space_offset, sizeof(uint16_t));
    return;
}

HeapFile::Slot HeapFile::readSlot(const Page* page, uint16_t slot_num){
    Slot slot;
    const char* data = page->getData();
    const char* base = data + HEADER_SIZE + slot_num * SLOT_SIZE;
    std::memcpy(&slot.offset, base, sizeof(uint16_t));
    std::memcpy(&slot.length, base + sizeof(uint16_t), sizeof(uint16_t));
    return slot;
}

void HeapFile::writeSlot(Page* page, uint16_t slot_num, const Slot& slot){
    char* data = page->getData();
    char* base = data + HEADER_SIZE + slot_num * SLOT_SIZE;
    std::memcpy(base, &slot.offset, sizeof(uint16_t));
    std::memcpy(base + sizeof(uint16_t), &slot.length, sizeof(uint16_t));
    return;
}

void HeapFile::initializeEmptyPage(Page* page, page_id_t next_page_id){
    PageHeader header;
    header.next_page_id = next_page_id;
    header.num_slots = 0;
    header.free_space_offset = static_cast<uint16_t>(PAGE_SIZE);
    writeHeader(page, header);
}

void HeapFile::initializeNewHeapPage(Page* page){
    initializeEmptyPage(page, INVALID_PAGE_ID);
}

uint16_t HeapFile::freeSpaceOnPage(const PageHeader& header){
    uint16_t slot_array_end = HEADER_SIZE + header.num_slots * SLOT_SIZE;
    return header.free_space_offset - slot_array_end;
}

//  -----Heap file ------
page_id_t HeapFile::getFirstPageId(){ return first_page_id_; }

HeapFile::HeapFile(BufferPoolManager* bpm, page_id_t first_page_id) : bpm_(bpm), first_page_id_(first_page_id),last_page_id_(first_page_id){}

std::optional<RecordID> HeapFile::insertRecord(const char* data, uint16_t len){
    const uint16_t need  = (len+SLOT_SIZE);

    page_id_t page_id = last_page_id_;
    while(true){
        Page* page = bpm_->fetchPage(page_id);
        if(page == nullptr)
        {
            return std::nullopt;
        }
        PageHeader header = readHeader(page);
        uint16_t free_space = freeSpaceOnPage(header);

        if(free_space>= need)
        {
            uint16_t new_offset = header.free_space_offset - len;
            std::memcpy(page->getData() + new_offset, data, len);

            uint16_t slot_num = header.num_slots;
            writeSlot(page, slot_num, Slot{new_offset, len});

            header.num_slots += 1;
            header.free_space_offset = new_offset;
            writeHeader(page, header);

            last_page_id_ = page_id; //cache for next insert
            
            if(!bpm_->unpinPage(page_id, true)){
                return std::nullopt;
            }

            return RecordID{page_id, slot_num};
        }
        // if not fit on page 
        if(header.next_page_id != INVALID_PAGE_ID){
            page_id_t next = header.next_page_id;
            if(next == page_id)
            { //this new happen but have a safe gaurd to avoid infinite loop(in case failure to write due to some reason)
                bpm_->unpinPage(page_id, false);
                return std::nullopt;
            }
            if(!bpm_->unpinPage(page_id, false)){ // is_dirty = false
                return std::nullopt;
            }
            page_id = next;
            continue;
        }
        // this last page in chain and its full allocae new one and link it
        page_id_t new_page_id;
        Page* new_page = bpm_->newPage(&new_page_id);
        if(new_page == nullptr){
            bpm_->unpinPage(page_id, /*is_dirty=*/false);
            return std::nullopt; // pool exhausted , cannot grow
        }

        initializeEmptyPage(new_page, INVALID_PAGE_ID);
        if(!bpm_->unpinPage(new_page_id, false)){
            return std::nullopt;
        }

        header.next_page_id = new_page_id;
        writeHeader(page, header);
        if(!bpm_->unpinPage(page_id, /*is_dirty=*/true)){
            return std::nullopt;
        }

        page_id = new_page_id;
        // loop again
    }
}

std::optional<std::vector<char>> HeapFile::getRecord(const RecordID& rid){
    Page* page = bpm_->fetchPage(rid.page_id);
    if(page == nullptr)
    {
        return std::nullopt;
    }
    Slot slot = readSlot(page, rid.slot_id);
    if(slot.length == 0)
    {// slot deleted
        (void)bpm_->unpinPage(rid.page_id, false);
        return std::nullopt;
    }
    std::vector<char> record(slot.length);
    std::memcpy(record.data(), page->getData() + slot.offset, slot.length);

    (void)bpm_->unpinPage(rid.page_id, false);
    return record;

}

bool HeapFile::deleteRecord(const RecordID& rid)
{
    Page* page = bpm_->fetchPage(rid.page_id);
    if(page==nullptr)
    {
        return false;
    }
    Slot slot = readSlot(page, rid.slot_id);
    if(slot.length == 0)
    {
        (void)bpm_->unpinPage(rid.page_id, false);
        return false;
    }
    writeSlot(page, rid.slot_id, Slot{slot.offset, 0});
    if(!bpm_->unpinPage(rid.page_id, true)){
        return false;
    }
    return true;
}

std::optional<RecordID> HeapFile::updateRecord(const RecordID& rid, const char* new_data, uint16_t new_len)
{
    Page* page = bpm_->fetchPage(rid.page_id);
    if(page == nullptr) 
        return std::nullopt;
    
    Slot slot = readSlot(page, rid.slot_id);
    if(slot.length == 0){
        (void)bpm_->unpinPage(rid.page_id, false);
        return std::nullopt; // alrready deleted
    }
    if(new_len <= slot.length)
    {
        std::memcpy(page->getData() + slot.offset, new_data, new_len);
        writeSlot(page, rid.slot_id, Slot{slot.offset, new_len});

        if(!bpm_->unpinPage(rid.page_id, true))
        {
            return std::nullopt;
        }
        return rid;
    }

    writeSlot(page, rid.slot_id, Slot{slot.offset, 0});
    if(!bpm_->unpinPage(rid.page_id, true))
    {
        return std::nullopt;
    }
    return insertRecord(new_data, new_len);
}


