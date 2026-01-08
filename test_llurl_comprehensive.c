#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "llurl.h"

/* Test counter */
static int test_count = 0;
static int test_passed = 0;

/* Helper function to print URL parse results */
void print_url_result(const char *url, const struct http_parser_url *u) {
  const char *field_names[] = {
    "SCHEMA", "HOST", "PORT", "PATH", "QUERY", "FRAGMENT", "USERINFO"
  };

  printf("URL: %s\n", url);

  for (int i = 0; i < UF_MAX; i++) {
    if (u->field_set & (1 << i)) {
      printf("  %s: off=%d len=%d value=\"%.*s\"\n",
             field_names[i],
             u->field_data[i].off,
             u->field_data[i].len,
             u->field_data[i].len,
             url + u->field_data[i].off);
    }
  }

  if (u->field_set & (1 << UF_PORT)) {
    printf("  PORT (decoded): %u\n", u->port);
  }

  printf("\n");
}

/* Helper function to check if field matches expected value */
int check_field(const char *url, const struct http_parser_url *u,
                enum http_parser_url_fields field,
                const char *expected) {
  if (expected == NULL) {
    return !(u->field_set & (1 << field));
  }

  if (!(u->field_set & (1 << field))) {
    return 0;
  }

  size_t expected_len = strlen(expected);
  if (u->field_data[field].len != expected_len) {
    return 0;
  }

  return memcmp(url + u->field_data[field].off, expected, expected_len) == 0;
}

#define TEST_START(name) \
  do { \
    test_count++; \
    printf("=== Test %d: %s ===\n", test_count, name); \
  } while(0)

#define TEST_PASS() \
  do { \
    test_passed++; \
    printf("✓ Test passed\n\n"); \
  } while(0)

/* ============================================
 * Positive Tests - Valid URLs
 * ============================================ */

void test_basic_https_url() {
  TEST_START("Basic HTTPS URL with all components");
  const char *url = "https://user:pass@example.com:8080/path?query=value#hash";
  struct http_parser_url u = { 0 };

  int result = http_parser_parse_url(url, strlen(url), 0, &u);
  assert(result == 0);

  assert(check_field(url, &u, UF_SCHEMA, "https"));
  assert(check_field(url, &u, UF_USERINFO, "user:pass"));
  assert(check_field(url, &u, UF_HOST, "example.com"));
  assert(check_field(url, &u, UF_PORT, "8080"));
  assert(u.port == 8080);
  assert(check_field(url, &u, UF_PATH, "/path"));
  assert(check_field(url, &u, UF_QUERY, "query=value"));
  assert(check_field(url, &u, UF_FRAGMENT, "hash"));

  TEST_PASS();
}

void test_url_without_auth_and_port() {
  TEST_START("URL without auth and port");
  const char *url = "http://example.com/path";
  struct http_parser_url u = { 0 };

  int result = http_parser_parse_url(url, strlen(url), 0, &u);
  assert(result == 0);

  assert(check_field(url, &u, UF_SCHEMA, "http"));
  assert(check_field(url, &u, UF_HOST, "example.com"));
  assert(check_field(url, &u, UF_PORT, NULL));
  assert(check_field(url, &u, UF_USERINFO, NULL));
  assert(check_field(url, &u, UF_PATH, "/path"));

  TEST_PASS();
}

void test_url_with_only_host() {
  TEST_START("URL with only host");
  const char *url = "http://example.com";
  struct http_parser_url u = { 0 };

  int result = http_parser_parse_url(url, strlen(url), 0, &u);
  assert(result == 0);

  assert(check_field(url, &u, UF_SCHEMA, "http"));
  assert(check_field(url, &u, UF_HOST, "example.com"));
  assert(check_field(url, &u, UF_PATH, NULL) || check_field(url, &u, UF_PATH, ""));

  TEST_PASS();
}

