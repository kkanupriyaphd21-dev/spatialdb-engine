#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <mutex>
#include <functional>

namespace spatialdb {
namespace storage {

enum class AOFSync {
    ALWAYS,
    EVERY_SECOND,
    NO,
};

struct AOFRecord {
    std::string command;
    std::vector<std::string> args;
    uint64_t timestamp;
};

class AOFWriter {
public:
    explicit AOFWriter(const std::string& path, AOFSync sync = AOFSync::EVERY_SECOND);
    ~AOFWriter();

    bool write(const AOFRecord& record);
    bool fsync();
    bool rewrite(const std::vector<AOFRecord>& records);
    size_t fileSize() const;
    bool   isOpen()   const { return file_.is_open(); }

private:
    std::string   path_;
    std::string   tmp_path_;
    std::ofstream file_;
    std::mutex    mu_;
    AOFSync       sync_;

    std::string serialize(const AOFRecord& r) const;
};

class AOFReader {
public:
    explicit AOFReader(const std::string& path);
    bool replay(std::function<void(const AOFRecord&)> handler);

private:
    std::string path_;
    bool parseRecord(std::ifstream& in, AOFRecord& out);
};

} // namespace storage
} // namespace spatialdb
