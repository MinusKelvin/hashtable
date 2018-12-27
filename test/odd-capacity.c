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

size_t bad_hasher(const void *v) {
    return 0;
}

#define TEST(x) if (!(x)) return __LINE__

int main() {
    HashTable *table = ht_new(fnv1a_hash_string, string_equals);
    ht_ensure_capacity(table, 3);

    TEST(string_equals(ht_insert(table, "foo", "1"), NULL));
    TEST(string_equals(ht_insert(table, "bar", "2"), NULL));
    TEST(string_equals(ht_insert(table, "baz", "3"), NULL));

    TEST(string_equals(ht_get(table, "empty"), NULL));

    TEST(string_equals(ht_insert(table, "foobar", "4"), NULL));

    TEST(ht_size(table) == 4);
    TEST(string_equals(ht_get(table, "foo"), "1"));
    TEST(string_equals(ht_get(table, "bar"), "2"));
    TEST(string_equals(ht_get(table, "baz"), "3"));
    TEST(string_equals(ht_get(table, "foobar"), "4"));

    ht_ensure_capacity(table, 17);
    
    TEST(ht_size(table) == 4);
    TEST(string_equals(ht_get(table, "foo"), "1"));
    TEST(string_equals(ht_get(table, "bar"), "2"));
    TEST(string_equals(ht_get(table, "baz"), "3"));
    TEST(string_equals(ht_get(table, "foobar"), "4"));

    ht_free(table);

    table = ht_new(bad_hasher, string_equals);
    ht_ensure_capacity(table, 3);

    TEST(string_equals(ht_insert(table, "foo", "1"), NULL));
    TEST(string_equals(ht_insert(table, "bar", "2"), NULL));
    TEST(string_equals(ht_insert(table, "baz", "3"), NULL));

    TEST(string_equals(ht_get(table, "empty"), NULL));

    ht_free(table);
}