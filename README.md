# llurl

A blazingly fast URL parser for C, based on llhttp's state machine approach.

## Features

- **Ultra-fast**: State machine-based parsing with lookup tables for optimal performance
- **Compatible**: API compatible with [http-parser](https://github.com/nodejs/http-parser)'s `http_parser_parse_url` function
- **Complete**: Handles all URL components (schema, userinfo, host, port, path, query, fragment)
- **Robust**: Supports IPv6 addresses, CONNECT requests, and edge cases
- **Zero dependencies**: Pure C99 with no external dependencies
- **Well-tested**: Comprehensive test suite included

## Building

```bash
make              # Build static and shared libraries
make test         # Build and run tests
make run-example  # Build and run example program
make run-benchmark # Build and run performance benchmarks
make debug        # Build with debug symbols and run tests
```

## Usage

### Basic Example

```c
#include "llurl.h"
#include <stdio.h>

int main() {
    const char *url = "http://example.com:8080/path?query=value#fragment";
    struct http_parser_url u;
    
    // Initialize the URL structure
    http_parser_url_init(&u);
    
    // Parse the URL
    int result = http_parser_parse_url(url, strlen(url), 0, &u);
    if (result != 0) {
        printf("Failed to parse URL\n");
        return 1;
    }
    
    // Check if a field is present
    if (u.field_set & (1 << UF_HOST)) {
        printf("Host: %.*s\n", 
               u.field_data[UF_HOST].len,
               url + u.field_data[UF_HOST].off);
    }
    
    // Get the port (automatically parsed)
    if (u.field_set & (1 << UF_PORT)) {
        printf("Port: %u\n", u.port);
    }
    
    return 0;
}
```

### API Reference

#### `http_parser_url_init`

```c
void http_parser_url_init(struct http_parser_url *u);
```

Initialize the URL structure to zeros before parsing.

#### `http_parser_parse_url`

```c
int http_parser_parse_url(const char *buf, size_t buflen,
                          int is_connect,
                          struct http_parser_url *u);
```

Parse a URL and extract its components.

**Parameters:**
- `buf`: URL string to parse
- `buflen`: Length of the URL string
- `is_connect`: Non-zero if this is a CONNECT request (expects authority form like `host:port`)
- `u`: Pointer to `http_parser_url` structure to fill

**Returns:** 0 on success, non-zero on failure

#### URL Components

The following URL components can be extracted:

- `UF_SCHEMA`: URL scheme (e.g., "http", "https", "ftp")
- `UF_HOST`: Hostname or IP address
- `UF_PORT`: Port number (also available as parsed integer in `u.port`)
- `UF_PATH`: Path component
- `UF_QUERY`: Query string (without the '?')
- `UF_FRAGMENT`: Fragment identifier (without the '#')
- `UF_USERINFO`: User information (username:password)

### Examples

#### Parsing a complete URL

```c
const char *url = "https://user:pass@example.com:443/api/v1?key=value#section";
struct http_parser_url u;
http_parser_url_init(&u);

if (http_parser_parse_url(url, strlen(url), 0, &u) == 0) {
    // Schema: "https"
    // Userinfo: "user:pass"
    // Host: "example.com"
    // Port: 443
    // Path: "/api/v1"
    // Query: "key=value"
    // Fragment: "section"
}
```

#### Parsing a relative URL

```c
const char *url = "/path/to/resource?id=123";
struct http_parser_url u;
http_parser_url_init(&u);

if (http_parser_parse_url(url, strlen(url), 0, &u) == 0) {
    // Path: "/path/to/resource"
    // Query: "id=123"
}
```

#### Parsing a CONNECT request

```c
const char *url = "example.com:443";
struct http_parser_url u;
http_parser_url_init(&u);

if (http_parser_parse_url(url, strlen(url), 1, &u) == 0) {
    // Host: "example.com"
    // Port: 443
}
```

#### Parsing IPv6 URLs

```c
const char *url = "http://[2001:db8::1]:8080/path";
struct http_parser_url u;
http_parser_url_init(&u);

if (http_parser_parse_url(url, strlen(url), 0, &u) == 0) {
    // Host: "[2001:db8::1]"
    // Port: 8080
    // Path: "/path"
}
```

## Performance

llurl uses a state machine approach inspired by [llhttp](https://github.com/nodejs/llhttp), which provides:

- **Lookup tables** for character classification (O(1) checks)
- **Minimal branching** through state machine design
- **Single-pass parsing** with no backtracking
- **Cache-friendly** memory access patterns

This makes llurl one of the fastest URL parsers available for C.

### Benchmark Results

Performance measurements on common URL patterns (1 million iterations each):

| URL Type | Throughput | Time per parse |
|----------|------------|----------------|
| Simple relative URL (`/path`) | ~100M parses/sec | ~0.01 µs |
| Simple absolute URL | ~32M parses/sec | ~0.03 µs |
| Complete URL with all components | ~12M parses/sec | ~0.08 µs |
| Query-heavy URL | ~10M parses/sec | ~0.10 µs |
| IPv6 URL | ~20M parses/sec | ~0.05 µs |
| CONNECT request | ~34M parses/sec | ~0.03 µs |

Run your own benchmarks:
```bash
make run-benchmark
```

## Design

The parser is implemented as a deterministic finite automaton (DFA) with the following states:

- `s_start`: Initial state, determines URL type
- `s_schema`: Parsing scheme (http, https, etc.)
- `s_schema_slash`, `s_schema_slash_slash`: Parsing "://"
- `s_server_start`, `s_server`, `s_server_with_at`: Parsing host/userinfo
- `s_path`: Parsing path component
- `s_query_or_fragment`: Deciding between query or fragment
- `s_query`: Parsing query string
- `s_fragment`: Parsing fragment identifier

Character classification is performed using pre-computed lookup tables for maximum performance.

## License

MIT License - See header files for full license text.

## Contributing

Contributions are welcome! Please ensure all tests pass before submitting a pull request.

## References

- [llhttp](https://github.com/nodejs/llhttp) - High-performance HTTP parser
- [http-parser](https://github.com/nodejs/http-parser) - Original Node.js HTTP parser
- [RFC 3986](https://tools.ietf.org/html/rfc3986) - URI Generic Syntax
