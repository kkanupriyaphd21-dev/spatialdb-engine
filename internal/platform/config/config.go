package config

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

// Validate performs comprehensive validation of the configuration.
func (s *Specification) Validate() error {
	var errs []string

	if err := s.Server.Validate(); err != nil {
		errs = append(errs, err.Error())
	}
	if err := s.Storage.Validate(); err != nil {
		errs = append(errs, err.Error())
	}
	if err := s.Logging.Validate(); err != nil {
		errs = append(errs, err.Error())
	}
	if err := s.Security.Validate(); err != nil {
		errs = append(errs, err.Error())
	}

	if len(errs) > 0 {
		return fmt.Errorf("configuration validation failed: %v", errs)
	}
	return nil
}

// Validate checks server configuration values.
func (c *ServerConfig) Validate() error {
	var errs []string

	if c.HTTPPort < 1 || c.HTTPPort > 65535 {
		errs = append(errs, fmt.Sprintf("http_port must be 1-65535, got %d", c.HTTPPort))
	}
	if c.GRPCPort < 1 || c.GRPCPort > 65535 {
		errs = append(errs, fmt.Sprintf("grpc_port must be 1-65535, got %d", c.GRPCPort))
	}
	if c.ReadTimeout <= 0 {
		errs = append(errs, "read_timeout must be positive")
	}
	if c.WriteTimeout <= 0 {
		errs = append(errs, "write_timeout must be positive")
	}
	if c.ShutdownGrace <= 0 {
		errs = append(errs, "shutdown_grace must be positive")
	}

	if len(errs) > 0 {
		return fmt.Errorf("server config: %v", errs)
	}
	return nil
}

// Validate checks storage configuration values.
func (c *StorageConfig) Validate() error {
	var errs []string

	if c.DataPath == "" {
		errs = append(errs, "data_path cannot be empty")
	}
	if c.SnapshotInterval <= 0 {
		errs = append(errs, "snapshot_interval must be positive")
	}
	if c.MaxMemoryMB <= 0 {
		errs = append(errs, "max_memory_mb must be positive")
	}
	if c.MaxMemoryMB > 65536 {
		errs = append(errs, fmt.Sprintf("max_memory_mb too large: %d (max 65536)", c.MaxMemoryMB))
	}
	if c.ConnectionPoolSize <= 0 {
		errs = append(errs, "connection_pool_size must be positive")
	}
	if c.ConnectionPoolSize > 10000 {
		errs = append(errs, fmt.Sprintf("connection_pool_size too large: %d (max 10000)", c.ConnectionPoolSize))
	}

	if len(errs) > 0 {
		return fmt.Errorf("storage config: %v", errs)
	}
	return nil
}

// Validate checks logging configuration values.
func (c *LoggingConfig) Validate() error {
	validLevels := map[string]bool{
		"debug": true, "info": true, "warn": true, "error": true, "fatal": true,
	}
	if !validLevels[c.Level] {
		return fmt.Errorf("logging level '%s' not valid (debug|info|warn|error|fatal)", c.Level)
	}
	validFormats := map[string]bool{
		"json": true, "text": true, "console": true,
	}
	if !validFormats[c.Format] {
		return fmt.Errorf("logging format '%s' not valid (json|text|console)", c.Format)
	}
	return nil
}

// Validate checks security configuration values.
func (c *SecurityConfig) Validate() error {
	if c.JWTSecretEnv == "" {
		return fmt.Errorf("jwt_secret_env cannot be empty")
	}
	for _, origin := range c.AllowedOrigins {
		if origin == "*" {
			return fmt.Errorf("wildcard '*' origin is not allowed for security reasons")
		}
	}
	return nil
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

	if err := spec.Validate(); err != nil {
		return nil, fmt.Errorf("config validation failed: %w", err)
	}

	return &spec, nil
}
