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
    std::memcpy(&header.next_leaf_or_unused, data + 3, sizeof(page_id_t));
    return header;
}

void BPlusTree::writeNodeHeader(Page* page, const NodeHeader& header)
{
    char* data = page->getData();
    data[0] = header.is_leaf ? 1 : 0;
    std::memcpy(data + 1, &header.num_keys, sizeof(uint16_t));
    std::memcpy(data + 3, &header.next_leaf_or_unused, sizeof(page_id_t));
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
    header.next_leaf_or_unused = next_leaf;
    writeNodeHeader(page, header);
}
void BPlusTree::initializeInternalPage(Page* page)
{
    NodeHeader header;
    header.is_leaf = false;
    header.num_keys = 0;
    header.next_leaf_or_unused = INVALID_PAGE_ID;
    writeNodeHeader(page, header);
}

void BPlusTree::initializeNewTreeRootPage(Page* page){
    initializeLeafPage(page, INVALID_PAGE_ID);
}





//////////////////////////////////////////////////////////
/////////////////////     B Plus tree    /////////////////
//////////////////////////////////////////////////////////

BPlusTree::BPlusTree(BufferPoolManager* bpm, page_id_t root_page_id, int order)
                            :bpm_(bpm), root_page_id_(root_page_id), order_(order){}

page_id_t BPlusTree::getRootPageId() const { return root_page_id_; }

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

std::optional<RecordID> BPlusTree::Search(int32_t key)
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


bool BPlusTree::Insert(int32_t key, const RecordID& rid) {
  bool ok = true;
  auto split = insertIntoNode(root_page_id_, key, rid, &ok);
  if (!ok) {
    return false;
  }

  if (split.has_value()) {
    // The root itself split. Allocate a new internal root with two
    // children: the old root (now the left child) and the new sibling.
    page_id_t new_root_id;
    Page* new_root = bpm_->newPage(&new_root_id);
    if (new_root == nullptr) {
      return false;
    }
    initializeInternalPage(new_root);

    NodeHeader header = readNodeHeader(new_root);
    header.num_keys = 1;
    writeNodeHeader(new_root, header);
    writeChild(new_root, 0, root_page_id_);
    writeInternalKey(new_root, 0, split->median_key);
    writeChild(new_root, 1, split->new_right_page_id);

    if (!bpm_->unpinPage(new_root_id, /*is_dirty=*/true)) {
      return false;
    }

    root_page_id_ = new_root_id;
  }

  return true;
}

