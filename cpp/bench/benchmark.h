#pragma once
#include <string>
#include <vector>
#include <functional>
#include <chrono>
#include <unordered_map>
#include <iostream>
#include <iomanip>

namespace spatialdb {
namespace bench {

struct BenchResult {
    std::string  name;
    size_t       iterations;
    double       total_ms;
    double       avg_ns;
    double       min_ns;
    double       max_ns;
    double       p99_ns;
    double       ops_per_sec;
};

class Benchmark {
public:
    static Benchmark& global();

    void add(const std::string& name,
             std::function<void()> setup,
             std::function<void()> fn,
             size_t iterations = 100000);

    void run(const std::string& name_filter = "");
    void printResults() const;

    const std::vector<BenchResult>& results() const { return results_; }

private:
    struct BenchCase {
        std::string           name;
        std::function<void()> setup;
        std::function<void()> fn;
        size_t                iterations;
    };

    std::vector<BenchCase>   cases_;
    std::vector<BenchResult> results_;

    BenchResult runCase(const BenchCase& bc);
};

// RAII timer
class Timer {
public:
    Timer() : start_(std::chrono::high_resolution_clock::now()) {}

    double elapsedNs() const {
        auto now = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<double, std::nano>(now - start_).count();
    }

    double elapsedMs() const { return elapsedNs() / 1e6; }

private:
    std::chrono::high_resolution_clock::time_point start_;
};

} // namespace bench
} // namespace spatialdb
