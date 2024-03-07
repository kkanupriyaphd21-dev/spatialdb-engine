#include "script_engine.h"
#include <stdexcept>

namespace spatialdb {
namespace lua {

ScriptEngine::ScriptEngine() {}
ScriptEngine::~ScriptEngine() {}

bool ScriptEngine::loadScript(const std::string& name, const std::string& source) {
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
    // Stub: in production this would init a Lua state, push args, pcall
    // For now just check for registered function calls
    std::lock_guard<std::mutex> lock(mu_);
    for (const auto& [fname, fn] : functions_) {
        if (source.find(fname) != std::string::npos) {
            return fn(args);
        }
    }
    return {true, "nil", ""};
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
