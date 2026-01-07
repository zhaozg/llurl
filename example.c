#include <stdio.h>
#include <string.h>
#include "llurl.h"

/* Example program demonstrating llurl usage */

void print_url_component(const char *url, const struct http_parser_url *u,
                        enum http_parser_url_fields field, const char *name) {
  if (u->field_set & (1 << field)) {
    printf("  %-10s: %.*s\n", name,
           u->field_data[field].len,
           url + u->field_data[field].off);
  }
}

void parse_and_print(const char *url, int is_connect) {
  struct http_parser_url u;
  
  printf("Parsing: %s\n", url);
  printf("Type: %s\n", is_connect ? "CONNECT" : "Normal");
  
  http_parser_url_init(&u);
  
  int result = http_parser_parse_url(url, strlen(url), is_connect, &u);
  
  if (result != 0) {
    printf("  ❌ Failed to parse URL\n\n");
    return;
  }
  
  printf("  ✓ Successfully parsed\n");
  
  print_url_component(url, &u, UF_SCHEMA, "Schema");
  print_url_component(url, &u, UF_USERINFO, "Userinfo");
  print_url_component(url, &u, UF_HOST, "Host");
  
  if (u.field_set & (1 << UF_PORT)) {
    printf("  %-10s: %u\n", "Port", u.port);
  }
  
  print_url_component(url, &u, UF_PATH, "Path");
  print_url_component(url, &u, UF_QUERY, "Query");
  print_url_component(url, &u, UF_FRAGMENT, "Fragment");
  
  printf("\n");
}

int main() {
  printf("=================================\n");
  printf("llurl - Fast URL Parser Demo\n");
  printf("=================================\n\n");
  
  /* Example 1: Complete URL */
  parse_and_print("https://user:pass@example.com:8443/api/v1/users?id=123&name=test#section", 0);
  
  /* Example 2: Simple URL */
  parse_and_print("http://example.com/", 0);
  
  /* Example 3: Relative URL */
  parse_and_print("/path/to/resource?query=value", 0);
  
  /* Example 4: CONNECT request */
  parse_and_print("example.com:443", 1);
  
  /* Example 5: IPv6 URL */
  parse_and_print("http://[2001:db8::1]:8080/path", 0);
  
  /* Example 6: URL with query only */
  parse_and_print("/?search=test", 0);
  
  /* Example 7: URL with fragment only */
  parse_and_print("/page#anchor", 0);
  
  /* Example 8: Complex query string */
  parse_and_print("https://api.example.com/search?q=hello+world&format=json&page=1", 0);
  
  return 0;
}
