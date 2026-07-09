#pragma once

#include <cstddef>
#include <vector>
#include "storage/page.h"

class ClockReplacer{
public:
    explicit ClockReplacer(size_t num_frames);
    ~ClockReplacer() = default;
    
    // ClockReplacer(ClockReplacer&) = delete;
    // ClockReplacer& operator=(ClockReplacer&&) = delete;

    bool victim(frame_id_t* frame_id);
    void pin(frame_id_t frame_id);
    void  unpin(frame_id_t frame_id);
    size_t size() const;
    

private:
    struct SlotState{
        bool in_clock = false;
        bool refernce_bit = false;
    };
    size_t num_frames_;
    std::vector<SlotState> slots_;
    size_t clock_hand_ = 0;
    size_t evictable_count_ = 0;
};