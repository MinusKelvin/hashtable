#include <stdlib.h>
#include <string.h>

#define HASHTABLE_IMPL
#include "../hashtable.h"

size_t fnv1a_hash_string(const void *_str) {
    const char *str = _str;
    size_t hash = 14695981039346656037u;
    for (; *str; str++) {
        hash ^= *str & 0xFF;
        hash *= 1099511628211;
    }
    return hash;
}

int string_equals(const void *a, const void *b) {
    if (!a || !b) return a == b;
    return !strcmp(a, b);
}

#define TEST(x) if (!(x)) return __LINE__

int main() {
    HashTable *table = ht_new(fnv1a_hash_string, string_equals);

    TEST(ht_insert(table, "foo", "bar") == NULL);
    TEST(ht_size(table) == 1);

    TEST(ht_insert(table, "bar", "foo") == NULL);
    TEST(ht_size(table) == 2);

    TEST(string_equals(ht_get(table, "bar"), "foo"));
    TEST(string_equals(ht_get(table, "foo"), "bar"));
    TEST(ht_get(table, "baz") == NULL);

    TEST(ht_remove(table, "foo") != NULL);
    TEST(ht_size(table) == 1);
    TEST(ht_get(table, "foo") == NULL);

    TEST(string_equals(ht_insert(table, "bar", "baz"), "foo"));
    TEST(string_equals(ht_get(table, "bar"), "baz"));
    TEST(ht_size(table) == 1);
    
    ht_free(table);
}