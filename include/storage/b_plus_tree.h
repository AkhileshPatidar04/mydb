#pragma once 

#include "storage/page.h"
#include "storage/heap_file.h"
#include "storage/buffer_pool_manager.h"

class BPlusTree{
    public:
    BPlusTree(BufferPoolManager* bpm, page_id_t root_page_id, int order = 4);

    page_id_t getRootPageId();
    static void initializeNewTreeRootPage(Page* page);

    std::optional<RecordID> search(int32_t key);
    bool insert(int32_t key, const RecordID& rid);

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

    // it only find page id, nothing about path or parent info
    // insert/delete use there own 
    page_id_t findLeaf(int32_t key);

    BufferPoolManager* bpm_;
    page_id_t root_page_id_;
    int order_;
};