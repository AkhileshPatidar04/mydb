#pragma once 

#include <optional>
#include <cstdint>
#include <vector>

#include "storage/page.h"
#include "storage/heap_file.h"
#include "storage/buffer_pool_manager.h"

class BPlusTree{
    public:
    BPlusTree(BufferPoolManager* bpm, page_id_t root_page_id, int order = 4);

    bool Insert(int32_t key, const RecordID& rid);
    std::optional<RecordID> Search(int32_t key);
    bool Delete(int32_t key);

    page_id_t getRootPageId() const;
    static void initializeNewTreeRootPage(Page* page);

    private:
    struct NodeHeader{
        bool is_leaf;
        uint16_t num_keys;
        page_id_t next_leaf; // use of it only in leaf node else it is unused;
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

    BufferPoolManager* bpm_;
    page_id_t root_page_id_;
    int order_;
};