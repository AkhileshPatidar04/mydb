#include "include/storage/buffer_pool.h"

BufferPool::BufferPool(HeapFile& heap_file, std::size_t num_frames = BUFFER_POOL_SIZE)
                                :heap_file_(heap_file), frames_(num_frames)
{}

//clock eviction

std::size_t BufferPool::evict_frame()
{
    const std::size_t N = frames_.size();
    // At most two full sweeps: first clears ref bits, second evicts
    for (std::size_t sweep = 0; sweep < 2 * N; ++sweep) {
        Frame& f = frames_[clock_hand_];
        const std::size_t candidate = clock_hand_;
        clock_hand_ = (clock_hand_ + 1) % N;

        if (f.is_empty())   return candidate;  // free frame
        if (f.is_pinned())  continue;          // can't evict

        if (f.ref_bit) {
            f.ref_bit = false;                 // give a second chance
            continue;
        }

        // Evict this frame
        if (f.dirty) {
            heap_file_.write_page(*f.page);
            f.dirty = false;
        }
        page_table_.erase(f.page->page_id());
        f.page.reset();
        f.pin_count = 0;
        return candidate;
    }
    throw std::runtime_error("BufferPool: all frames are pinned, cannot evict");
}


Page& BufferPool::fetch_page(uint32_t page_id)
{
    // Already in pool?
    if (auto it = page_table_.find(page_id); it != page_table_.end()) {
        Frame& f = frames_[it->second];
        ++f.pin_count;
        f.ref_bit = true;
        return *f.page;
    }

    // Need to load from disk
    std::size_t frame_idx = evict_frame();
    Frame& f = frames_[frame_idx];

    f.page      = heap_file_.read_page(page_id);
    f.pin_count = 1;
    f.ref_bit   = true;
    f.dirty     = false;

    page_table_[page_id] = frame_idx;
    return *f.page;
}


void BufferPool::unpin_page(uint32_t page_id, bool is_dirty)
{
    auto it = page_table_.find(page_id);
    if (it == page_table_.end())
        throw std::runtime_error("BufferPool::unpin_page: page not in pool");

    Frame& f = frames_[it->second];
    if (f.pin_count <= 0)
        throw std::runtime_error("BufferPool::unpin_page: pin_count already 0");

    --f.pin_count;
    if (is_dirty) f.dirty = true;
}


void BufferPool::flush_page(uint32_t page_id)
{
    auto it = page_table_.find(page_id);
    if (it == page_table_.end()) return;   // not in pool — nothing to flush

    Frame& f = frames_[it->second];
    if (f.dirty && f.page.has_value()) {
        heap_file_.write_page(*f.page);
        f.dirty = false;
    }
}

void BufferPool::flush_all()
{
    for (auto& f : frames_) {
        if (!f.is_empty() && f.dirty) {
            heap_file_.write_page(*f.page);
            f.dirty = false;
        }
    }
}

Page& BufferPool::new_page(uint32_t& page_id_out)
{
    // Allocate via HeapFile (extends the file, writes blank page)
    Page blank = heap_file_.allocate_page();
    page_id_out = blank.page_id();

    std::size_t frame_idx = evict_frame();
    Frame& f = frames_[frame_idx];

    f.page      = std::move(blank);
    f.pin_count = 1;
    f.ref_bit   = true;
    f.dirty     = false;      // already written to disk by allocate_page()

    page_table_[page_id_out] = frame_idx;
    return *f.page;
}