void test_relative_url() {
  TEST_START("Relative URL");
  const char *url = "/foo/t.html?qstring#frag";
  struct http_parser_url u = { 0 };

  int result = http_parser_parse_url(url, strlen(url), 0, &u);
  assert(result == 0);

  assert(check_field(url, &u, UF_PATH, "/foo/t.html"));
  assert(check_field(url, &u, UF_QUERY, "qstring"));
  assert(check_field(url, &u, UF_FRAGMENT, "frag"));
  assert(check_field(url, &u, UF_SCHEMA, NULL));
  assert(check_field(url, &u, UF_HOST, NULL));

  TEST_PASS();
}

void test_url_with_ipv4() {
  TEST_START("URL with IPv4 address");
  const char *url = "http://192.168.1.1:8080/test";
  struct http_parser_url u = { 0 };

  int result = http_parser_parse_url(url, strlen(url), 0, &u);
  assert(result == 0);

  assert(check_field(url, &u, UF_HOST, "192.168.1.1"));
  assert(check_field(url, &u, UF_PORT, "8080"));
  assert(u.port == 8080);

  TEST_PASS();
}

void test_url_with_ipv6() {
  TEST_START("URL with IPv6 address");
  const char *url = "http://[::1]:8080/path";
  struct http_parser_url u = { 0 };

  int result = http_parser_parse_url(url, strlen(url), 0, &u);
  assert(result == 0);

  assert(check_field(url, &u, UF_HOST, "::1"));
  assert(check_field(url, &u, UF_PORT, "8080"));
  assert(u.port == 8080);
  assert(check_field(url, &u, UF_PATH, "/path"));

  TEST_PASS();
}

void test_url_with_ipv6_link_local() {
  TEST_START("URL with IPv6 link-local address");
  const char *url = "http://[fe80::1%eth0]:8080/path";
  struct http_parser_url u = { 0 };

  int result = http_parser_parse_url(url, strlen(url), 0, &u);
  assert(result == 0);

  assert(check_field(url, &u, UF_HOST, "fe80::1%eth0"));
  assert(check_field(url, &u, UF_PORT, "8080"));
  assert(u.port == 8080);

  TEST_PASS();
}

void test_url_with_encoded_characters() {
  TEST_START("URL with encoded characters");
  const char *url = "http://dev:123456@hello.com:8080/some/path?with=1%23&args=value#hash";
  struct http_parser_url u = { 0 };

  int result = http_parser_parse_url(url, strlen(url), 0, &u);
  assert(result == 0);

  assert(check_field(url, &u, UF_SCHEMA, "http"));
  assert(check_field(url, &u, UF_USERINFO, "dev:123456"));
  assert(check_field(url, &u, UF_HOST, "hello.com"));
  assert(check_field(url, &u, UF_PORT, "8080"));
  assert(check_field(url, &u, UF_PATH, "/some/path"));
  assert(check_field(url, &u, UF_QUERY, "with=1%23&args=value"));
  assert(check_field(url, &u, UF_FRAGMENT, "hash"));

  TEST_PASS();
}

void test_url_with_special_query_chars() {
  TEST_START("URL with special characters in query");
  const char *url = "/search?q=hello+world&lang=en-US";
  struct http_parser_url u = { 0 };

  int result = http_parser_parse_url(url, strlen(url), 0, &u);
  assert(result == 0);

  assert(check_field(url, &u, UF_QUERY, "q=hello+world&lang=en-US"));

  TEST_PASS();
}

void test_url_with_asterisk() {
  TEST_START("URL with asterisk in path");
  const char *url = "*";
  struct http_parser_url u = { 0 };

  int result = http_parser_parse_url(url, strlen(url), 0, &u);
  assert(result == 0);

  assert(check_field(url, &u, UF_PATH, "*"));

  TEST_PASS();
}

