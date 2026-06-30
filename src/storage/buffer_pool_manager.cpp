#pragma once

#include "include/storage/buffer_pool_manager.h"

BufferPoolManager::BufferPoolManager(size_t pool_size, DiskManager* disk_manager)
        :   pool_size_(pool_size),
            pages_(pool_size),
            replacer_(std::make_unique<ClockReplacer>(pool_size)),
            disk_manager_(disk_manager){
                // every frame starts out free
    for(size_t i = 0; i < pool_size_; i++){
        free_list_.push_back(static_cast<frame_id_t>(i));
    }
}

BufferPoolManager::~BufferPoolManager() { flushAllPages(); }

bool BufferPoolManager::findFreeFrame(frame_id_t* out_frame_id){
    if(!free_list_.empty())
    {
        *out_frame_id = free_list_.front();
        free_list_.pop_front();
        return true;
    }
    frame_id_t victim_frame;
    if(!replacer_->victim(&victim_frame)){
        return false;   // every frame pinned, nothing evictable
    }
    
    Page& victim_page = pages_[static_cast<size_t>(victim_frame)];
    if(victim_page.isDirty()){
        disk_manager_->writePage(victim_page.getPageId(), victim_page.getData());
    }

    if (victim_page.getPageId() != INVALID_PAGE_ID){
        page_table_.erase(victim_page.getPageId());
    }

    *out_frame_id = victim_frame;
    return true;
}

Page*  BufferPoolManager::fetchPage(page_id_t page_id){
    auto it = page_table_.find(page_id);
    if(it != page_table_.end()){
        frame_id_t frame_id = it->second;
        Page& page = pages_[static_cast<size_t>(frame_id)];
        if(page.pin_count_ == 0){
            // Was evictable until now; it's about to be used again, so remove
            // it from victim candidacy.
            replacer_->pin(frame_id);
        }
        ++page.pin_count_;
        return &page;
    }

    frame_id_t frame_id;
    if(!findFreeFrame(&frame_id)){
        return nullptr;
    }

    Page& page = pages_[static_cast<size_t>(frame_id)];
    page.resetMemory();
    page.page_id_ = page_id;
    page.is_dirty_ = false;
    page.pin_count_ = 1;
    if(!disk_manager_-> readPage(page_id, page.getData()))
    {
        // Genuine I/O failure -- undo the frame assignment so it doesn't leak
        // as a permanently broken entry, and return failure to the caller.
        page.page_id_ = INVALID_PAGE_ID;
        page.is_dirty_ = false;
        page.pin_count_ = 0;
        free_list_.push_back(frame_id);
        return nullptr;
    }

    page_table_[page_id] = frame_id;
    return &page;
}

bool BufferPoolManager::unpinPage(page_id_t page_id, bool is_dirty)
{
    auto it = page_table_.find(page_id);
    if(it == page_table_.end()){
        return false;
    }

    frame_id_t frame_id = it->second;
    Page& page = pages_[static_cast<size_t>(frame_id)];

    if(page.pin_count_ <= 0){
        return false;  // unpin without a matching fetch -- caller bug
    }
    
    if(is_dirty)
    {
        page.is_dirty_ =true;   // sticky: stays dirty until actually flushed
    }
    --page.pin_count_;
    if(page.pin_count_ == 0)
    {
        replacer_->unpin(frame_id);
    }
    return true;
}


Page* BufferPoolManager::newPage(page_id_t* out_page_id) {
  frame_id_t frame_id;
  if (!findFreeFrame(&frame_id)) {
    return nullptr;
  }

  page_id_t new_page_id = disk_manager_->allocatePage();

  Page& page = pages_[static_cast<size_t>(frame_id)];
  page.resetMemory();
  page.page_id_ = new_page_id;
  page.is_dirty_ = false;
  page.pin_count_ = 1;

  page_table_[new_page_id] = frame_id;
  *out_page_id = new_page_id;
  return &page;
}

bool BufferPoolManager::deletePage(page_id_t page_id) {
  auto it = page_table_.find(page_id);
  if (it == page_table_.end()) {
    return true;  // not resident -- nothing to do, not an error
  }

  frame_id_t frame_id = it->second;
  Page& page = pages_[static_cast<size_t>(frame_id)];
  if (page.pin_count_ > 0) {
    return false;  // still in use, cannot evict
  }

  replacer_->pin(frame_id);  // ensure it's not sitting in the replacer's candidate set
  page_table_.erase(it);
  page.resetMemory();
  page.page_id_ = INVALID_PAGE_ID;
  page.is_dirty_ = false;
  free_list_.push_back(frame_id);
  return true;
}

void BufferPoolManager::flushAllPages() {
  for (auto& [page_id, frame_id] : page_table_) {
    Page& page = pages_[static_cast<size_t>(frame_id)];
    disk_manager_->writePage(page.getPageId(), page.getData());
    page.is_dirty_ = false;
  }
}

size_t BufferPoolManager::getPoolSize() const { return pool_size_; }

