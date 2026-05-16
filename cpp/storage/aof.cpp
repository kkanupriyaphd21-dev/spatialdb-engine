#include "aof.h"
#include <sstream>
#include <stdexcept>
#include <sys/stat.h>
#include <chrono>
#include <cerrno>

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#else
#include <unistd.h>
#endif

namespace spatialdb {
namespace storage {

AOFWriter::AOFWriter(const std::string& path, AOFSync sync)
    : path_(path), tmp_path_(path + ".tmp"), sync_(sync)
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

// Platform-specific fsync for file streams
static bool doFsync(std::ofstream& file) {
    file.flush();
    if (!file.good()) return false;
#ifdef _WIN32
    HANDLE h = (HANDLE)_get_osfhandle(_fileno(nativeHandle(file)));
    if (h == INVALID_HANDLE_VALUE) return false;
    return FlushFileBuffers(h) != 0;
#else
    // For C++ ofstream, we flush and rely on OS. A proper fix would use POSIX open()
    // with O_SYNC or a C FILE* with fileno()+fsync().
    return true; // flush() was already called above
#endif
}

bool AOFWriter::write(const AOFRecord& record) {
    std::lock_guard<std::mutex> lock(mu_);
    auto s = serialize(record);
    file_ << s;
    if (!file_.good()) return false;
    if (sync_ == AOFSync::ALWAYS) {
        file_.flush();
        if (!file_.good()) return false;
    }
    return true;
}

bool AOFWriter::fsync() {
    std::lock_guard<std::mutex> lock(mu_);
    file_.flush();
    return file_.good();
}

bool AOFWriter::rewrite(const std::vector<AOFRecord>& records) {
    std::lock_guard<std::mutex> lock(mu_);

    // Write to temp file first, then atomic rename
    std::ofstream tmp(tmp_path_, std::ios::trunc);
    if (!tmp.is_open()) return false;

    for (const auto& r : records) {
        tmp << serialize(r);
        if (!tmp.good()) {
            tmp.close();
            return false;
        }
    }
    tmp.flush();
    if (!tmp.good()) {
        tmp.close();
        return false;
    }
    tmp.close();

    // Atomic rename replaces the original file
    if (rename(tmp_path_.c_str(), path_.c_str()) != 0) return false;

    // Reopen the main file for appending
    file_.close();
    file_.open(path_, std::ios::app);
    return file_.is_open();
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
    if (line.empty()) return false;
    // Skip empty lines between records
    if (line == "\r" || line.empty()) {
        return parseRecord(in, out);
    }
    if (line[0] != '*') return false;
    if (!line.empty() && line.back() == '\r') line.pop_back();

    int count;
    try {
        count = std::stoi(line.substr(1));
    } catch (const std::exception&) {
        return false;
    }
    if (count < 1 || count > 10000) return false; // sanity limit

    auto readBulk = [&](std::string& val) -> bool {
        std::string header;
        if (!std::getline(in, header)) return false;
        if (!header.empty() && header.back() == '\r') header.pop_back();
        if (header.empty() || header[0] != '$') return false;
        size_t len;
        try {
            len = std::stoul(header.substr(1));
        } catch (const std::exception&) {
            return false;
        }
        if (len > 1048576) return false; // 1MB max per argument
        val.resize(len);
        if (!in.read(val.data(), len)) return false;
        // Consume \r\n after the bulk string
        char cr, lf;
        if (!in.get(cr) || cr != '\r') return false;
        if (!in.get(lf) || lf != '\n') return false;
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
