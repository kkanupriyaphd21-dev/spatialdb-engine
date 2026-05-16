#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>
#include <mutex>

namespace spatialdb {
namespace lua {

struct ScriptConfig {
    size_t max_instructions = 1000000; // max Lua instructions per execution
    size_t max_memory_kb      = 10240; // max memory per script (10MB)
    int    timeout_ms         = 5000;  // execution timeout
};

struct ScriptResult {
    bool        ok = false;
    std::string value;
    std::string error;
    int64_t     instructions_executed = 0;
    int64_t     duration_ms = 0;
};

// Light script sandbox — runs Lua snippets against the spatial data
// Actual Lua interpreter integration stubbed; structure mirrors the Go version
class ScriptEngine {
public:
    explicit ScriptEngine(ScriptConfig cfg = {});
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
    const ScriptConfig& config() const { return cfg_; }

private:
    ScriptConfig cfg_;
    mutable std::mutex mu_;
    std::unordered_map<std::string, std::string>           scripts_;
    std::unordered_map<std::string,
        std::function<ScriptResult(std::vector<std::string>)>> functions_;

    ScriptResult execute(const std::string& source,
                          const std::vector<std::string>& args);
};

} // namespace lua
} // namespace spatialdb
