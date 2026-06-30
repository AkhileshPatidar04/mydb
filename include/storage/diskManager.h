#pragma once

#include <fstream>
#include <string>
#include "include/storage/page.h"
class DiskManager{
public:
    explicit DiskManager(const std::string& db_file);
    ~DiskManager();

    DiskManager(const DiskManager&) = delete;
    DiskManager& operator=(const DiskManager&) = delete;

    bool readPage(page_id_t page_id, char* out_buf);
    bool writePage(page_id_t page_id, const char* in_buf);
    page_id_t allocatePage();

private:
    std::fstream db_io_;
    std::string file_name_;
    int next_page_id_ {0}; // use for new page allocation
};