
#include "include/storage/heap_file.h"
#include <cstring>

//helper
std::streamoff HeapFile::page_offset(uint32_t page_id) 
{
    return page_id * Config::PAGE_SIZE;
}

uint32_t HeapFile::num_data_pages() {
    return dir_header_.num_data_pages;
}


// ructor
HeapFile::HeapFile( std::filesystem::path& path) :path_(path)
{
    bool exists = std::filesytem::exists(path_);

    auto mode = std::ios::in | std::ios::out | std::ios::binary;
    if(!exists) mode |= std::ios::trunc;

    file_.open(path_, mode);
    if(!file_.is_open())
    {
        throw std:: runtime_error("HeapFile: cannot open " + path_.string());
    }

    if(!exists)
    {
        Page dir_page(DIRECTORY_PAGE_ID);

        auto raw = dir_page.to_bytes();
        DirectoryHeader header{};
        std::memcpy(raw.data() + Config::PAGE_HEADER_SIZE, &header, sizeof(header));
        file_.write(raw.data(), Config::PAGE_SIZE);
        file.flush();

    }
    load_directory();

}

HeapFile::~HeapFile()
{
    if(file.is_open())
    {
        flush_directory();
        file_.close();
    }
}

// Directory  IO

void HeapFile::load_directory()
{
    file_.seekg(page_offset(Directory_PAGE_ID));
    Page::RawData raw{};
    file_.read(reinterpret_cast<char*> (raw.data(), Config::PAGE_SIZE));

    std::memcpy(&dir_header_, raw.data()+PAGE_HEADER_SIZE, sizeof(DirectoryHeader)); // skip header

    dir_entries_.resize(dir_header_.num_data_pages);
    const/////////////////////////////////////
    //////////////////////////////
    ////////////////


}


// page IO
Page HeapFile::read_page(uint32_t page_id)
{

}

void HeapFile::write_page(Page& page)
{

}

Page HeapFile::allocate_page()
{

}


// record operations
RecordID HeapFile::insert_record(std::vector<char> data)
{

}
std::vector<char> HeapFile::get_record(RecordID& rid)
{
    
}

