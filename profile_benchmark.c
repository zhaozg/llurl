#include <stdio.h>
#include <string.h>
#include <time.h>
#include "llurl.h"

/* Profiling benchmark - runs fewer iterations but exercises all code paths */

#define ITERATIONS 100000

/* Get current time in seconds */
static double get_time() {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ts.tv_sec + ts.tv_nsec / 1000000000.0;
}

int main() {
  struct http_parser_url u;
  int i;
  double start, end;
  
  const char *urls[] = {
    "/path",
    "http://example.com/",
    "https://user:pass@example.com:8443/api/v1/users?id=123&name=test#section",
    "https://api.example.com/search?q=test&format=json&page=1&limit=100&sort=desc&filter=active",
    "http://[2001:db8::1]:8080/path?query=value",
    "example.com:443",
    "//example.com/path",
    "http://192.168.1.1:8080/api",
    "/path/to/resource?key1=value1&key2=value2&key3=value3#anchor",
    "ftp://ftp.example.com/files/document.pdf"
  };
  
  int num_urls = sizeof(urls) / sizeof(urls[0]);
  int total = 0;
  
  printf("Starting profiling benchmark with %d iterations per URL...\n", ITERATIONS);
  printf("Total operations: %d\n\n", ITERATIONS * num_urls);
  
  start = get_time();
  
  for (i = 0; i < ITERATIONS; i++) {
    for (int j = 0; j < num_urls; j++) {
      const char *url = urls[j];
      int is_connect = (j == 5) ? 1 : 0;  // CONNECT mode for example.com:443
      
      memset(&u, 0, sizeof(u));
      int result = http_parser_parse_url(url, strlen(url), is_connect, &u);
      total += (result == 0);
    }
  }
  
  end = get_time();
  
  printf("Completed %d successful parses in %.3f seconds\n", total, end - start);
  printf("Throughput: %.2f parses/second\n", total / (end - start));
  
  return 0;
}
