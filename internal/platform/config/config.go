package config
// fix: guard concurrent access

import (
    "fmt"
    "time"

    "github.com/spf13/viper"
)

// Specification defines all runtime configuration for GeoEngine.
type Specification struct {
    Server   ServerConfig   ` + "`" + `mapstructure:"server"` + "`" + `
    Storage  StorageConfig  ` + "`" + `mapstructure:"storage"` + "`" + `
    Logging  LoggingConfig  ` + "`" + `mapstructure:"logging"` + "`" + `
    Security SecurityConfig ` + "`" + `mapstructure:"security"` + "`" + `
}

// ServerConfig holds HTTP/gRPC listener parameters.
type ServerConfig struct {
    HTTPPort      int           ` + "`" + `mapstructure:"http_port"` + "`" + `
    GRPCPort      int           ` + "`" + `mapstructure:"grpc_port"` + "`" + `
    ReadTimeout   time.Duration ` + "`" + `mapstructure:"read_timeout"` + "`" + `
    WriteTimeout  time.Duration ` + "`" + `mapstructure:"write_timeout"` + "`" + `
    ShutdownGrace time.Duration ` + "`" + `mapstructure:"shutdown_grace"` + "`" + `
}

// StorageConfig holds persistence layer parameters.
type StorageConfig struct {
    DataPath           string        ` + "`" + `mapstructure:"data_path"` + "`" + `
    SnapshotInterval   time.Duration ` + "`" + `mapstructure:"snapshot_interval"` + "`" + `
    MaxMemoryMB        int           ` + "`" + `mapstructure:"max_memory_mb"` + "`" + `
    ConnectionPoolSize int           ` + "`" + `mapstructure:"connection_pool_size"` + "`" + `
}

// LoggingConfig holds observability parameters.
type LoggingConfig struct {
    Level      string ` + "`" + `mapstructure:"level"` + "`" + `
    Format     string ` + "`" + `mapstructure:"format"` + "`" + `
    OutputPath string ` + "`" + `mapstructure:"output_path"` + "`" + `
}

// SecurityConfig holds TLS and auth parameters.
type SecurityConfig struct {
    TLSCertPath    string   ` + "`" + `mapstructure:"tls_cert_path"` + "`" + `
    TLSKeyPath     string   ` + "`" + `mapstructure:"tls_key_path"` + "`" + `
    AllowedOrigins []string ` + "`" + `mapstructure:"allowed_origins"` + "`" + `
    JWTSecretEnv   string   ` + "`" + `mapstructure:"jwt_secret_env"` + "`" + `
}

// Load reads configuration from file and environment overrides.
func Load(path string) (*Specification, error) {
    v := viper.New()
    v.SetConfigFile(path)
    v.SetEnvPrefix("GEOENGINE")
    v.AutomaticEnv()

    v.SetDefault("server.http_port", 8080)
    v.SetDefault("server.grpc_port", 9090)
    v.SetDefault("server.read_timeout", "30s")
    v.SetDefault("server.write_timeout", "30s")
    v.SetDefault("server.shutdown_grace", "15s")
    v.SetDefault("storage.data_path", "./data")
    v.SetDefault("storage.snapshot_interval", "5m")
    v.SetDefault("storage.max_memory_mb", 512)
    v.SetDefault("storage.connection_pool_size", 20)
    v.SetDefault("logging.level", "info")
    v.SetDefault("logging.format", "json")
    v.SetDefault("security.allowed_origins", []string{"https://app.geoengine.io"})
    v.SetDefault("security.jwt_secret_env", "GEOENGINE_JWT_SECRET")

    if err := v.ReadInConfig(); err != nil {
        return nil, fmt.Errorf("config load failed: %w", err)
    }

    var spec Specification
    if err := v.Unmarshal(&spec); err != nil {
        return nil, fmt.Errorf("config unmarshal failed: %w", err)
    }
    return &spec, nil
}
