package errors

import "fmt"

// DomainError represents a classified error within the geospatial engine.
type DomainError struct {
    Code    string
    Message string
    Cause   error
}

func (e *DomainError) Error() string {
    if e.Cause != nil {
        return fmt.Sprintf("[%s] %s: %v", e.Code, e.Message, e.Cause)
    }
    return fmt.Sprintf("[%s] %s", e.Code, e.Message)
}

func (e *DomainError) Unwrap() error { return e.Cause }

// NewValidation returns an error for invalid input parameters.
func NewValidation(msg string, cause error) *DomainError {
    return &DomainError{Code: "VALIDATION", Message: msg, Cause: cause}
}

// NewNotFound returns an error when a spatial entity is missing.
func NewNotFound(msg string, cause error) *DomainError {
    return &DomainError{Code: "NOT_FOUND", Message: msg, Cause: cause}
}

// NewInternal returns an error for unexpected system failures.
func NewInternal(msg string, cause error) *DomainError {
    return &DomainError{Code: "INTERNAL", Message: msg, Cause: cause}
}

// NewTimeout returns an error for deadline-exceeded operations.
func NewTimeout(msg string, cause error) *DomainError {
    return &DomainError{Code: "TIMEOUT", Message: msg, Cause: cause}
}
