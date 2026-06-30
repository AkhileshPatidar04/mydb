#pragma once

#include <cstddef>
#include <cstdint>

constexpr size_t PAGE_SIZE = 4096;
using page_id_t = uint32_t;
using frame_id_t = uint32_t;
const page_id_t INVALID_PAGE_ID = -1;

class Page{
public:
    Page();

    char* getData();
    const char* getData() const;
    page_id_t getPageId() const;
    bool isDirty() const;
    int getPinCount() const;
    void setDirty();
private:
    friend class BufferPoolManager; 
    void resetMemory();

    char data_[PAGE_SIZE];
    page_id_t page_id_ = INVALID_PAGE_ID;
    bool is_dirty_ = false;
    int pin_count_ = 0;
};