void test_url_with_dot_in_hostname() {
  TEST_START("URL with dot in hostname");
  const char *url = "http://example.co.uk/path";
  struct http_parser_url u = { 0 };

  int result = http_parser_parse_url(url, strlen(url), 0, &u);
  assert(result == 0);

  assert(check_field(url, &u, UF_HOST, "example.co.uk"));

  TEST_PASS();
}

void test_url_with_underscore_in_hostname() {
  TEST_START("URL with underscore in hostname");
  const char *url = "http://my_server.com/path";
  struct http_parser_url u = { 0 };

  int result = http_parser_parse_url(url, strlen(url), 0, &u);
  assert(result == 0);

  assert(check_field(url, &u, UF_HOST, "my_server.com"));

  TEST_PASS();
}

void test_url_with_multiple_query_params() {
  TEST_START("URL with multiple query parameters");
  const char *url = "/test?a=1&b=2&c=3";
  struct http_parser_url u = { 0 };

  int result = http_parser_parse_url(url, strlen(url), 0, &u);
  assert(result == 0);

  assert(check_field(url, &u, UF_QUERY, "a=1&b=2&c=3"));

  TEST_PASS();
}

void test_url_with_empty_query() {
  TEST_START("URL with empty query");
  const char *url = "/test?";
  struct http_parser_url u = { 0 };

  int result = http_parser_parse_url(url, strlen(url), 0, &u);
  assert(result == 0);

  assert(check_field(url, &u, UF_QUERY, ""));
  assert(check_field(url, &u, UF_PATH, "/test"));

  TEST_PASS();
}

void test_url_with_empty_fragment() {
  TEST_START("URL with empty fragment");
  const char *url = "/test#";
  struct http_parser_url u = { 0 };

  int result = http_parser_parse_url(url, strlen(url), 0, &u);
  assert(result == 0);

  assert(check_field(url, &u, UF_FRAGMENT, ""));
  assert(check_field(url, &u, UF_PATH, "/test"));

  TEST_PASS();
}

void test_url_with_only_fragment() {
  TEST_START("URL with only fragment");
  const char *url = "#fragment-only";
  struct http_parser_url u = { 0 };

  int result = http_parser_parse_url(url, strlen(url), 0, &u);
  // This should fail according to the Lua tests
  assert(result != 0);

  TEST_PASS();
}

void test_url_with_only_query() {
  TEST_START("URL with only query");
  const char *url = "?query-only";
  struct http_parser_url u = { 0 };

  int result = http_parser_parse_url(url, strlen(url), 0, &u);
  // This should fail according to the Lua tests
  assert(result != 0);

  TEST_PASS();
}

void test_protocol_relative_url_with_host() {
  TEST_START("Protocol-relative URL //host");
  const char *url = "//host";
  struct http_parser_url u = { 0 };

  int result = http_parser_parse_url(url, strlen(url), 0, &u);
  assert(result == 0);
  assert(check_field(url, &u, UF_HOST, "host"));

  TEST_PASS();
}

void test_protocol_relative_url_with_path() {
  TEST_START("Protocol-relative URL //example.com/path");
  const char *url = "//example.com/path";
  struct http_parser_url u = { 0 };

  int result = http_parser_parse_url(url, strlen(url), 0, &u);
  assert(result == 0);
  assert(check_field(url, &u, UF_HOST, "example.com"));
  assert(check_field(url, &u, UF_PATH, "/path"));

  TEST_PASS();
}

void test_protocol_relative_url_with_port() {
  TEST_START("Protocol-relative URL //example.com:8080/path");
  const char *url = "//example.com:8080/path";
  struct http_parser_url u = { 0 };

  int result = http_parser_parse_url(url, strlen(url), 0, &u);
  assert(result == 0);

  TEST_PASS();
}

/* ============================================
 * CONNECT Mode Tests
 * ============================================ */

void test_connect_host_port() {
  TEST_START("CONNECT mode: host:port");
  const char *url = "192.168.0.1:80";
  struct http_parser_url u = { 0 };

  int result = http_parser_parse_url(url, strlen(url), 1, &u);
  assert(result == 0);
  assert(check_field(url, &u, UF_HOST, "192.168.0.1"));
  assert(check_field(url, &u, UF_PORT, "80"));
  assert(u.port == 80);

  TEST_PASS();
}

