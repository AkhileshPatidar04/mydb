#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <optional>
#include <unordered_map>
#include <memory>


#include "storage/page.h"
#include "storage/buffer_pool_manager.h"
#include "storage/heap_file.h"
#include "storage/b_plus_tree.h"


// ---- Schema ----
enum class ColumnType:uint8_t  {
    INTEGER =0,
    VARCHAR = 1,
    Float = 2
};
struct Column{
    std::string name;
    ColumnType type;
};
using Schema = std::vector<Column>;

// --- Table----  
// need to store on disk on catalog page
struct TableInfo {
    std::string name;
    Schema schema;
    page_id_t heap_root_page_id = INVALID_PAGE_ID;
    page_id_t btree_root_page_id = INVALID_PAGE_ID;
};


// it is only entry point to storage for all table access

// on-disk catalog page format
// [num_tables: 2 Byte]
// for each table:
//     [name_len: 2 Byte][name: name_len_bytes]
//     [heap_root_page_id: 4B][btree_root_page_id: 4B]
//     [num_columns:2 byte]
//         for each column:
//         [col_name_len:2B][col_name][col_type: 1 byte]


class Catalog {
    public:
    explicit Catalog(BufferPoolManager* bpm);

    bool createTable(const std::string& table_name, const Schema& schema, bool create_index = false);

    std::optional<Schema> getSchema(const std::string& table_name) const;

    HeapFile* getHeapFile(std::string& table_name);

    BPlusTree* GetIndex(std::string_view table_name);

    std::vector<std::string> getTableNames() const;

    private:

    bool persistCatalog(const std::unordered_map<std::string, TableInfo>& tables);

    void loadCatalog();

    void buildLiveObjects(const TableInfo& info);

    static constexpr page_id_t CATALOG_PAGE_ID = 0;

    BufferPoolManager* bpm_;
    std::unordered_map<std::string, TableInfo> tables_;
    std::unordered_map<std::string, std::unique_ptr<HeapFile>> heap_files_;
    std::unordered_map<std::string, std::unique_ptr<BPlusTree>> indexes_;
};