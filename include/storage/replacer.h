#pragma once

#include <cstddef>

#include "storage/page.h"


// Replacer decides which in-memory frame to evict when the buffer pool
// needs to bring in a new page and has no free frames left. It only knows
// about frame_ids -- it has no idea what page (if any) lives in a frame,
// and no idea about dirty bits or disk I/O. BufferPoolManager is the glue
// that connects "this frame was chosen as victim" to "flush it if dirty,
// then reuse it."
class Replacer {
 public:
  virtual ~Replacer() = default;

  // Selects a frame to evict and writes its id into *frame_id. Returns
  // false if there is nothing currently evictable (every tracked frame is
  // pinned, or no frames have been registered at all).
  virtual bool victim(frame_id_t* frame_id) = 0;

  // Marks a frame as currently in use / pinned -- it must not be chosen as
  // a victim until a matching Unpin call makes it evictable again.
  virtual void pin(frame_id_t frame_id) = 0;

  // Marks a frame as evictable (its pin count dropped to zero).
  virtual void unpin(frame_id_t frame_id) = 0;

  // Number of frames currently tracked as evictable.
  virtual size_t size() const = 0;
};

