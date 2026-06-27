#include "include/storage/page.h"
#include <cstring>

Page::Page() { ResetMemory();}

char* Page::getData() {return data; }

const char* Page::getData() const { return data; }

void Page::setDirty() {
    is_dirty = true; 
}

bool Page::isDirty() const { return is_dirty; }

int Page::getPinCount() const {return pin_count;}

page_id_t Page::getPageId() const {return page_id;}

void Page::ResetMemory() {
    std::memset(data, 0, PAGE_SIZE);
}