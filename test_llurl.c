#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "llurl.h"

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

/* Test cases */
void test_absolute_url() {
  printf("=== Test: Absolute URL ===\n");
  const char *url = "http://example.com/path?query=value#fragment";
  struct http_parser_url u;
  
  int result = http_parser_parse_url(url, strlen(url), 0, &u);
  assert(result == 0);
  
  print_url_result(url, &u);
  
  assert(check_field(url, &u, UF_SCHEMA, "http"));
  assert(check_field(url, &u, UF_HOST, "example.com"));
  assert(check_field(url, &u, UF_PATH, "/path"));
  assert(check_field(url, &u, UF_QUERY, "query=value"));
  assert(check_field(url, &u, UF_FRAGMENT, "fragment"));
  
  printf("✓ Absolute URL test passed\n\n");
}

void test_absolute_url_with_port() {
  printf("=== Test: Absolute URL with port ===\n");
  const char *url = "http://example.com:8080/path?query=value#fragment";
  struct http_parser_url u;
  
  int result = http_parser_parse_url(url, strlen(url), 0, &u);
  assert(result == 0);
  
  print_url_result(url, &u);
  
  assert(check_field(url, &u, UF_SCHEMA, "http"));
  assert(check_field(url, &u, UF_HOST, "example.com"));
  assert(check_field(url, &u, UF_PORT, "8080"));
  assert(u.port == 8080);
  assert(check_field(url, &u, UF_PATH, "/path"));
  assert(check_field(url, &u, UF_QUERY, "query=value"));
  assert(check_field(url, &u, UF_FRAGMENT, "fragment"));
  
  printf("✓ Absolute URL with port test passed\n\n");
}

void test_relative_url() {
  printf("=== Test: Relative URL ===\n");
  const char *url = "/path?query=value#fragment";
  struct http_parser_url u;
  
  int result = http_parser_parse_url(url, strlen(url), 0, &u);
  assert(result == 0);
  
  print_url_result(url, &u);
  
  assert(check_field(url, &u, UF_SCHEMA, NULL));
  assert(check_field(url, &u, UF_HOST, NULL));
  assert(check_field(url, &u, UF_PATH, "/path"));
  assert(check_field(url, &u, UF_QUERY, "query=value"));
  assert(check_field(url, &u, UF_FRAGMENT, "fragment"));
  
  printf("✓ Relative URL test passed\n\n");
}

void test_connect_request() {
  printf("=== Test: CONNECT request ===\n");
  const char *url = "example.com:443";
  struct http_parser_url u;
  
  int result = http_parser_parse_url(url, strlen(url), 1, &u);
  assert(result == 0);
  
  print_url_result(url, &u);
  
  assert(check_field(url, &u, UF_HOST, "example.com"));
  assert(check_field(url, &u, UF_PORT, "443"));
  assert(u.port == 443);
  
  printf("✓ CONNECT request test passed\n\n");
}

void test_ipv6_url() {
  printf("=== Test: IPv6 URL ===\n");
  const char *url = "http://[1:2::3:4]/path";
  struct http_parser_url u;
  
  int result = http_parser_parse_url(url, strlen(url), 0, &u);
  assert(result == 0);
  
  print_url_result(url, &u);
  
  assert(check_field(url, &u, UF_SCHEMA, "http"));
  assert(check_field(url, &u, UF_HOST, "[1:2::3:4]"));
  assert(check_field(url, &u, UF_PATH, "/path"));
  
  printf("✓ IPv6 URL test passed\n\n");
}

