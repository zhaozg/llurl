# URL Parser Test Suite

This document describes the comprehensive test suite for the llurl URL parser.

## Test Files

- **test_llurl.c** - Original basic test suite (13 tests)
- **test_llurl_comprehensive.c** - Comprehensive test suite (51 tests)

## Running Tests

```bash
make test               # Run basic tests only
make test-comprehensive # Run comprehensive tests only
make test-all           # Run all tests (basic + comprehensive)
```

## Comprehensive Test Coverage

### 1. Positive Tests - Valid URLs (20 tests)

These tests verify that the parser correctly handles valid URLs according to RFC 3986:

- Basic HTTPS URL with all components (protocol, auth, host, port, path, query, fragment)
- URLs without authentication and port
- URLs with only host
- Relative URLs (path-only, path+query, path+fragment)
- URLs with IPv4 addresses
- URLs with IPv6 addresses
- URLs with IPv6 link-local addresses (with zone ID)
- URLs with encoded characters (percent-encoding)
- URLs with special characters in query strings
- URLs with asterisk in path (for HTTP OPTIONS * requests)
- URLs with dots in hostname (e.g., example.co.uk)
- URLs with underscores in hostname
- URLs with multiple query parameters
- URLs with empty query string (trailing ?)
- URLs with empty fragment (trailing #)
- URLs with only fragment (should fail)
- URLs with only query (should fail)
- Protocol-relative URLs (//host, //host/path, //host:port/path)

### 2. CONNECT Mode Tests (7 tests)

These tests verify the special behavior for HTTP CONNECT requests:

- Valid host:port combination in CONNECT mode
- Reject URLs with path in CONNECT mode
- Reject URLs with query in CONNECT mode
- Reject URLs without port in CONNECT mode
- IPv6 address with port in CONNECT mode
- IPv6 address without port in CONNECT mode (should fail)
- Comparison between normal and CONNECT mode parsing

### 3. Negative Tests - Invalid URLs (11 tests)

These tests verify that the parser correctly rejects invalid URLs:

- Empty string
- Spaces in hostname
- Port number with letters
- Port number out of range (> 65535)
- Missing host after schema (http:///)
- Unclosed IPv6 bracket
- Double @ in authentication
- Control characters (newline, tab, etc.)
- Only schema (http:)
- Only schema and slashes (http://)
- Bad schema format (http:/path)

### 4. Edge Case Tests (10 tests)

These tests verify boundary conditions and special cases:

- Very long URLs (1000+ characters in path)
- Maximum valid port number (65535)
- Port number 0
- Mixed case scheme (HTTP vs http)
- International domain names (Unicode characters)
- Plus sign and percent-encoding in query
- State isolation between multiple parser calls
- Root path only (http://example.com/)
- Query string with embedded ? character
- Fragment with special characters (?, #)

### 5. Additional Protocol Tests (3 tests)

These tests verify support for various URL schemes:

- FTP protocol (ftp://example.com/file.txt)
- WebSocket protocol (ws://example.com/chat)
- HTTPS API URLs with multiple query parameters

## Test Results

All 51 comprehensive tests pass with 100% success rate:

```
=====================================
  TEST SUMMARY
=====================================
Total tests: 51
Passed:      51
Failed:      0

✓ ALL TESTS PASSED!
=====================================
```

## Test Implementation

The tests use:

- **Assertions** for validation
- **Helper functions** for field checking
- **Test macros** for consistent test structure
- **Descriptive names** for easy identification
- **Comprehensive error checking** for all edge cases

Each test:
1. Initializes a clean parser state
2. Parses the URL
3. Validates the result (success/failure)
4. Checks each extracted field against expected values
5. Reports pass/fail status

## References

Tests are based on:
- RFC 3986 (URI Generic Syntax)
- HTTP/1.1 CONNECT method specification
- Real-world URL parsing requirements
- Security best practices for input validation
