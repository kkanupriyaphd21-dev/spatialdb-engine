#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>
#include <mutex>

namespace spatialdb {
namespace lua {

struct ScriptResult {
    bool        ok = false;
    std::string value;
    std::string error;
};

// Light script sandbox — runs Lua snippets against the spatial data
// Actual Lua interpreter integration stubbed; structure mirrors the Go version
class ScriptEngine {
public:
    ScriptEngine();
    ~ScriptEngine();

    bool   loadScript(const std::string& name, const std::string& source);
    bool   unloadScript(const std::string& name);
    bool   hasScript(const std::string& name) const;

    ScriptResult runScript(const std::string& name,
                            const std::vector<std::string>& args);
    ScriptResult runInline(const std::string& source,
                            const std::vector<std::string>& args);

    void registerFunction(const std::string& name,
                          std::function<ScriptResult(std::vector<std::string>)> fn);

    size_t scriptCount() const;

private:
    mutable std::mutex mu_;
    std::unordered_map<std::string, std::string>           scripts_;
    std::unordered_map<std::string,
        std::function<ScriptResult(std::vector<std::string>)>> functions_;

    ScriptResult execute(const std::string& source,
                          const std::vector<std::string>& args);
};

} // namespace lua
} // namespace spatialdb
