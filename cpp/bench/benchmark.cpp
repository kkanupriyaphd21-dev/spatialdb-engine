#include "benchmark.h"
#include <algorithm>
#include <numeric>

namespace spatialdb {
namespace bench {

Benchmark& Benchmark::global() {
    static Benchmark instance;
    return instance;
}

void Benchmark::add(const std::string& name,
                     std::function<void()> setup,
                     std::function<void()> fn,
                     size_t iterations)
{
    cases_.push_back({name, std::move(setup), std::move(fn), iterations});
}

BenchResult Benchmark::runCase(const BenchCase& bc) {
    if (bc.setup) bc.setup();

    std::vector<double> timings;
    timings.reserve(bc.iterations);

    double total = 0.0;
    for (size_t i = 0; i < bc.iterations; ++i) {
        Timer t;
        bc.fn();
        double ns = t.elapsedNs();
        timings.push_back(ns);
        total += ns;
    }

    std::sort(timings.begin(), timings.end());

    BenchResult r;
    r.name        = bc.name;
    r.iterations  = bc.iterations;
    r.total_ms    = total / 1e6;
    r.avg_ns      = total / bc.iterations;
    r.min_ns      = timings.front();
    r.max_ns      = timings.back();
    r.p99_ns      = timings[(size_t)(0.99 * timings.size())];
    r.ops_per_sec = bc.iterations / (total / 1e9);
    return r;
}

void Benchmark::run(const std::string& name_filter) {
    results_.clear();
    for (const auto& bc : cases_) {
        if (!name_filter.empty() && bc.name.find(name_filter) == std::string::npos)
            continue;
        std::cout << "Running: " << bc.name << " (" << bc.iterations << " iters)...\n";
        results_.push_back(runCase(bc));
    }
    printResults();
}

void Benchmark::printResults() const {
    std::cout << "\n" << std::string(80, '-') << "\n";
    std::cout << std::left
              << std::setw(30) << "Benchmark"
              << std::setw(12) << "Avg (ns)"
              << std::setw(12) << "Min (ns)"
              << std::setw(12) << "p99 (ns)"
              << std::setw(14) << "ops/sec"
              << "\n";
    std::cout << std::string(80, '-') << "\n";

    for (const auto& r : results_) {
        std::cout << std::left
                  << std::setw(30) << r.name
                  << std::setw(12) << std::fixed << std::setprecision(1) << r.avg_ns
                  << std::setw(12) << r.min_ns
                  << std::setw(12) << r.p99_ns
                  << std::setw(14) << std::setprecision(0) << r.ops_per_sec
                  << "\n";
    }
    std::cout << std::string(80, '-') << "\n\n";
}

} // namespace bench
} // namespace spatialdb
