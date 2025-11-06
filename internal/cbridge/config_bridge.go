package cbridge

// CppEngineConfig holds configuration for the C++ spatial engine.
type CppEngineConfig struct {
    // IndexType selects the spatial index backend: "rtree", "quadtree", "grid"
    IndexType string
    // WALPath is the path for the write-ahead log file
    WALPath string
    // SnapshotPath is the path for periodic snapshots
    SnapshotPath string
    // MaxMemTableBytes controls when to flush the memtable
    MaxMemTableBytes uint64
    // CompactionThreshold sets the WAL entry count before compaction
    CompactionThreshold uint64
    // EnableTLS enables TLS on the TCP server
    EnableTLS bool
    // TLSCertFile and TLSKeyFile for TLS configuration
    TLSCertFile string
    TLSKeyFile  string
    // ReplicationEnabled enables leader-follower replication
    ReplicationEnabled bool
    // NodeID is this node's unique identifier in the cluster
    NodeID string
}

// DefaultConfig returns a CppEngineConfig with sane defaults.
func DefaultConfig() CppEngineConfig {
    return CppEngineConfig{
        IndexType:           "rtree",
        WALPath:             "data/spatialdb.wal",
        SnapshotPath:        "data/spatialdb.snap",
        MaxMemTableBytes:    64 * 1024 * 1024, // 64 MB
        CompactionThreshold: 10000,
        EnableTLS:           false,
    }
}

// ApplyConfig applies the configuration to the C++ engine.
func ApplyConfig(cfg CppEngineConfig) error {
    // Stub: real impl would marshal to C struct and call into C++
    _ = cfg
    return nil
}
