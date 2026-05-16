#pragma once
#include <string>
#include <vector>
#include <variant>
#include <optional>

namespace spatialdb {
namespace net {

// Protocol limits to prevent abuse
static constexpr size_t MAX_BULK_STRING_SIZE = 512 * 1024 * 1024; // 512MB
static constexpr size_t MAX_ARRAY_SIZE       = 1024 * 1024;       // 1M elements
static constexpr size_t MAX_LINE_LENGTH      = 8192;              // 8KB

enum class RespType {
    SIMPLE_STRING,
    ERROR,
    INTEGER,
    BULK_STRING,
    ARRAY,
    NULL_BULK,
};

struct RespValue {
    RespType type;
    std::string str;
    int64_t     int_val = 0;
    std::vector<RespValue> arr_val;

    static RespValue simpleString(std::string s) {
        return {RespType::SIMPLE_STRING, std::move(s), 0, {}};
    }
    static RespValue error(std::string s) {
        return {RespType::ERROR, std::move(s), 0, {}};
    }
    static RespValue makeInteger(int64_t i) {
        return {RespType::INTEGER, "", i, {}};
    }
    static RespValue bulkString(std::string s) {
        return {RespType::BULK_STRING, std::move(s), 0, {}};
    }
    static RespValue nullBulk() {
        return {RespType::NULL_BULK, "", 0, {}};
    }
    static RespValue makeArray(std::vector<RespValue> arr) {
        return {RespType::ARRAY, "", 0, std::move(arr)};
    }
};

class RespEncoder {
public:
    static std::string encode(const RespValue& val);
    static std::string encodeArray(const std::vector<std::string>& strs);
    static std::string encodeError(const std::string& msg);
    static std::string encodeOK();
    static std::string encodeInteger(int64_t n);
    static std::string encodeNull();
};

class RespDecoder {
public:
    explicit RespDecoder(std::string buf);

    bool hasComplete() const;
    std::optional<RespValue> decode();

private:
    std::string buf_;
    size_t      pos_ = 0;

    std::optional<RespValue> decodeOne();
    std::optional<std::string> readLine();
    std::optional<RespValue> decodeBulkString(int64_t len);
    std::optional<RespValue> decodeArray(int64_t count);
};

} // namespace net
} // namespace spatialdb