void test_connect_reject_path() {
  TEST_START("CONNECT mode: reject URL with path");
  const char *url = "192.168.0.1:80/path";
  struct http_parser_url u = { 0 };

  int result = http_parser_parse_url(url, strlen(url), 1, &u);
  assert(result != 0);

  TEST_PASS();
}

void test_connect_reject_query() {
  TEST_START("CONNECT mode: reject URL with query");
  const char *url = "192.168.0.1:80?query";
  struct http_parser_url u = { 0 };

  int result = http_parser_parse_url(url, strlen(url), 1, &u);
  assert(result != 0);

  TEST_PASS();
}

void test_connect_reject_no_port() {
  TEST_START("CONNECT mode: reject URL without port");
  const char *url = "192.168.0.1";
  struct http_parser_url u = { 0 };

  int result = http_parser_parse_url(url, strlen(url), 1, &u);
  assert(result != 0);

  TEST_PASS();
}

void test_connect_ipv6() {
  TEST_START("CONNECT mode: IPv6 with port");
  const char *url = "[::1]:8080";
  struct http_parser_url u = { 0 };

  int result = http_parser_parse_url(url, strlen(url), 1, &u);
  assert(result == 0);
  assert(check_field(url, &u, UF_HOST, "::1"));
  assert(check_field(url, &u, UF_PORT, "8080"));
  assert(u.port == 8080);

  TEST_PASS();
}

void test_connect_ipv6_no_port() {
  TEST_START("CONNECT mode: reject IPv6 without port");
  const char *url = "[::1]";
  struct http_parser_url u = { 0 };

  int result = http_parser_parse_url(url, strlen(url), 1, &u);
  assert(result != 0);

  TEST_PASS();
}

void test_connect_vs_normal_mode() {
  TEST_START("CONNECT mode vs normal mode comparison");
  const char *url = "192.168.0.1:80";
  struct http_parser_url u1 = { 0 };
  struct http_parser_url u2 = { 0 };

  // Should fail in normal mode
  int result1 = http_parser_parse_url(url, strlen(url), 0, &u1);
  assert(result1 != 0);

  // Should succeed in CONNECT mode
  int result2 = http_parser_parse_url(url, strlen(url), 1, &u2);
  assert(result2 == 0);

  TEST_PASS();
}

/* ============================================
 * Negative Tests - Invalid URLs
 * ============================================ */

void test_invalid_empty_string() {
  TEST_START("Invalid: empty string");
  const char *url = "";
  struct http_parser_url u = { 0 };

  int result = http_parser_parse_url(url, strlen(url), 0, &u);
  assert(result != 0);

  TEST_PASS();
}

void test_invalid_spaces_in_hostname() {
  TEST_START("Invalid: spaces in hostname");
  const char *url = "http://exa mple.com/path";
  struct http_parser_url u = { 0 };

  int result = http_parser_parse_url(url, strlen(url), 0, &u);
  assert(result != 0);

  TEST_PASS();
}

void test_invalid_port_with_letters() {
  TEST_START("Invalid: port with letters");
  const char *url = "http://example.com:80abc/path";
  struct http_parser_url u = { 0 };

  int result = http_parser_parse_url(url, strlen(url), 0, &u);
  assert(result != 0);

  TEST_PASS();
}

void test_invalid_port_out_of_range() {
  TEST_START("Invalid: port out of range (70000)");
  const char *url = "http://example.com:70000/path";
  struct http_parser_url u = { 0 };

  int result = http_parser_parse_url(url, strlen(url), 0, &u);
  assert(result != 0);

  TEST_PASS();
}

