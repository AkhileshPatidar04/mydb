
#include "storage/clock_replacer.h"

ClockReplacer::ClockReplacer(size_t num_frames):
            num_frames_(num_frames), slots_(num_frames){}
bool ClockReplacer::victim(frame_id_t* frame_id)
{
    if(evictable_count_ == 0 || num_frames_ == 0){
        return false;
    }

    for(size_t steps = 0; steps < 2 * num_frames_; steps++){
        SlotState& slot = slots_[clock_hand_];
        frame_id_t current = static_cast<frame_id_t>(clock_hand_);
        clock_hand_ = (clock_hand_ + 1) % num_frames_;

        if(!slot.in_clock){
            continue;   // not currently evictable, skip
        }

        if(slot.refernce_bit){
            // Give it a second chance, clear the bit, move on.
            slot.refernce_bit = false;
            continue;
        }

        // Reference bit already clear -- this is the victim.
        slot.in_clock = false;
        --evictable_count_;
        *frame_id = current;
        return true;
    }

    return false;
}
void ClockReplacer::pin(frame_id_t frame_id){
    if(static_cast<size_t>(frame_id) >= num_frames_){
        return ;
    }
    SlotState& slot = slots_[static_cast<size_t>(frame_id)];
    if(slot.in_clock){
        slot.in_clock = false;
        --evictable_count_;
    }
}
void ClockReplacer::unpin(frame_id_t frame_id){
    if( static_cast<size_t>(frame_id) >= num_frames_){
        return;
    }
    SlotState& slot = slots_[static_cast<frame_id_t>(frame_id)];
    if(slot.in_clock){
        slot.in_clock = true;
        slot.in_clock = true;
        ++evictable_count_;
    }
}

size_t ClockReplacer::size() const { return evictable_count_; }