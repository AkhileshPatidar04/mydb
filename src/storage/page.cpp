#include "storage/page.h"
#include <cstring>

Page::Page() { resetMemory();}

char* Page::getData() {return data_; }

const char* Page::getData() const { return data_; }

void Page::setDirty() {
    is_dirty_ = true;
}

bool Page::isDirty() const { return is_dirty_; }

int Page::getPinCount() const {return pin_count_;}

page_id_t Page::getPageId() const {return page_id_;}

void Page::resetMemory() {
    std::memset(data_, 0, PAGE_SIZE);
}