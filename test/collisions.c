#include <stdlib.h>
#include <string.h>

#define HASHTABLE_IMPL
#include "../hashtable.h"

size_t radix_hash(const void *str) {
    return *((const char *) str) & 0xFF;
}

int string_equals(const void *a, const void *b) {
    if (!a || !b) return a == b;
    return !strcmp(a, b);
}

#define TEST(x) if (!(x)) exit(__LINE__)

void bar_baz_car(HashTable *table) {
    // | bar | baz | car | -> | baz | car | ___ |
    TEST(string_equals(ht_remove(table, "bar"), "1"));

    TEST(ht_get(table, "bar") == NULL);
    TEST(string_equals(ht_get(table, "car"), "2"));
    TEST(string_equals(ht_get(table, "baz"), "3"));
    TEST(ht_size(table) == 2);

    // | baz | car | ___ | -> | ___ | car | ___ |
    TEST(string_equals(ht_remove(table, "baz"), "3"));

    TEST(ht_get(table, "bar") == NULL);
    TEST(string_equals(ht_get(table, "car"), "2"));
    TEST(ht_get(table, "baz") == NULL);
    TEST(ht_size(table) == 1);

    // | ___ | car | ___ | -> | ___ | ___ | ___ |
    TEST(string_equals(ht_remove(table, "car"), "2"));

    TEST(ht_get(table, "bar") == NULL);
    TEST(ht_get(table, "car") == NULL);
    TEST(ht_get(table, "baz") == NULL);
    TEST(ht_size(table) == 0);

    ht_free(table);
}

void car_baz_bar(HashTable *table) {
    // | bar | baz | car | -> | bar | baz | ___ |
    TEST(string_equals(ht_remove(table, "car"), "2"));

    TEST(string_equals(ht_get(table, "bar"), "1"));
    TEST(ht_get(table, "car") == NULL);
    TEST(string_equals(ht_get(table, "baz"), "3"));
    TEST(ht_size(table) == 2);

    // | bar | baz | ___ | -> | bar | ___ | ___ |
    TEST(string_equals(ht_remove(table, "baz"), "3"));

    TEST(string_equals(ht_get(table, "bar"), "1"));
    TEST(ht_get(table, "car") == NULL);
    TEST(ht_get(table, "baz") == NULL);
    TEST(ht_size(table) == 1);

    // | bar | ___ | ___ | -> | ___ | ___ | ___ |
    TEST(string_equals(ht_remove(table, "bar"), "1"));

    TEST(ht_get(table, "bar") == NULL);
    TEST(ht_get(table, "car") == NULL);
    TEST(ht_get(table, "baz") == NULL);
    TEST(ht_size(table) == 0);

    ht_free(table);
}

void baz_bar_car(HashTable *table) {
    // | bar | baz | car | -> | bar | car | ___ |
    TEST(string_equals(ht_remove(table, "baz"), "3"));

    TEST(string_equals(ht_get(table, "bar"), "1"));
    TEST(string_equals(ht_get(table, "car"), "2"));
    TEST(ht_get(table, "baz") == NULL);
    TEST(ht_size(table) == 2);

    // | bar | car | ___ | -> | ___ | car | ___ |
    TEST(string_equals(ht_remove(table, "bar"), "1"));

    TEST(ht_get(table, "bar") == NULL);
    TEST(string_equals(ht_get(table, "car"), "2"));
    TEST(ht_get(table, "baz") == NULL);
    TEST(ht_size(table) == 1);

    // | ___ | car | ___ | -> | ___ | ___ | ___ |
    TEST(string_equals(ht_remove(table, "car"), "2"));

    TEST(ht_get(table, "bar") == NULL);
    TEST(ht_get(table, "car") == NULL);
    TEST(ht_get(table, "baz") == NULL);
    TEST(ht_size(table) == 0);

    ht_free(table);
}

int main() {
    HashTable *table = ht_new(radix_hash, string_equals);
    TEST(ht_insert(table, "bar", "1") == NULL);
    TEST(ht_insert(table, "car", "2") == NULL);
    TEST(ht_insert(table, "baz", "3") == NULL);

    TEST(string_equals(ht_get(table, "bar"), "1"));
    TEST(string_equals(ht_get(table, "car"), "2"));
    TEST(string_equals(ht_get(table, "baz"), "3"));
    TEST(ht_size(table) == 3);

    bar_baz_car(ht_clone(table));
    car_baz_bar(ht_clone(table));
    baz_bar_car(ht_clone(table));

    ht_free(table);
}