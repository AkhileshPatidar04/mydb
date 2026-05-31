#pragma once
/*
page 0 : directory Page
[directoryHeader] | [directoryEntry[]]  atmost 1 pthread_attr_getschedparam

page 1+: data pages(slotted format each 4KB)
*/

#include "include/storage/page.h"
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#pragma pack(push, 1)
struct DirectoryHeader{
    uint32_t num_data_pages{0};
    uint8_t _pad[12]       {};
};
#pragma pack(pop)

#pragma pack(push, 1)
struct DirectoryEntry{
    uint16_t page_id{INVALID_PAGE_ID};
    uint16_t free_space{0};
};
#pragma pack(pop)



constexpr std::size_t MAX_DIR_ENTRIES = (Config::PAGE_SIZE - Config::PAGE_HEADER_SIZE - sizeof(DirectoryHeader)) / sizeof(DirectoryEntry);

class HeapFile{
    std::filesystem::path path_;
    std::fstream file_;
    DirectoryHeader dir_header_ {};
    std::vector<DirectoryEntry> dir_entries_;
    
    
public:
    explicit HeapFile(const std::filesystem::path& path);
    ~HeapFile();


    // Non-copyable; moveable
    HeapFile(const HeapFile&) = delete;
    HeapFile& operator=(const HeapFile&) = delete;
    HeapFile(HeapFile&&) noexcept = default;
    HeapFile& operator=(HeapFile&&) noexcept = default;



    RecordID insert_record(std::vector<const std::byte> data);     // raw bytes as char(may allocate new page)  and return RecordID

    std::vector<std::byte> get_record(const RecordID& rid);   //  get raw bytes(as char), return   empty vector if deleted


    // IO operation
    Page read_page(uint32_t page_id);
    void write_page(const Page& page);
    Page allocate_page();

    // helper
    uint32_t num_data_pages() const {return dir_header_.num_data_pages; }
    const std::filesystem::path& path() const {return path_;}


private:
    void load_directory();
    void flush_directory();
    std::streamoff page_offset(uint32_t page_id) ;

};

