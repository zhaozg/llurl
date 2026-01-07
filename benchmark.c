#include <stdio.h>
#include <string.h>
#include <time.h>
#include "llurl.h"

/* Simple benchmark program for llurl */

#define ITERATIONS 1000000

/* Get current time in seconds */
static double get_time() {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ts.tv_sec + ts.tv_nsec / 1000000000.0;
}

/* Benchmark a single URL */
void benchmark_url(const char *name, const char *url, int is_connect) {
  struct http_parser_url u;
  int result = 0;
  int i, success = 0;
  double start, end, elapsed;

  printf("Benchmarking: %s\n", name);
  printf("  URL: %s\n", url);
  printf("  Iterations: %d\n", ITERATIONS);

  /* Warm up */
  double warmup_start = get_time();
  for (i = 0; i < 1000; i++) {
    memset(&u, 0, sizeof(u));
    http_parser_parse_url(url, strlen(url), is_connect, &u);
  }
  double warmup_end = get_time();
  printf("  Warmup time: %.0f nanoseconds\n", (warmup_end - warmup_start) * 1e9);

  /* Actual benchmark */
  start = get_time();
  for (i = 0; i < ITERATIONS; i++) {
    memset(&u, 0, sizeof(u));
    result = http_parser_parse_url(url, strlen(url), is_connect, &u);
    success += (result == 0);
  }
  end = get_time();

  elapsed = end - start;

  if (success != ITERATIONS) {
    printf("  ❌ Error: Failed to parse URL (%d/%d)\n\n", success, ITERATIONS);
    return;
  }

  printf("  ✓ Success\n");
  printf("  Total time: %.0f nanoseconds\n", elapsed * 1e9);
  printf("  Time per parse: %.3f nanoseconds\n", (elapsed / ITERATIONS) * 1e9);
  printf("  Throughput: %.2f parses/second\n\n", ITERATIONS / elapsed);
}

int main() {
  printf("=================================\n");
  printf("llurl Performance Benchmark\n");
  printf("=================================\n\n");

  /* Simple URLs */
  benchmark_url("Simple relative URL", "/path", 0);
  benchmark_url("Simple absolute URL", "http://example.com/", 0);

  /* Complex URLs */
  benchmark_url("Complete URL",
                "https://user:pass@example.com:8443/api/v1/users?id=123&name=test#section",
                0);

  /* Query-heavy URLs */
  benchmark_url("Query-heavy URL",
                "https://api.example.com/search?q=test&format=json&page=1&limit=100&sort=desc&filter=active",
                0);

  /* IPv6 URLs */
  benchmark_url("IPv6 URL",
                "http://[2001:db8::1]:8080/path?query=value",
                0);

  /* CONNECT requests */
  benchmark_url("CONNECT request",
                "example.com:443",
                1);

  printf("=================================\n");
  printf("Benchmark Complete\n");
  printf("=================================\n");

  return 0;
}