void test_invalid_missing_host() {
  TEST_START("Invalid: missing host after schema");
  const char *url = "http:///path";
  struct http_parser_url u = { 0 };

  int result = http_parser_parse_url(url, strlen(url), 0, &u);
  assert(result != 0);

  TEST_PASS();
}

void test_invalid_ipv6_unclosed() {
  TEST_START("Invalid: unclosed IPv6 bracket");
  const char *url = "http://[::1/path";
  struct http_parser_url u = { 0 };

  int result = http_parser_parse_url(url, strlen(url), 0, &u);
  assert(result != 0);

  TEST_PASS();
}

void test_invalid_double_at() {
  TEST_START("Invalid: double @ in auth");
  const char *url = "http://user@@example.com/path";
  struct http_parser_url u = { 0 };

  int result = http_parser_parse_url(url, strlen(url), 0, &u);
  assert(result != 0);

  TEST_PASS();
}

void test_invalid_control_characters() {
  TEST_START("Invalid: control characters in path");
  const char *url = "http://example.com/\npath";
  struct http_parser_url u = { 0 };

  int result = http_parser_parse_url(url, strlen(url), 0, &u);
  assert(result != 0);

  TEST_PASS();
}

void test_invalid_only_schema() {
  TEST_START("Invalid: only schema");
  const char *url = "http:";
  struct http_parser_url u = { 0 };

  int result = http_parser_parse_url(url, strlen(url), 0, &u);
  assert(result != 0);

  TEST_PASS();
}

void test_invalid_only_schema_and_slashes() {
  TEST_START("Invalid: only schema and slashes");
  const char *url = "http://";
  struct http_parser_url u = { 0 };

  int result = http_parser_parse_url(url, strlen(url), 0, &u);
  assert(result != 0);

  TEST_PASS();
}

void test_invalid_bad_schema() {
  TEST_START("Invalid: bad schema format");
  const char *url = "http:/path";
  struct http_parser_url u = { 0 };

  int result = http_parser_parse_url(url, strlen(url), 0, &u);
  assert(result != 0);

  TEST_PASS();
}

/* ============================================
 * Edge Case Tests
 * ============================================ */

void test_edge_very_long_url() {
  TEST_START("Edge case: very long URL");
  char url[2048];
  strcpy(url, "http://example.com/");
  for (int i = 0; i < 1000; i++) {
    strcat(url, "a");
  }
  
  struct http_parser_url u = { 0 };
  int result = http_parser_parse_url(url, strlen(url), 0, &u);
  assert(result == 0);
  assert(u.field_data[UF_PATH].len == 1001); // 1000 'a's + '/'

  TEST_PASS();
}

void test_edge_max_port() {
  TEST_START("Edge case: maximum port number (65535)");
  const char *url = "http://example.com:65535/path";
  struct http_parser_url u = { 0 };

  int result = http_parser_parse_url(url, strlen(url), 0, &u);
  assert(result == 0);
  assert(check_field(url, &u, UF_PORT, "65535"));
  assert(u.port == 65535);

  TEST_PASS();
}

void test_edge_port_zero() {
  TEST_START("Edge case: port 0");
  const char *url = "http://example.com:0/path";
  struct http_parser_url u = { 0 };

  int result = http_parser_parse_url(url, strlen(url), 0, &u);
  assert(result == 0);
  assert(check_field(url, &u, UF_PORT, "0"));
  assert(u.port == 0);

  TEST_PASS();
}

void test_edge_mixed_case_scheme() {
  TEST_START("Edge case: mixed case scheme");
  const char *url = "HTTP://example.com/path";
  struct http_parser_url u = { 0 };

  int result = http_parser_parse_url(url, strlen(url), 0, &u);
  assert(result == 0);
  // Schema should be parsed (case may vary by implementation)
  assert(u.field_set & (1 << UF_SCHEMA));

  TEST_PASS();
}

