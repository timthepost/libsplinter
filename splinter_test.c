// TAP-compatible unit tests
// Feel free to use the macro in whatever (public domain)
// Splinter's license is Apache 2
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include "splinter.h"

#ifdef HAVE_VALGRIND_H
#include <valgrind/valgrind.h>
#endif

static int total = 0;
static int passed = 0;

#define TEST(name, expr) do { \
  total++; \
  if (expr) { \
    passed++; \
    printf("ok %d - %s\n", total, name); \
  } else { \
    printf("not ok %d - %s\n", total, name); \
  } \
} while (0)

int main(void) {
  printf("1..17\n");

  // Test 1: Create new splinter store
  TEST("create splinter store", splinter_create_or_open("splinter_stress", 1000, 4096) == 0);

  // Test 2: Basic set operation
  const char *test_key = "test_key";
  const char *test_value = "hello world";
  TEST("set key-value pair", splinter_set(test_key, test_value, strlen(test_value)) == 0);

  // Test 3: Basic get operation
  char buf[256];
  size_t out_sz;
  TEST("get key-value pair", splinter_get(test_key, buf, sizeof(buf), &out_sz) == 0);

  // Test 4: Verify retrieved value matches
  buf[out_sz] = '\0'; // Null-terminate for comparison
  TEST("retrieved value matches", strcmp(buf, test_value) == 0);

  // Test 5: Verify retrieved size is correct
  TEST("retrieved size is correct", out_sz == strlen(test_value));

  // Test 6: Get with NULL buffer to query size
  size_t query_sz;
  TEST("query size with NULL buffer", splinter_get(test_key, NULL, 0, &query_sz) == 0);

  // Test 7: Verify queried size matches
  TEST("queried size matches", query_sz == strlen(test_value));

  // Test 8: Update existing key
  const char *new_value = "updated value";
  TEST("update existing key", splinter_set(test_key, new_value, strlen(new_value)) == 0);

  // Test 9: Verify updated value
  TEST("get updated value", splinter_get(test_key, buf, sizeof(buf), &out_sz) == 0);
  buf[out_sz] = '\0';
  TEST("updated value is correct", strcmp(buf, new_value) == 0);

  // Test 10: Set multiple keys
  TEST("set second key", splinter_set("key2", "value2", 6) == 0);
  TEST("set third key", splinter_set("key3", "value3", 6) == 0);

  // Test 11: List keys
  char *key_list[10];
  size_t key_count;
  TEST("list keys", splinter_list(key_list, 10, &key_count) == 0);

  // Test 12: Verify key count
  TEST("correct number of keys", key_count == 3);

  // Test 13: Unset a key
  TEST("unset key", splinter_unset("key2") >= 0);

  // Test 14: Auto vacuum functions
  int original_av = splinter_get_av();
  TEST("set auto vacuum mode", splinter_set_av(0) == 0);
  TEST("get auto vacuum mode", splinter_get_av() == 0);
  splinter_set_av((uint32_t) original_av);

  // TODO: Need to test flipping auto vacuum on / off during 
  // between / after / again after read write ops. Not urgent
  // at all, trivial really, maybe even unnecessary? Noting it
  // for diligence and completeness.

  // Cleanup
  splinter_close();

#ifdef HAVE_VALGRIND_H
  if (RUNNING_ON_VALGRIND) {
    printf("\n** Valgrind Detected. Thank you for your diligence! **\n\n");
  }
#endif // HAVE_VALGRIND_H
  
  return (passed == total) ? 0 : 1;
}