std::optional<BPlusTree::SplitResult> BPlusTree::insertIntoNode(
    page_id_t node_page_id, int32_t key, const RecordID& rid, bool* out_ok) {
  Page* page = bpm_->fetchPage(node_page_id);
  if (page == nullptr) {
    *out_ok = false;
    return std::nullopt;
  }

  NodeHeader header = readNodeHeader(page);

  if (header.is_leaf) {
    // Find insertion position (keep keys sorted).
    int pos = header.num_keys;
    for (int i = 0; i < header.num_keys; ++i) {
      if (key < readLeafKey(page, i)) {
        pos = i;
        break;
      }
      if (key == readLeafKey(page, i)) {
        // Duplicate key -- this simple index does not support duplicates.
        // Treat as failure rather than silently overwriting.
        (void)bpm_->unpinPage(node_page_id, false);
        *out_ok = false;
        return std::nullopt;
      }
    }

    // Shift entries at and after pos one slot to the right.
    for (int i = header.num_keys; i > pos; --i) {
      writeLeafKey(page, i, readLeafKey(page, i - 1));
      writeLeafRid(page, i, readLeafRid(page, i - 1));
    }
    writeLeafKey(page, pos, key);
    writeLeafRid(page, pos, rid);
    header.num_keys = static_cast<uint16_t>(header.num_keys + 1);

    if (header.num_keys <= order_) {
      // Fits, no split needed.
      writeNodeHeader(page, header);
      if (!bpm_->unpinPage(node_page_id, /*is_dirty=*/true)) {
        *out_ok = false;
        return std::nullopt;
      }
      *out_ok = true;
      return std::nullopt;
    }

    // Overflow -- split this leaf into two. Left keeps the first half,
    // right gets the second half. The right sibling's first key becomes
    // the median pushed up to the parent.
    int total = header.num_keys;
    int left_count = (total + 1) / 2;
    int right_count = total - left_count;

    page_id_t new_right_id;
    Page* new_right = bpm_->newPage(&new_right_id);
    if (new_right == nullptr) {
      *out_ok = false;
      return std::nullopt;
    }
    initializeLeafPage(new_right, header.next_leaf_or_unused);

    for (int i = 0; i < right_count; ++i) {
      writeLeafKey(new_right, i, readLeafKey(page, left_count + i));
      writeLeafRid(new_right, i, readLeafRid(page, left_count + i));
    }
    NodeHeader right_header;
    right_header.is_leaf = true;
    right_header.num_keys = static_cast<uint16_t>(right_count);
    right_header.next_leaf_or_unused = header.next_leaf_or_unused;
    writeNodeHeader(new_right, right_header);

    int32_t median_key = readLeafKey(new_right, 0);

    header.num_keys = static_cast<uint16_t>(left_count);
    header.next_leaf_or_unused = new_right_id;  // relink leaf chain
    writeNodeHeader(page, header);

    if (!bpm_->unpinPage(new_right_id, /*is_dirty=*/true)) {
      *out_ok = false;
      return std::nullopt;
    }
    if (!bpm_->unpinPage(node_page_id, /*is_dirty=*/true)) {
      *out_ok = false;
      return std::nullopt;
    }

    *out_ok = true;
    return SplitResult{median_key, new_right_id};
  }

  // Internal node: find which child to descend into.
  int child_index = header.num_keys;
  for (int i = 0; i < header.num_keys; ++i) {
    if (key < readInternalKey(page, i)) {
      child_index = i;
      break;
    }
  }
  page_id_t child_id = readChild(page, child_index);

  if (!bpm_->unpinPage(node_page_id, /*is_dirty=*/false)) {
    *out_ok = false;
    return std::nullopt;
  }

  auto child_split = insertIntoNode(child_id, key, rid, out_ok);
  if (!*out_ok) {
    return std::nullopt;
  }
  if (!child_split.has_value()) {
    return std::nullopt;  // child absorbed the insert without splitting
  }

  // Child split -- absorb (median_key, new_right_page_id) into this node
  // at child_index.
  page = bpm_->fetchPage(node_page_id);
  if (page == nullptr) {
    *out_ok = false;
    return std::nullopt;
  }
  header = readNodeHeader(page);

  // Shift children/keys right to make room at child_index.
  for (int i = header.num_keys; i > child_index; --i) {
    writeInternalKey(page, i, readInternalKey(page, i - 1));
    writeChild(page, i + 1, readChild(page, i));
  }
  writeInternalKey(page, child_index, child_split->median_key);
  writeChild(page, child_index + 1, child_split->new_right_page_id);
  header.num_keys = static_cast<uint16_t>(header.num_keys + 1);

  if (header.num_keys <= order_) {
    writeNodeHeader(page, header);
    if (!bpm_->unpinPage(node_page_id, /*is_dirty=*/true)) {
      *out_ok = false;
      return std::nullopt;
    }
    *out_ok = true;
    return std::nullopt;
  }

  // This internal node overflowed too -- split it.
  int total = header.num_keys;
  int left_count = total / 2;            // keys kept on the left
  int32_t median_key = readInternalKey(page, left_count);
  int right_count = total - left_count - 1;  // median key itself is promoted, not copied

  page_id_t new_right_id;
  Page* new_right = bpm_->newPage(&new_right_id);
  if (new_right == nullptr) {
    *out_ok = false;
    return std::nullopt;
  }
  initializeInternalPage(new_right);

  // Right node gets children [left_count+1 .. total] and keys
  // [left_count+1 .. total-1] (i.e. everything after the promoted median).
  writeChild(new_right, 0, readChild(page, left_count + 1));
  for (int i = 0; i < right_count; ++i) {
    writeInternalKey(new_right, i, readInternalKey(page, left_count + 1 + i));
    writeChild(new_right, i + 1, readChild(page, left_count + 2 + i));
  }
  NodeHeader right_header;
  right_header.is_leaf = false;
  right_header.num_keys = static_cast<uint16_t>(right_count);
  right_header.next_leaf_or_unused = INVALID_PAGE_ID;
  writeNodeHeader(new_right, right_header);

  header.num_keys = static_cast<uint16_t>(left_count);
  writeNodeHeader(page, header);

  if (!bpm_->unpinPage(new_right_id, /*is_dirty=*/true)) {
    *out_ok = false;
    return std::nullopt;
  }
  if (!bpm_->unpinPage(node_page_id, /*is_dirty=*/true)) {
    *out_ok = false;
    return std::nullopt;
  }

  *out_ok = true;
  return SplitResult{median_key, new_right_id};
}

// ---- Delete ----
//
// Per project rule: on underflow, do nothing (no merge/redistribute).
// A tombstone-the-key style delete -- remove the key/rid from its leaf,
// shifting later entries left to close the gap. Leaving an underfull leaf
// is correct (Search/RangeScan still work), just not space-optimal.

bool BPlusTree::Delete(int32_t key) {
  page_id_t leaf_id = findLeaf(key);
  if (leaf_id == INVALID_PAGE_ID) {
    return false;
  }

  Page* leaf = bpm_->fetchPage(leaf_id);
  if (leaf == nullptr) {
    return false;
  }

  NodeHeader header = readNodeHeader(leaf);
  int found_index = -1;
  for (int i = 0; i < header.num_keys; ++i) {
    if (readLeafKey(leaf, i) == key) {
      found_index = i;
      break;
    }
  }

  if (found_index == -1) {
    (void)bpm_->unpinPage(leaf_id, false);
    return false;
  }

  for (int i = found_index; i < header.num_keys - 1; ++i) {
    writeLeafKey(leaf, i, readLeafKey(leaf, i + 1));
    writeLeafRid(leaf, i, readLeafRid(leaf, i + 1));
  }
  header.num_keys = static_cast<uint16_t>(header.num_keys - 1);
  writeNodeHeader(leaf, header);

  if (!bpm_->unpinPage(leaf_id, /*is_dirty=*/true)) {
    return false;
  }
  return true;
}