void test_edge_international_domain() {
  TEST_START("Edge case: international domain name");
  const char *url = "http://例子.测试/path";
  struct http_parser_url u = { 0 };

  int result = http_parser_parse_url(url, strlen(url), 0, &u);
  // IDN support depends on implementation
  if (result == 0) {
    printf("  (IDN supported)\n");
    assert(check_field(url, &u, UF_HOST, "例子.测试"));
  } else {
    printf("  (IDN not supported - acceptable)\n");
  }

  TEST_PASS();
}

void test_edge_plus_in_query() {
  TEST_START("Edge case: plus sign in query");
  const char *url = "/search?q=c%2B%2B";
  struct http_parser_url u = { 0 };

  int result = http_parser_parse_url(url, strlen(url), 0, &u);
  assert(result == 0);
  assert(check_field(url, &u, UF_QUERY, "q=c%2B%2B"));

  TEST_PASS();
}

void test_edge_state_isolation() {
  TEST_START("Edge case: state isolation between calls");
  struct http_parser_url u1 = { 0 };
  struct http_parser_url u2 = { 0 };
  
  const char *url1 = "http://example.com/path1";
  const char *url2 = "/path2";

  int result1 = http_parser_parse_url(url1, strlen(url1), 0, &u1);
  int result2 = http_parser_parse_url(url2, strlen(url2), 0, &u2);

  assert(result1 == 0);
  assert(result2 == 0);
  
  assert(check_field(url1, &u1, UF_HOST, "example.com"));
  assert(check_field(url1, &u1, UF_PATH, "/path1"));
  assert(check_field(url2, &u2, UF_PATH, "/path2"));
  assert(check_field(url2, &u2, UF_HOST, NULL));

  TEST_PASS();
}

void test_edge_root_path() {
  TEST_START("Edge case: root path only");
  const char *url = "http://example.com/";
  struct http_parser_url u = { 0 };

  int result = http_parser_parse_url(url, strlen(url), 0, &u);
  assert(result == 0);
  assert(check_field(url, &u, UF_SCHEMA, "http"));
  assert(check_field(url, &u, UF_HOST, "example.com"));
  assert(check_field(url, &u, UF_PATH, "/"));

  TEST_PASS();
}

void test_edge_query_with_question() {
  TEST_START("Edge case: query string with embedded ?");
  const char *url = "/path?query=value?extra=stuff";
  struct http_parser_url u = { 0 };

  int result = http_parser_parse_url(url, strlen(url), 0, &u);
  assert(result == 0);
  assert(check_field(url, &u, UF_PATH, "/path"));
  assert(check_field(url, &u, UF_QUERY, "query=value?extra=stuff"));

  TEST_PASS();
}

void test_edge_fragment_with_special() {
  TEST_START("Edge case: fragment with special characters");
  const char *url = "/path#fragment?with#special";
  struct http_parser_url u = { 0 };

  int result = http_parser_parse_url(url, strlen(url), 0, &u);
  assert(result == 0);
  assert(check_field(url, &u, UF_PATH, "/path"));
  assert(check_field(url, &u, UF_FRAGMENT, "fragment?with#special"));

  TEST_PASS();
}

/* ============================================
 * Additional protocol tests
 * ============================================ */

void test_ftp_protocol() {
  TEST_START("FTP protocol URL");
  const char *url = "ftp://example.com/file.txt";
  struct http_parser_url u = { 0 };

  int result = http_parser_parse_url(url, strlen(url), 0, &u);
  assert(result == 0);
  assert(check_field(url, &u, UF_SCHEMA, "ftp"));
  assert(check_field(url, &u, UF_HOST, "example.com"));
  assert(check_field(url, &u, UF_PATH, "/file.txt"));

  TEST_PASS();
}

void test_ws_protocol() {
  TEST_START("WebSocket protocol URL");
  const char *url = "ws://example.com/chat";
  struct http_parser_url u = { 0 };

  int result = http_parser_parse_url(url, strlen(url), 0, &u);
  assert(result == 0);
  assert(check_field(url, &u, UF_SCHEMA, "ws"));
  assert(check_field(url, &u, UF_HOST, "example.com"));
  assert(check_field(url, &u, UF_PATH, "/chat"));

  TEST_PASS();
}

