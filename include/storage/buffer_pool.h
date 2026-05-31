#pragma once
//Buffer Pool Manager
// uses clock eviction 
// disk read write go thorugh buffer pool manager



// frame lifecycle
//-> EMPTY ->fetch page(id) -> Pinned(pin_count>0 , ref_bit =true)
//->PINNED ->unpin_page(id) -> UNPINNED(pin_count ==0, eligible for eviction)
//->UNPINNED->clock sweep -> EMPTY(dirty pages are flushed first)


//Clock replacement
//  each frame has a refence bit. clock hand sweep frames:
//         if pinned: skip
//         if ref_bit == true: clear it, advance
//         if ref_bit == false: evict this frame



#include "include/storage/heap_file.h"
#include "include/storage/page.h"
#include <unordered_map>


struct Frame{
    std::optional<Page> page {};
    int  pin_count {0}; // no of threads holding page
    bool ref_bit{false}; // clock refernce bit
    bool dirty{false}; // needs to write back

    bool is_empty() const {return !page.has_value(); }
    bool is_pinned() const {return pin_count > 0; }
};

class BufferPool
{
    HeapFile& heap_file_;
    std::vector<Frame> frames_;
    std::size_t clock_hand_{0};
    // page -> frame index
    std::unordered_map<uint32_t, std::size_t>page_table_;
    
    // return frame index to evict, or throws if impossible
    std::size_t evict_frame();

public:
    explicit BufferPool(HeapFile& heap_file, std::size_t num_frames = BUFFER_POOL_SIZE);

    Page& fetch_page(uint32_t page_id);

    void unpin_page(uint32_t page_id, bool is_dirty = false);

    void flush_page(uint32_t page_id);

    void flush_all();

    Page& new_page(uint32_t& page_id_out);

    std::size_t num_frames() const{return frames_.size();}
};