void test_ipv6_url_with_port() {
  printf("=== Test: IPv6 URL with port ===\n");
  const char *url = "http://[1:2::3:4]:8080/path";
  struct http_parser_url u;
  
  int result = http_parser_parse_url(url, strlen(url), 0, &u);
  assert(result == 0);
  
  print_url_result(url, &u);
  
  assert(check_field(url, &u, UF_SCHEMA, "http"));
  assert(check_field(url, &u, UF_HOST, "[1:2::3:4]"));
  assert(check_field(url, &u, UF_PORT, "8080"));
  assert(u.port == 8080);
  assert(check_field(url, &u, UF_PATH, "/path"));
  
  printf("✓ IPv6 URL with port test passed\n\n");
}

void test_userinfo() {
  printf("=== Test: URL with userinfo ===\n");
  const char *url = "http://user:pass@example.com/path";
  struct http_parser_url u;
  
  int result = http_parser_parse_url(url, strlen(url), 0, &u);
  assert(result == 0);
  
  print_url_result(url, &u);
  
  assert(check_field(url, &u, UF_SCHEMA, "http"));
  assert(check_field(url, &u, UF_USERINFO, "user:pass"));
  assert(check_field(url, &u, UF_HOST, "example.com"));
  assert(check_field(url, &u, UF_PATH, "/path"));
  
  printf("✓ URL with userinfo test passed\n\n");
}

void test_query_with_question_mark() {
  printf("=== Test: Query string with ? ===\n");
  const char *url = "/path?query=value?extra=stuff";
  struct http_parser_url u;
  
  int result = http_parser_parse_url(url, strlen(url), 0, &u);
  assert(result == 0);
  
  print_url_result(url, &u);
  
  assert(check_field(url, &u, UF_PATH, "/path"));
  assert(check_field(url, &u, UF_QUERY, "query=value?extra=stuff"));
  
  printf("✓ Query string with ? test passed\n\n");
}

void test_fragment_with_special_chars() {
  printf("=== Test: Fragment with special characters ===\n");
  const char *url = "/path#fragment?with#special";
  struct http_parser_url u;
  
  int result = http_parser_parse_url(url, strlen(url), 0, &u);
  assert(result == 0);
  
  print_url_result(url, &u);
  
  assert(check_field(url, &u, UF_PATH, "/path"));
  assert(check_field(url, &u, UF_FRAGMENT, "fragment?with#special"));
  
  printf("✓ Fragment with special characters test passed\n\n");
}

void test_root_path() {
  printf("=== Test: Root path ===\n");
  const char *url = "http://example.com/";
  struct http_parser_url u;
  
  int result = http_parser_parse_url(url, strlen(url), 0, &u);
  assert(result == 0);
  
  print_url_result(url, &u);
  
  assert(check_field(url, &u, UF_SCHEMA, "http"));
  assert(check_field(url, &u, UF_HOST, "example.com"));
  assert(check_field(url, &u, UF_PATH, "/"));
  
  printf("✓ Root path test passed\n\n");
}

void test_invalid_url_empty() {
  printf("=== Test: Invalid URL (empty) ===\n");
  const char *url = "";
  struct http_parser_url u;
  
  int result = http_parser_parse_url(url, strlen(url), 0, &u);
  assert(result != 0);
  
  printf("✓ Empty URL correctly rejected\n\n");
}

void test_invalid_url_bad_schema() {
  printf("=== Test: Invalid URL (bad schema) ===\n");
  const char *url = "http:/path";
  struct http_parser_url u;
  
  int result = http_parser_parse_url(url, strlen(url), 0, &u);
  assert(result != 0);
  
  printf("✓ Bad schema correctly rejected\n\n");
}

int main() {
  printf("Running llurl tests...\n\n");
  
  test_absolute_url();
  test_absolute_url_with_port();
  test_relative_url();
  test_connect_request();
  test_ipv6_url();
  test_ipv6_url_with_port();
  test_userinfo();
  test_query_with_question_mark();
  test_fragment_with_special_chars();
  test_root_path();
  test_invalid_url_empty();
  test_invalid_url_bad_schema();
  
  printf("===================================\n");
  printf("All tests passed! ✓\n");
  printf("===================================\n");
  
  return 0;
}