void test_https_api_url() {
  TEST_START("HTTPS API URL with query params");
  const char *url = "https://api.example.com/v1/users?page=1&limit=10";
  struct http_parser_url u = { 0 };

  int result = http_parser_parse_url(url, strlen(url), 0, &u);
  assert(result == 0);
  assert(check_field(url, &u, UF_SCHEMA, "https"));
  assert(check_field(url, &u, UF_HOST, "api.example.com"));
  assert(check_field(url, &u, UF_PATH, "/v1/users"));
  assert(check_field(url, &u, UF_QUERY, "page=1&limit=10"));

  TEST_PASS();
}

/* ============================================
 * Main Test Runner
 * ============================================ */

int main() {
  printf("\n");
  printf("=====================================\n");
  printf("  Comprehensive llurl Test Suite\n");
  printf("=====================================\n\n");

  /* Positive Tests */
  printf("*** POSITIVE TESTS - Valid URLs ***\n\n");
  test_basic_https_url();
  test_url_without_auth_and_port();
  test_url_with_only_host();
  test_relative_url();
  test_url_with_ipv4();
  test_url_with_ipv6();
  test_url_with_ipv6_link_local();
  test_url_with_encoded_characters();
  test_url_with_special_query_chars();
  test_url_with_asterisk();
  test_url_with_dot_in_hostname();
  test_url_with_underscore_in_hostname();
  test_url_with_multiple_query_params();
  test_url_with_empty_query();
  test_url_with_empty_fragment();
  test_url_with_only_fragment();
  test_url_with_only_query();
  test_protocol_relative_url_with_host();
  test_protocol_relative_url_with_path();
  test_protocol_relative_url_with_port();

  /* CONNECT Mode Tests */
  printf("\n*** CONNECT MODE TESTS ***\n\n");
  test_connect_host_port();
  test_connect_reject_path();
  test_connect_reject_query();
  test_connect_reject_no_port();
  test_connect_ipv6();
  test_connect_ipv6_no_port();
  test_connect_vs_normal_mode();

  /* Negative Tests */
  printf("\n*** NEGATIVE TESTS - Invalid URLs ***\n\n");
  test_invalid_empty_string();
  test_invalid_spaces_in_hostname();
  test_invalid_port_with_letters();
  test_invalid_port_out_of_range();
  test_invalid_missing_host();
  test_invalid_ipv6_unclosed();
  test_invalid_double_at();
  test_invalid_control_characters();
  test_invalid_only_schema();
  test_invalid_only_schema_and_slashes();
  test_invalid_bad_schema();

  /* Edge Case Tests */
  printf("\n*** EDGE CASE TESTS ***\n\n");
  test_edge_very_long_url();
  test_edge_max_port();
  test_edge_port_zero();
  test_edge_mixed_case_scheme();
  test_edge_international_domain();
  test_edge_plus_in_query();
  test_edge_state_isolation();
  test_edge_root_path();
  test_edge_query_with_question();
  test_edge_fragment_with_special();

  /* Protocol Tests */
  printf("\n*** ADDITIONAL PROTOCOL TESTS ***\n\n");
  test_ftp_protocol();
  test_ws_protocol();
  test_https_api_url();

  /* Summary */
  printf("\n=====================================\n");
  printf("  TEST SUMMARY\n");
  printf("=====================================\n");
  printf("Total tests: %d\n", test_count);
  printf("Passed:      %d\n", test_passed);
  printf("Failed:      %d\n", test_count - test_passed);
  
  if (test_passed == test_count) {
    printf("\n✓ ALL TESTS PASSED!\n");
  } else {
    printf("\n✗ SOME TESTS FAILED!\n");
  }
  printf("=====================================\n\n");

  return (test_passed == test_count) ? 0 : 1;
}
