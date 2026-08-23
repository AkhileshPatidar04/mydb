#pragma once
/*
 * row.h — Row and Schema types with binary serialisation
 *
 * Wire format per field
 * ─────────────────────
 *  NULL         : 0xFF  (1 byte — NULL_BYTE sentinel)
 *  INT32        : 0x01  type-tag, then 4 bytes little-endian int32
 *  FLOAT        : 0x02  type-tag, then 4 bytes IEEE-754 float
 *  VARCHAR      : 0x03  type-tag, then 2-byte length, then UTF-8 bytes
 *
 * All integers are stored little-endian.
 */

#include "constants.h"
#include <cstdint>
#include <string>
#include <variant>
#include <vector>
#include <optional>
#include <stdexcept>

// ---------------------------------------------------------------------------
// Column types
// ---------------------------------------------------------------------------
enum class ColType : uint8_t {
    INT32   = 1,
    FLOAT   = 2,
    VARCHAR = 3,
};

struct ColumnDef {
    std::string name;
    ColType     type;
    bool        nullable {true};
};

struct Schema {
    std::vector<ColumnDef> columns;

    std::size_t num_columns() const { return columns.size(); }
};

// ---------------------------------------------------------------------------
// Value — one field value (null or typed)
// ---------------------------------------------------------------------------
using ValueVariant = std::variant<
    std::monostate,   // NULL
    int32_t,          // INT32
    float,            // FLOAT
    std::string       // VARCHAR
>;

struct Value {
    ValueVariant v;

    static Value make_null()             { return Value{std::monostate{}}; }
    static Value make_int(int32_t i)     { return Value{i}; }
    static Value make_float(float f)     { return Value{f}; }
    static Value make_string(std::string s) { return Value{std::move(s)}; }

    bool is_null()   const { return std::holds_alternative<std::monostate>(v); }
    bool is_int()    const { return std::holds_alternative<int32_t>(v); }
    bool is_float()  const { return std::holds_alternative<float>(v); }
    bool is_string() const { return std::holds_alternative<std::string>(v); }

    int32_t           as_int()    const { return std::get<int32_t>(v); }
    float             as_float()  const { return std::get<float>(v); }
    const std::string& as_string() const { return std::get<std::string>(v); }
};

// ---------------------------------------------------------------------------
// Row — a vector of Values
// ---------------------------------------------------------------------------
struct Row {
    std::vector<Value> fields;

    std::size_t size() const { return fields.size(); }
    const Value& operator[](std::size_t i) const { return fields[i]; }
    Value& operator[](std::size_t i)             { return fields[i]; }
};

// ---------------------------------------------------------------------------
// Serialisation helpers
// ---------------------------------------------------------------------------

// Encode a Row to a byte vector.
// The schema is used to validate types; pass nullopt to skip validation.
std::vector<std::byte> serialize_row(const Row& row,
                                     const std::optional<Schema>& schema = std::nullopt);

// Decode bytes back into a Row.
Row deserialize_row(const std::vector<std::byte>& data,
                    const std::optional<Schema>& schema = std::nullopt);
