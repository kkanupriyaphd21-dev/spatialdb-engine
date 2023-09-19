# Security Policy

## Supported Versions

| Version | Supported |
|---------|-----------|
| 1.x     | ✅        |
| < 1.0   | ❌        |

## Reporting a Vulnerability

Report security vulnerabilities by emailing **security@spatialdb.io**.

Do **not** open a public issue for security vulnerabilities.

You will receive a response within 48 hours. If the issue is confirmed, a patch
will be released as soon as possible — typically within 7 days for critical issues.

## Security Hardening

- TLS 1.3 enforced on all network interfaces
- No sensitive data in logs
- All secrets loaded from environment variables only
- Input validation on all API endpoints
- Dependency scanning via `govulncheck` on every release
<!-- rev: 1 -->
<!-- rev: 2 -->
<!-- rev: 3 -->
<!-- rev: 4 -->
