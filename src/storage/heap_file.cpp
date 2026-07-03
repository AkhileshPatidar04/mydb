
#include <cstring>
#include "include/storage/heap_file.h"

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

void HeapFile::writeSlot(Page* page, uint16_t slot_num, Slot& slot){
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

uint16_t HeapFile::freeSpaceOnPage(const PageHeader& header){
    uint16_t slot_array_end = HEADER_SIZE + header.num_slots * SLOT_SIZE;
    return header.free_space_offset - slot_array_end;
}




