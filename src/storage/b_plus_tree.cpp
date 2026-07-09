#include <cstring>
#include <optional>
#include "storage/b_plus_tree.h"

// layout page
//     [is_leaf: 1][num_keys: 2][next_leaf: 4B][body....]

// body:
//     Internal Node body: child[0], key[0], child[1], key[1], ..., child[n]
//         child i occupies Body + i*(4+4)
//         key i occupies Body + i*(4+4)+4

//     Leaf Node  body: 
//          [next_leaf:page_id_t(4 bytes)]
//          key[0], rid[0], key[1], rid[1], .... num_keys times

//         key i occupies : i*(4+4+2)  ->(key(4), page_id(4), slot_id(2));
//         key i occupies : Body + i*(4+4+2) + 4

constexpr size_t HEADER_SIZE = 1 + 2 + 4;
constexpr size_t INTERNAL_ENTRY_SIZE = sizeof(page_id_t) + sizeof(int32_t); // child + key
constexpr size_t LEAF_ENTRY_SIZE = sizeof(int32_t) + sizeof(page_id_t) + sizeof(uint16_t);


BPlusTree::NodeHeader BPlusTree::readNodeHeader(const Page* page)
{
    // 1(is_leaf) + 2(num_keys) + 4(next_page (in case of leaf node))
    NodeHeader header;
    const char* data = page->getData();
    header.is_leaf = (data[0] != 0);
    // uint16_t num_keys;
    std::memcpy(&header.num_keys, data + 1, sizeof(uint16_t));
    std::memcpy(&header.next_leaf, data + 3, sizeof(page_id_t));
    return header;
}

void BPlusTree::writeNodeHeader(Page* page, const NodeHeader& header)
{
    char* data = page->getData();
    data[0] = header.is_leaf ? 1 : 0;
    std::memcpy(data + 1, &header.num_keys, sizeof(uint16_t));
    std::memcpy(data + 3, &header.next_leaf, sizeof(page_id_t));
}

page_id_t BPlusTree::readChild(const Page*page, int index)
{
    page_id_t child;
    const char* data = page->getData();
    size_t offset = HEADER_SIZE + static_cast<size_t>(index) * INTERNAL_ENTRY_SIZE;
    std::memcpy(&child, data + offset, sizeof(page_id_t));
    return child;
}

void BPlusTree::writeChild(Page* page, int index, page_id_t child)
{
    char* data = page->getData();
    size_t offset = HEADER_SIZE + static_cast<size_t>(index)* INTERNAL_ENTRY_SIZE;
    std::memcpy(data + offset, &child, sizeof(page_id_t));
}

int32_t BPlusTree::readInternalKey(const Page* page, int index)
{
    int32_t key;
    const char* data = page->getData();
    size_t offset = HEADER_SIZE + static_cast<size_t>(index) * INTERNAL_ENTRY_SIZE + sizeof(page_id_t);
    std::memcpy(&key, data + offset, sizeof(int32_t));
    return key;
} 

void BPlusTree::writeInternalKey(Page* page, int index, int32_t key)
{
    char* data = page->getData();
    size_t offset = HEADER_SIZE + static_cast<size_t>(index) * INTERNAL_ENTRY_SIZE + sizeof(page_id_t);
    std::memcpy(data+offset, &key, sizeof(int32_t));
}

int32_t BPlusTree::readLeafKey(const Page* page, int index)
{
    int key;
    const char* data = page->getData();
    size_t offset = HEADER_SIZE + static_cast<size_t>(index) * LEAF_ENTRY_SIZE;
    std::memcpy(&key, data + offset, sizeof(int32_t));
    return key;
}

void BPlusTree::writeLeafKey(Page* page, int index, int32_t key)
{
    char* data= page->getData();
    size_t offset = HEADER_SIZE + static_cast<size_t>(index) * LEAF_ENTRY_SIZE;
    std::memcpy(data + offset, &key, sizeof(int32_t));
}

