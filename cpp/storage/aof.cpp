#include "aof.h"
#include <sstream>
#include <stdexcept>
#include <sys/stat.h>
#include <chrono>

namespace spatialdb {
namespace storage {

AOFWriter::AOFWriter(const std::string& path, AOFSync sync)
    : path_(path), sync_(sync)
{
    file_.open(path_, std::ios::app);
    if (!file_.is_open())
        throw std::runtime_error("AOF: cannot open " + path_);
}

AOFWriter::~AOFWriter() {
    if (file_.is_open()) {
        file_.flush();
        file_.close();
    }
}

std::string AOFWriter::serialize(const AOFRecord& r) const {
    std::ostringstream ss;
    // RESP3-ish format: *N\r\n$cmd\r\ncmd\r\n...
    ss << "*" << (1 + r.args.size()) << "\r\n";
    ss << "$" << r.command.size() << "\r\n" << r.command << "\r\n";
    for (const auto& arg : r.args) {
        ss << "$" << arg.size() << "\r\n" << arg << "\r\n";
    }
    return ss.str();
}

bool AOFWriter::write(const AOFRecord& record) {
    std::lock_guard<std::mutex> lock(mu_);
    auto s = serialize(record);
    file_ << s;
    if (sync_ == AOFSync::ALWAYS) file_.flush();
    return file_.good();
}

bool AOFWriter::fsync() {
    std::lock_guard<std::mutex> lock(mu_);
    file_.flush();
    return file_.good();
}

bool AOFWriter::rewrite(const std::vector<AOFRecord>& records) {
    std::lock_guard<std::mutex> lock(mu_);
    file_.close();
    file_.open(path_, std::ios::trunc);
    if (!file_.is_open()) return false;
    for (const auto& r : records) {
        file_ << serialize(r);
    }
    file_.flush();
    return file_.good();
}

size_t AOFWriter::fileSize() const {
    struct stat st;
    if (stat(path_.c_str(), &st) != 0) return 0;
    return (size_t)st.st_size;
}

// ─── AOFReader ────────────────────────────────────────────────────────────────

AOFReader::AOFReader(const std::string& path) : path_(path) {}

bool AOFReader::parseRecord(std::ifstream& in, AOFRecord& out) {
    std::string line;
    if (!std::getline(in, line)) return false;
    if (line.empty() || line[0] != '*') return false;
    if (!line.empty() && line.back() == '\r') line.pop_back();

    int count = std::stoi(line.substr(1));
    if (count < 1) return false;

    auto readBulk = [&](std::string& val) -> bool {
        std::string header;
        if (!std::getline(in, header)) return false;
        if (!header.empty() && header.back() == '\r') header.pop_back();
        if (header.empty() || header[0] != '$') return false;
        size_t len = std::stoul(header.substr(1));
        val.resize(len);
        if (!in.read(val.data(), len)) return false;
        in.ignore(2); // \r\n
        return true;
    };

    if (!readBulk(out.command)) return false;
    out.args.resize(count - 1);
    for (auto& arg : out.args) {
        if (!readBulk(arg)) return false;
    }
    out.timestamp = 0;
    return true;
}

bool AOFReader::replay(std::function<void(const AOFRecord&)> handler) {
    std::ifstream in(path_);
    if (!in.is_open()) return false;

    AOFRecord record;
    size_t count = 0;
    while (parseRecord(in, record)) {
        handler(record);
        ++count;
    }
    return true;
}

} // namespace storage
} // namespace spatialdb
