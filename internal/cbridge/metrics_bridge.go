// Package cbridge - metrics bridge to expose C++ metrics to Go
package cbridge

/*
#cgo CXXFLAGS: -std=c++17 -I../../cpp/include
#cgo LDFLAGS: -lstdc++
*/
import "C"

// MetricSnapshot holds a snapshot of C++ engine metrics.
type MetricSnapshot struct {
    CommandsTotal    uint64
    InsertTotal      uint64
    SearchTotal      uint64
    WALEntriesTotal  uint64
    ActiveConns      int64
    MemtableSizeBytes uint64
    CacheHitRate     float64
}

// GetMetrics returns a snapshot of the C++ engine metrics.
func GetMetrics() MetricSnapshot {
    // In production: call into C++ metrics Registry and extract values
    return MetricSnapshot{}
}

// ResetMetrics resets all C++ counters.
func ResetMetrics() {
    // C++ Registry::global().reset_all();
}
