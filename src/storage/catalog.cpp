#include "storage/catalog.h"

#include <cstring>
#include <cstdint>

// binary Serialization/desiralization

bool writeU16(char* buf, size_t& offset, uint16_t val)
{
    if(offset + sizeof(int32_t) > PAGE_SIZE) return false;
    std::memcpy(buf + offset, &val, sizeof(uint16_t));
    offset += sizeof(uint16_t);
    return true;
}

bool writeI32(char* buf, size_t& offset, int32_t val)
{
    if(offset + sizeof(int32_t) > PAGE_SIZE) return false;
    std::memcpy(buf + offset, &val, sizeof(int32_t));
    offset += sizeof(int32_t);
    return true;
}

bool writeU8(char* buf, size_t& offset, uint8_t val)
{
    if(offset + sizeof(uint8_t) > PAGE_SIZE) return false;
    std::memcpy(buf + offset, &val, sizeof(uint8_t));
    offset += sizeof(uint8_t);
    return true;
}

bool writeString(char* buf, size_t& offset, const std::string& s)
{
    if(s.size() >= (1 << sizeof(uint16_t))) return false;  // exceed len of  cannot be stored in uint16_t (CASE of overflow due to int overflow)
    uint16_t len = s.size();
    
    if(offset + sizeof(uint16_t) + len > PAGE_SIZE) return false;
    if(!writeU16(buf, offset, len)) return false;

    std::memcpy(buf + offset, s.data(), len);
    offset += len;
    return true;
}

bool readU16(const char* buf, size_t& offset, uint16_t& out)
{
    if(offset + sizeof(int32_t) > PAGE_SIZE) return false;
    std::memcpy(&out, buf + offset, sizeof(uint16_t));
    offset += sizeof(uint16_t);
    return true; 
}

bool readI32(const char* buf, size_t& offset, int32_t& out) {
  if (offset + sizeof(int32_t) > PAGE_SIZE) return false;
  std::memcpy(&out, buf + offset, sizeof(int32_t));
  offset += sizeof(int32_t);
  return true;
}

bool readU8(const char* buf, size_t& offset, uint8_t& out) {
  if (offset + 1 > PAGE_SIZE) return false;
  out = static_cast<uint8_t>(buf[offset]);
  offset += 1;
  return true;
}

bool readString(const char* buf, size_t& offset, std::string& out) {
  uint16_t len;
  if (!readU16(buf, offset, len)) return false;
  if (offset + len > PAGE_SIZE) return false;
  out.assign(buf + offset, len);
  offset += len;
  return true;
}










////// ////////    CATALOG    //////////////


Catalog::Catalog(BufferPoolManager* bpm) : bpm_(bpm) { loadCatalog(); }

void Catalog::loadCatalog(){
    
    Page* page = bpm_->fetchPage(CATALOG_PAGE_ID);
    if(page == nullptr){
        return ;
    }

    const char* buf = page->getData();
    size_t offset = 0;
    uint16_t num_tables;
    if(!readU16(buf, offset, num_tables) || num_tables == 0)
    {
        (void) bpm_->unpinPage(CATALOG_PAGE_ID, false);
        (void) bpm_->deletePage(CATALOG_PAGE_ID);

        page_id_t allocated_id;
        Page* fresh = bpm_->newPage(&allocated_id);
        if(fresh != nullptr)
        {
            char* wbuf = fresh->getData();
            std::memset(wbuf, 0, PAGE_SIZE);
            uint16_t zero = 0;
            std::memcpy(wbuf, &zero, sizeof(uint16_t));
            (void)bpm_->unpinPage(allocated_id, true);
            (void)bpm_->flushPage(allocated_id);
        }
        return;
    }

    for(uint16_t t =0; t < num_tables; ++t)
    {
        TableInfo info;
        if(!readString(buf, offset, info.name)) break;

        int32_t heap_root;
        if(!readI32(buf, offset, heap_root)) break;
        info.heap_root_page_id = heap_root;

        int32_t btree_root;
        if(!readI32(buf, offset, btree_root)) break;
        info.btree_root_page_id = btree_root;

        uint16_t num_cols;
        if(!readU16(buf, offset, num_cols)) break;

        bool col_ok = true;
        for(uint16_t c = 0; c < num_cols; c++){
            Column col;
            if(!readString(buf, offset, col.name)){ col_ok = false; break; }
            uint8_t type_byte;
            if(!readU8(buf, offset, type_byte)){ col_ok = false; break; }
            col.type = static_cast<ColumnType> (type_byte);
            info.schema.push_back(col);
        }
        if(!col_ok) break;

        tables_[info.name] = info;
        buildLiveObjects(info);
    }
    (void) bpm_->unpinPage(CATALOG_PAGE_ID, false);
}

void Catalog::buildLiveObjects(const TableInfo& info)
{
    heap_files_[info.name] =
        std::make_unique<HeapFile>(bpm_, info.heap_root_page_id);
    // bplus only when index for this table is created
    if(info.btree_root_page_id != INVALID_PAGE_ID)
    {
        indexes_[info.name] = 
            std::make_unique<BPlusTree>(bpm_, info.btree_root_page_id, 16);
    }
}









