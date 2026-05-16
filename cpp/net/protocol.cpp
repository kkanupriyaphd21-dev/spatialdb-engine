#include "protocol.h"
#include <sstream>
#include <stdexcept>

namespace spatialdb {
namespace net {

std::string RespEncoder::encode(const RespValue& val) {
    std::ostringstream ss;
    switch (val.type) {
        case RespType::SIMPLE_STRING:
            ss << "+" << val.str << "\r\n";
            break;
        case RespType::ERROR:
            ss << "-" << val.str << "\r\n";
            break;
        case RespType::INTEGER:
            ss << ":" << val.int_val << "\r\n";
            break;
        case RespType::BULK_STRING:
            ss << "$" << val.str.size() << "\r\n" << val.str << "\r\n";
            break;
        case RespType::NULL_BULK:
            ss << "$-1\r\n";
            break;
        case RespType::ARRAY:
            ss << "*" << val.arr_val.size() << "\r\n";
            for (const auto& v : val.arr_val) ss << encode(v);
            break;
    }
    return ss.str();
}

std::string RespEncoder::encodeArray(const std::vector<std::string>& strs) {
    std::ostringstream ss;
    ss << "*" << strs.size() << "\r\n";
    for (const auto& s : strs) {
        ss << "$" << s.size() << "\r\n" << s << "\r\n";
    }
    return ss.str();
}

std::string RespEncoder::encodeError(const std::string& msg) {
    return "-ERR " + msg + "\r\n";
}

std::string RespEncoder::encodeOK() {
    return "+OK\r\n";
}

std::string RespEncoder::encodeInteger(int64_t n) {
    return ":" + std::to_string(n) + "\r\n";
}

std::string RespEncoder::encodeNull() {
    return "$-1\r\n";
}

// ─── Decoder ─────────────────────────────────────────────────────────────────

RespDecoder::RespDecoder(std::string buf) : buf_(std::move(buf)) {}

std::optional<std::string> RespDecoder::readLine() {
    auto pos = buf_.find("\r\n", pos_);
    if (pos == std::string::npos) return std::nullopt;
    auto line = buf_.substr(pos_, pos - pos_);
    pos_ = pos + 2;
    return line;
}

std::optional<RespValue> RespDecoder::decodeBulkString(int64_t len) {
    if (len < 0) return RespValue::nullBulk();
    if (pos_ + len + 2 > buf_.size()) return std::nullopt;
    auto s = buf_.substr(pos_, len);
    pos_ += len + 2;
    return RespValue::bulkString(std::move(s));
}

std::optional<RespValue> RespDecoder::decodeArray(int64_t count) {
    std::vector<RespValue> arr;
    arr.reserve(count);
    for (int64_t i = 0; i < count; ++i) {
        auto v = decodeOne();
        if (!v) return std::nullopt;
        arr.push_back(std::move(*v));
    }
    return RespValue::makeArray(std::move(arr));
}

std::optional<RespValue> RespDecoder::decodeOne() {
    if (pos_ >= buf_.size()) return std::nullopt;

    char prefix = buf_[pos_++];
    auto line   = readLine();
    if (!line) return std::nullopt;

    switch (prefix) {
        case '+': return RespValue::simpleString(*line);
        case '-': return RespValue::error(*line);
        case ':': return RespValue::makeInteger(std::stoll(*line));
        case '$': {
            int64_t len = std::stoll(*line);
            return decodeBulkString(len);
        }
        case '*': {
            int64_t count = std::stoll(*line);
            return decodeArray(count);
        }
        default:
            return std::nullopt;
    }
}

std::optional<RespValue> RespDecoder::decode() {
    return decodeOne();
}

bool RespDecoder::hasComplete() const {
    return buf_.find("\r\n") != std::string::npos;
}

} // namespace net
} // namespace spatialdb
