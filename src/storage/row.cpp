#include "include/storage/row.h"
#include <cstring>
#include <stdexcept>
#include <limits>


// ---------------------------------------------------------------------------
// Little-endian helpers (C++20 bit_cast is cleaner than memcpy here)
// ---------------------------------------------------------------------------

void write_u8(std::vector<std::byte>& buf, uint8_t v)
{
    buf.push_back(static_cast<std::byte>(v));
}

void write_u16_le(std::vector<std::byte>& buf, uint16_t v)
{
    buf.push_back(static_cast<std::byte>(v & 0xFF));
    buf.push_back(static_cast<std::byte>((v >> 8) & 0xFF));
}

void write_i32_le(std::vector<std::byte>& buf, int32_t v)
{
    uint32_t u = static_cast<uint32_t>(v);
    for (int i = 0; i < 4; ++i)
        buf.push_back(static_cast<std::byte>((u >> (8*i)) & 0xFF));
}

void write_f32_le(std::vector<std::byte>& buf, float v)
{
    uint32_t bits;
    std::memcpy(&bits, &v, sizeof(bits));
    for (int i = 0; i < 4; ++i)
        buf.push_back(static_cast<std::byte>((bits >> (8*i)) & 0xFF));
}

uint8_t read_u8(const std::vector<std::byte>& src, std::size_t& pos)
{
    if (pos >= src.size()) throw std::runtime_error("deserialize_row: unexpected end");
    return static_cast<uint8_t>(src[pos++]);
}

uint16_t read_u16_le(const std::vector<std::byte>& src, std::size_t& pos)
{
    uint8_t lo = read_u8(src, pos);
    uint8_t hi = read_u8(src, pos);
    return static_cast<uint16_t>(lo | (hi << 8));
}

int32_t read_i32_le(const std::vector<std::byte>& src, std::size_t& pos)
{
    uint32_t u = 0;
    for (int i = 0; i < 4; ++i)
        u |= static_cast<uint32_t>(read_u8(src, pos)) << (8*i);
    return static_cast<int32_t>(u);
}

float read_f32_le(const std::vector<std::byte>& src, std::size_t& pos)
{
    uint32_t u = 0;
    for (int i = 0; i < 4; ++i)
        u |= static_cast<uint32_t>(read_u8(src, pos)) << (8*i);
    float value;
    std::memcpy(&value, &u, sizeof(value));
    return value;
}


// ---------------------------------------------------------------------------
// serialize_row
// ---------------------------------------------------------------------------
std::vector<std::byte> serialize_row(const Row& row, const std::optional<Schema>& schema)
{
    if (schema && schema->num_columns() != row.size())
        throw std::invalid_argument("serialize_row: row size doesn't match schema");

    std::vector<std::byte> buf;
    buf.reserve(row.size() * 5);  // rough estimate

    for (std::size_t i = 0; i < row.size(); ++i) {
        const Value& val = row[i];

        if (val.is_null()) {
            if (schema && !schema->columns[i].nullable)
                throw std::invalid_argument("serialize_row: NULL in non-nullable column");
            write_u8(buf, NULL_BYTE);
            continue;
        }

        if (val.is_int()) {
            write_u8(buf, static_cast<uint8_t>(ColType::INT32));
            write_i32_le(buf, val.as_int());
        } else if (val.is_float()) {
            write_u8(buf, static_cast<uint8_t>(ColType::FLOAT));
            write_f32_le(buf, val.as_float());
        } else if (val.is_string()) {
            const auto& s = val.as_string();
            if (s.size() > 65535)
                throw std::invalid_argument("serialize_row: VARCHAR too long (>65535)");
            write_u8(buf, static_cast<uint8_t>(ColType::VARCHAR));
            write_u16_le(buf, static_cast<uint16_t>(s.size()));
            for (char c : s)
                buf.push_back(static_cast<std::byte>(c));
        } else {
            throw std::runtime_error("serialize_row: unknown value type");
        }
    }
    return buf;
}

// ---------------------------------------------------------------------------
// deserialize_row
// ---------------------------------------------------------------------------
Row deserialize_row(const std::vector<std::byte>& data, const std::optional<Schema>& schema)
{
    Row row;
    std::size_t pos = 0;

    // When no schema, read until we exhaust data
    std::size_t expected = schema ? schema->num_columns() : std::numeric_limits<std::size_t>::max();

    while (pos < data.size()) {
        if (schema && row.size() >= expected) break;

        uint8_t tag = read_u8(data, pos);

        if (tag == NULL_BYTE) {
            row.fields.push_back(Value::make_null());
            continue;
        }

        switch (static_cast<ColType>(tag)) {
        case ColType::INT32:
            row.fields.push_back(Value::make_int(read_i32_le(data, pos)));
            break;
        case ColType::FLOAT:
            row.fields.push_back(Value::make_float(read_f32_le(data, pos)));
            break;
        case ColType::VARCHAR: {
            uint16_t len = read_u16_le(data, pos);
            if (pos + len > data.size())
                throw std::runtime_error("deserialize_row: varchar length overrun");
            std::string s;
            s.reserve(len);
            for (uint16_t j = 0; j < len; ++j)
                s.push_back(static_cast<char>(data[pos++]));
            row.fields.push_back(Value::make_string(std::move(s)));
            break;
        }
        default:
            throw std::runtime_error("deserialize_row: unknown type tag " + std::to_string(tag));
        }
    }

    if (schema && row.size() != schema->num_columns())
        throw std::runtime_error("deserialize_row: got fewer fields than schema columns");

    return row;
}

