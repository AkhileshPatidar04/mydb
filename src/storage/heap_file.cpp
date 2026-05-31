
#include "include/storage/heap_file.h"
#include <cstring>

//helper
std::streamoff HeapFile::page_offset(uint32_t page_id) 
{
    return page_id * Config::PAGE_SIZE;
}


// ructor
HeapFile::HeapFile(const std::filesystem::path& path) :path_(path)
{
    bool exists = std::filesystem::exists(path_);

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
        file_.write(reinterpret_cast<const char*>(raw.data()), Config::PAGE_SIZE);
        file_.flush();

    }
    load_directory();

}

HeapFile::~HeapFile()
{
    if(file_.is_open())
    {
        flush_directory();
        file_.close();
    }
}

// Directory  IO

void HeapFile::load_directory()
{
    file_.seekg(page_offset(DIRECTORY_PAGE_ID));
    Page::RawData raw{};
    file_.read(reinterpret_cast<char*> (raw.data()), Config::PAGE_SIZE);

    std::memcpy(&dir_header_, raw.data() + Config::PAGE_HEADER_SIZE, sizeof(DirectoryHeader)); // skip header

    dir_entries_.resize(dir_header_.num_data_pages);
    const std::size_t entries_offset = Config::PAGE_HEADER_SIZE + sizeof(DirectoryEntry);
    std::memcpy(raw.data() + entries_offset, dir_entries_.data(),
                dir_entries_.size() * sizeof(DirectoryEntry));

    file_.seekg(page_offset(DIRECTORY_PAGE_ID));
    file_.write(reinterpret_cast<const char*>(raw.data()), Config::PAGE_SIZE);
    file_.flush();

}
void HeapFile::flush_directory()
{
    Page::RawData raw{};

    PageHeader header{};
    header.page_id = DIRECTORY_PAGE_ID;
    std::memcpy(raw.data(), &header, sizeof(header));

    std::memcpy(raw.data() + Config::PAGE_HEADER_SIZE, &dir_header_, sizeof(DirectoryHeader));

    const std::size_t entries_offset = Config::PAGE_HEADER_SIZE + sizeof(DirectoryHeader);
    std::memcpy(raw.data() + entries_offset, dir_entries_.data(),
                dir_entries_.size() * sizeof(DirectoryEntry));

    file_.seekp(page_offset(DIRECTORY_PAGE_ID));
    file_.write(reinterpret_cast<const char*>(raw.data()), Config::PAGE_SIZE);
    file_.flush();
}


// page IO
Page HeapFile::read_page(uint32_t page_id)
{
    file_.seekg(page_offset(page_id));
    Page::RawData raw{};
    if (!file_.read(reinterpret_cast<char*>(raw.data()), Config::PAGE_SIZE))
        throw std::runtime_error("HeapFile::read_page: failed to read page " +
                                 std::to_string(page_id));
    return Page::from_bytes(raw);
}

void HeapFile::write_page(const Page& page)
{
    file_.seekp(page_offset(page.page_id()));
    auto raw = page.to_bytes();
    file_.write(reinterpret_cast<const char*>(raw.data()), Config::PAGE_SIZE);
    file_.flush();
    if (!file_) throw std::runtime_error("HeapFile::write_page: write failed");
}

Page HeapFile::allocate_page()
{
    if (dir_header_.num_data_pages >= MAX_DIR_ENTRIES)
        throw std::runtime_error("HeapFile: directory full (extend to multi-page dir not yet implemented)");

    // Data pages start at page_id 1 (page 0 is directory)
    const uint32_t new_page_id = dir_header_.num_data_pages + 1;
    Page p(new_page_id);

    // Extend the file with a blank page
    write_page(p);

    // Register in directory
    DirectoryEntry entry;
    entry.page_id    = new_page_id;
    entry.free_space = p.free_space();
    dir_entries_.push_back(entry);
    ++dir_header_.num_data_pages;
    flush_directory();

    return p;
}


// record operations
RecordID HeapFile::insert_record(std::vector<const std::byte> data)
{
    if (data.empty())
        throw std::invalid_argument("HeapFile::insert_record: empty data");

    const std::size_t needed = data.size() + Config::SLOT_SIZE;

    // Find the first directory entry with enough space
    uint32_t target_page_id = INVALID_PAGE_ID;
    for (auto& entry : dir_entries_) {
        if (entry.free_space >= needed) {
            target_page_id = entry.page_id;
            break;
        }
    }

    Page page = (target_page_id == INVALID_PAGE_ID)
        ? allocate_page()
        : read_page(target_page_id);

    uint16_t slot_id = page.insert_record(data);

    // Update directory entry for this page
    for (auto& entry : dir_entries_) {
        if (entry.page_id == page.page_id()) {
            entry.free_space = page.free_space();
            break;
        }
    }
    flush_directory();
    write_page(page);

    return RecordID{page.page_id(), slot_id};
}
 std::vector<std::byte> HeapFile::get_record(const RecordID& rid)
{
    if (!rid.valid()) return {};
    Page page = read_page(rid.page_id);
    auto result = page.get_record(rid.slot_id);
    return result.value_or(std::vector<std::byte>{});
}

