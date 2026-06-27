#pragma once

#include <cstddef>
#include <cstdint>

constexpr size_t PAGE_SIZE = 4096;
using page_id_t = uint32_t;
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
    void ResetMemory();

    char data[PAGE_SIZE];
    page_id_t page_id = INVALID_PAGE_ID;
    bool is_dirty = false;
    int pin_count = 0;
};