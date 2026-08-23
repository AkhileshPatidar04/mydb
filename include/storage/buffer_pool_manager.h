#pragma once

#include <cstddef>
#include <vector>
#include <list>
#include <memory>
#include <unordered_map>

#include "storage/page.h"
#include "storage/clock_replacer.h"
#include "storage/disk_manager.h"

class BufferPoolManager{
public:
    BufferPoolManager(size_t pool_size, DiskManager* disk_manager);
    ~BufferPoolManager();

    BufferPoolManager(const BufferPoolManager&) =delete;
    BufferPoolManager& operator=(const BufferPoolManager) = delete; 

    Page* fetchPage(page_id_t page_id);
    bool unpinPage(page_id_t page_id, bool is_dirty);
    bool flushPage(page_id_t page_id);
    Page* newPage(page_id_t* out_page_id);
    bool deletePage(page_id_t page_id);
    void flushAllPages();
    size_t getPoolSize() const;

private:

    bool findFreeFrame(frame_id_t* out_frame_id);
    std::size_t pool_size_;
    std::vector<Page> pages_;
    std::unordered_map<page_id_t, frame_id_t> page_table_;
    std::unique_ptr<ClockReplacer> replacer_;
    std::list<frame_id_t> free_list_;
    DiskManager* disk_manager_; // non-owning


};