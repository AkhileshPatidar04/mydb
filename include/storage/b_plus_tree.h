#pragma once 

#include <optional>
#include <cstdint>
#include <vector>

#include "storage/page.h"
#include "storage/heap_file.h"
#include "storage/buffer_pool_manager.h"

// templated) for this phase -- see project rule against premature
// generality. Every node lives in exactly one Page; BPlusTree reaches disk
// EXCLUSIVELY through BufferPoolManager, never DiskManager directly.
//
// On-page node layout (fits in one 4096-byte Page):
//
//   [ is_leaf: 1 byte ]
//   [ num_keys: 2 bytes ]
//   internal node:
//     [ next_leaf / unused: 4 bytes -- not used by internal nodes, kept
//       for layout uniformity so ReadNodeHeader doesn't need to branch ]
//     [ children[0]: page_id_t (4B) ]
//     [ key[0]: int32_t (4B) ] [ children[1]: page_id_t (4B) ]
//     [ key[1]: int32_t (4B) ] [ children[2]: page_id_t (4B) ]
//     ... (num_keys keys, num_keys+1 children)
//   leaf node:
//     [ next_leaf: page_id_t (4 bytes) ]
//     [ key[0]: int32_t (4B) ] [ rid[0]: page_id_t(4B)+slot_num(2B) ]
//     [ key[1]: int32_t (4B) ] [ rid[1]: ... ]
//     ... (num_keys keys, num_keys RecordIDs)
//
// `order` is the maximum number of KEYS a node may hold before it must
// split (so an internal node may have up to order+1 children).

class BPlusTree{
    public:
    BPlusTree(BufferPoolManager* bpm, page_id_t root_page_id, int order = 4);

    bool Insert(int32_t key, const RecordID& rid);
    std::optional<RecordID> Search(int32_t key);
    bool Delete(int32_t key);

    page_id_t getRootPageId() const;
    static void initializeNewTreeRootPage(Page* page);

    class RangeIterator {
    public:
        RangeIterator(BPlusTree* tree, page_id_t leaf_page_id, int32_t start_key,
                    int32_t end_key);

        bool hasNext() const;
        std::pair<int32_t, RecordID> next();

    private:
        void advanceToNextValid();

        BPlusTree* tree_;
        page_id_t current_page_id_;
        int current_index_;
        int32_t end_key_;
        bool has_cached_next_ = false;
    };

    RangeIterator rangeScan(int32_t start_key, int32_t end_key);

    private:
    struct NodeHeader{
        bool is_leaf;
        uint16_t num_keys;
        page_id_t next_leaf_or_unused; // use of it only in leaf node else it is unused;
    };
    static NodeHeader readNodeHeader(const Page* page);
    static void writeNodeHeader(Page* page, const NodeHeader& header);

    // internal-node :
    static page_id_t readChild(const Page* page, int index);
    static void writeChild(Page* page, int index, page_id_t child);
    static int32_t readInternalKey(const Page* page, int index);
    static void writeInternalKey(Page* page, int index, int32_t key);

    // leaf node:
    static int32_t readLeafKey(const Page* page, int index);
    static void writeLeafKey(Page* page, int index, int32_t key);
    static RecordID readLeafRid(const Page* page, int index);
    static void writeLeafRid(Page* page, int index, const RecordID& rid);

    static void initializeLeafPage(Page* page, page_id_t next_leaf);
    static void initializeInternalPage(Page* page);

    struct SplitResult
    {
        int32_t median_key;
        page_id_t new_right_page_id;
    };
    std::optional<SplitResult> insertIntoNode(page_id_t node_page_id, int32_t key, const RecordID& rid, bool* out_ok);
    // it only find page id, nothing about path or parent info
    // insert/delete use there own 
    page_id_t findLeaf(int32_t key);

    size_t OFFSET_IS_LEAF() const { return 0; }
    size_t OFFSET_NUM_KEYS() const { return 1; }
    size_t OFFSET_NEXT_LEAF() const { return 3; }
    size_t OFFSET_BODY() const { return 7; }  // where children/keys start


    BufferPoolManager* bpm_;
    page_id_t root_page_id_;
    int order_;
};