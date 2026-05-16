#include "script_engine.h"
#include <stdexcept>
#include <chrono>

namespace spatialdb {
namespace lua {

static constexpr size_t MAX_SOURCE_SIZE = 1024 * 1024; // 1MB max script size

ScriptEngine::ScriptEngine(ScriptConfig cfg) : cfg_(cfg) {}
ScriptEngine::~ScriptEngine() {}

bool ScriptEngine::loadScript(const std::string& name, const std::string& source) {
    if (source.size() > MAX_SOURCE_SIZE) return false;
    std::lock_guard<std::mutex> lock(mu_);
    scripts_[name] = source;
    return true;
}

bool ScriptEngine::unloadScript(const std::string& name) {
    std::lock_guard<std::mutex> lock(mu_);
    return scripts_.erase(name) > 0;
}

bool ScriptEngine::hasScript(const std::string& name) const {
    std::lock_guard<std::mutex> lock(mu_);
    return scripts_.count(name) > 0;
}

void ScriptEngine::registerFunction(const std::string& name,
    std::function<ScriptResult(std::vector<std::string>)> fn)
{
    std::lock_guard<std::mutex> lock(mu_);
    functions_[name] = std::move(fn);
}

ScriptResult ScriptEngine::execute(const std::string& source,
                                    const std::vector<std::string>& args) {
    auto t0 = std::chrono::steady_clock::now();
    ScriptResult result;

    // Stub: in production this would init a Lua state, push args, pcall
    // For now just check for registered function calls
    std::lock_guard<std::mutex> lock(mu_);
    for (const auto& [fname, fn] : functions_) {
        if (source.find(fname) != std::string::npos) {
            result = fn(args);
            result.duration_ms = (int64_t)std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - t0).count();
            return result;
        }
    }
    result.ok = true;
    result.value = "nil";
    result.duration_ms = (int64_t)std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - t0).count();
    return result;
}

ScriptResult ScriptEngine::runScript(const std::string& name,
                                      const std::vector<std::string>& args) {
    std::string src;
    {
        std::lock_guard<std::mutex> lock(mu_);
        auto it = scripts_.find(name);
        if (it == scripts_.end())
            return {false, "", "script not found: " + name};
        src = it->second;
    }
    return execute(src, args);
}

ScriptResult ScriptEngine::runInline(const std::string& source,
                                      const std::vector<std::string>& args) {
    return execute(source, args);
}

size_t ScriptEngine::scriptCount() const {
    std::lock_guard<std::mutex> lock(mu_);
    return scripts_.size();
}

} // namespace lua
} // namespace spatialdb