RecordID BPlusTree::readLeafRid(const Page* page, int index)
{
    RecordID rid;
    const char* data = page->getData();
    size_t offset = HEADER_SIZE + index * LEAF_ENTRY_SIZE + sizeof(int32_t);
    std::memcpy(&rid.page_id, data + offset, sizeof(page_id_t));
    std::memcpy(&rid.slot_id, data + offset + sizeof(page_id_t), sizeof(uint16_t));
    return rid;
}

void BPlusTree::writeLeafRid(Page* page, int index, const RecordID& rid)
{
    char* data = page->getData();
    int offset = HEADER_SIZE + index * LEAF_ENTRY_SIZE + sizeof(int32_t);
    std::memcpy(data + offset, &rid.page_id, sizeof(page_id_t));
    std::memcpy(data + offset + sizeof(page_id_t), &rid.slot_id, sizeof(uint16_t));
    return;
}

void BPlusTree::initializeLeafPage(Page* page, page_id_t next_leaf)
{
    NodeHeader header;
    header.is_leaf = true;
    header.num_keys = 0;
    header.next_leaf = next_leaf;
    writeNodeHeader(page, header);
}
void BPlusTree::initializeInternalPage(Page* page)
{
    NodeHeader header;
    header.is_leaf = false;
    header.num_keys = 0;
    header.next_leaf = INVALID_PAGE_ID;
    writeNodeHeader(page, header);
}

void BPlusTree::initializeNewTreeRootPage(Page* page){
    initializeLeafPage(page, INVALID_PAGE_ID);
}






/////////////////////   B Plus tree      /////////////////

BPlusTree::BPlusTree(BufferPoolManager* bpm, page_id_t root_page_id, int order)
                            :bpm_(bpm), root_page_id_(root_page_id), order_(order){}

page_id_t BPlusTree::getRootPageId(){ return root_page_id_; }

page_id_t BPlusTree::findLeaf(int32_t key){
    page_id_t current_page_id =  root_page_id_;
    while(true)
    {
        Page* page = bpm_->fetchPage(current_page_id);
        if(page == nullptr)
        {
            return INVALID_PAGE_ID;
        }
        NodeHeader header = readNodeHeader(page);
        if(header.is_leaf)
        {
            (void)bpm_->unpinPage(current_page_id, false);
            return current_page_id;
        }

        int child_index = header.num_keys; // initialize to last child
        // find strictly greater key and descend into child to its left
        for(int i = 0; i < header.num_keys; i++)
        {
            if(key < readInternalKey(page, i)){
                child_index = i;
                break;
            }
        }
        page_id_t child = readChild(page, child_index);
        (void)bpm_->unpinPage(current_page_id, false);
        current_page_id = child;
    }
}

std::optional<RecordID> BPlusTree::search(int32_t key)
{
    page_id_t leaf_id = findLeaf(key);
    if(leaf_id == INVALID_PAGE_ID)
    {
        return std::nullopt;
    }
    Page* leaf = bpm_->fetchPage(leaf_id);
    if(leaf == nullptr)
    {
        return std::nullopt;
    }

    NodeHeader header = readNodeHeader(leaf);
    int num_keys = header.num_keys;
    for(int i =0; i < num_keys; i++)
    {
        if(key == readLeafKey(leaf, i))
        {
            RecordID rid = readLeafRid(leaf, i);
            (void)bpm_->unpinPage(leaf_id, false);
            return rid;
        }
    }
    (void)bpm_->unpinPage(leaf_id, false);
    return std::nullopt;
} 


// bool BPlusTree::insert(int32_t key, const RecordID& rid)
// {
//     bool ok = true;
//     auto split = insertIntoNode(root_page_id_, key, rid, &ok);
//     if(!ok)
//     {
//         return false;
//     }

//     if(split.has_value())
//     {
//         // root itself split. 
//     }
// }
