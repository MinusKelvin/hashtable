#ifndef _H_HASHTABLE_
#define _H_HASHTABLE_

#include <stdlib.h>
#include <stdio.h>

typedef struct HashTable HashTable;
typedef struct HT_Entry {
    const void *key;
    void *value;
} HT_Entry;
typedef size_t (*HT_HashFunction)(const void*);
typedef int (*HT_EqualityFunction)(const void*, const void*);

HashTable *ht_new(HT_HashFunction hasher, HT_EqualityFunction eq);
HashTable *ht_clone(HashTable *table);
void ht_free(HashTable *table);
void ht_ensure_capacity(HashTable *table, size_t minimum_capacity);

void *ht_get(HashTable *table, const void *key);
void *ht_remove(HashTable *table, const void *key);
void *ht_insert(HashTable *table, const void *key, void *value);
size_t ht_size(HashTable *table);

HT_Entry *ht_entries(HashTable *table);

#endif // _H_HASHTABLE_

#ifdef HASHTABLE_IMPL

#include <assert.h>
#include <stdlib.h>

#define DEFAULT_CAPACITY 16

typedef struct _ht_entry {
    size_t cached_hash;
    const void *key;
    void *value;
} _ht_entry;

struct HashTable {
    size_t capacity;
    size_t count;
    HT_HashFunction hasher;
    HT_EqualityFunction eq;
    // Null key is handled specially since normally null keys indicate an empty bucket
    int null_present;
    void *null;
    _ht_entry *table;
};

static size_t _ht_identity_hasher(const void *ptr) {
    return (size_t) ptr;
}

static int _ht_identity_eq(const void *a, const void *b) {
    return a == b;
}

HashTable *ht_new(HT_HashFunction hasher, HT_EqualityFunction eq) {
    HashTable *new = calloc(1, sizeof(HashTable));
    if (!new) {
        fprintf(stderr, "Out of Memory\n");
        abort();
    }
    new->hasher = hasher ? hasher : _ht_identity_hasher;
    new->eq = eq ? eq : _ht_identity_eq;
    return new;
}

HashTable *ht_clone(HashTable *table) {
    HashTable *new = malloc(sizeof(HashTable));
    if (!new) {
        fprintf(stderr, "Out of Memory\n");
        abort();
    }
    memcpy(new, table, sizeof(HashTable));
    if (new->table) {
        new->table = malloc(new->capacity * sizeof(_ht_entry));
        if (!new->table) {
            fprintf(stderr, "Out of Memory\n");
            abort();
        }
        memcpy(new->table, table->table, new->capacity * sizeof(_ht_entry));
    }
    return new;
}

void ht_free(HashTable *table) {
    if (table->table) {
        free(table->table);
    }
    free(table);
}

static inline size_t _ht_probe_distance(size_t hash, size_t index, size_t capacity) {
    size_t initial_bucket = hash % capacity;
    // Index wrapped from the end of the array to the beginning
    if (index < initial_bucket) {
        index += capacity;
    }
    return index - initial_bucket;
}

static void *_ht_insert(
    _ht_entry *table,
    size_t capacity,
    _ht_entry entry,
    HT_EqualityFunction eq
) {
    // For each bucket, starting at the preferred bucket
    for (size_t i = entry.cached_hash % capacity ;; i = (i + 1) % capacity) {
        _ht_entry *bucket = &table[i];
        if (!bucket->key) {
            // No key? Empty bucket!
            *bucket = entry;
            return NULL;
        } else if (
            bucket->key == entry.key ||
            bucket->cached_hash == entry.cached_hash && eq(bucket->key, entry.key)
        ) {
            // Key matches bucket; replace value
            void *old = bucket->value;
            bucket->value = entry.value;
            return old;
        } else if (_ht_probe_distance(bucket->cached_hash, i, capacity) < _ht_probe_distance(entry.cached_hash, i, capacity)) {
            // Robin hood bucket stealing; now we need to find a new place for the
            // entry whose bucket we just stole using the same strategy!
            _ht_entry tmp = *bucket;
            *bucket = entry;
            entry = tmp;
        }
    }
}

static void _ht_resize(HashTable *table, size_t new_capacity) {
    _ht_entry *new_table = calloc(new_capacity, sizeof(_ht_entry));
    if (!new_table) {
        fprintf(stderr, "Out of Memory\n");
        abort();
    }
    
    if (table->count != 0) {
        // Reinsert old entries into new backing memory.

        // Due to the invariants Robin Hood bucket stealing upholds,
        // there is a better way of doing this when we are simply
        // doubling the capacity but we're doing it the easy way for now.
        for (size_t i = 0; i < table->capacity; i++) {
            if (!table->table[i].key) continue;
            void *displaced = _ht_insert(
                new_table,
                new_capacity,
                table->table[i],
                table->eq
            );
            assert(!displaced);
        }
    }

    table->capacity = new_capacity;
    table->table = new_table;
}

void ht_ensure_capacity(HashTable *table, size_t minimum_capacity) {
    if (table->capacity >= minimum_capacity) return;
    _ht_resize(table, minimum_capacity);
}

void *ht_get(HashTable *table, const void *key) {
    // Null key is handled specially
    if (!key && table->null_present) return table->null;

    if (table->count == 0) return NULL;

    const size_t hash = table->hasher(key);
    // For each bucket, starting at the preferred bucket
    for (size_t i = hash % table->capacity ;; i = (i + 1) % table->capacity) {
        _ht_entry *bucket = &table->table[i];
        if (
            // Empty bucket terminates the list
            !bucket->key ||
            // Encountering a bucket that would have been stolen if the entry existed also terminates the list
            _ht_probe_distance(hash, i, table->capacity) > _ht_probe_distance(bucket->cached_hash, i, table->capacity)
        ) {
            return NULL;
        } else if (
            bucket->key == key ||
            bucket->cached_hash == hash && table->eq(bucket->key, key)
        ) {
            // Key matches; Found value
            return bucket->value;
        }
    }
}

void *ht_remove(HashTable *table, const void *key) {
    if (!key) {
        table->null_present = 0;
        void *old = table->null;
        table->null = NULL;
        return old;
    }

    if (table->count == 0) return NULL;

    const size_t hash = table->hasher(key);
    // For each bucket, starting at the preferred bucket
    for (size_t i = hash % table->capacity ;; i = (i + 1) % table->capacity) {
        _ht_entry *bucket = &table->table[i];
        if (
            // Empty bucket terminates the list
            !bucket->key ||
            // Encountering a bucket that would have been stolen if the entry existed also terminates the list
            _ht_probe_distance(hash, i, table->capacity) > _ht_probe_distance(bucket->cached_hash, i, table->capacity)
        ) {
            return NULL;
        } else if (
            bucket->key == key ||
            bucket->cached_hash == hash && table->eq(bucket->key, key)
        ) {
            // Key matches; To remove value, shift the rest of
            // the entries not in their preferred bucket down.
            void *old = bucket->value;
            for (
                size_t next = (i + 1) % table->capacity
                ;; // We use next like this because we could wrap
                i = next, next = (i + 1) % table->capacity
            ) {
                if (
                    !table->table[next].key ||
                    _ht_probe_distance(table->table[next].cached_hash, next, table->capacity) == 0
                ) break;
                table->table[i] = table->table[next];
            }
            table->table[i] = (_ht_entry) { 0, NULL, NULL };
            table->count--;
            return old;
        }
    }
}

void *ht_insert(HashTable *table, const void *key, void *value) {
    if (!key) {
        table->null_present = 1;
        void *old = table->null;
        table->null = value;
        return old;
    }
    if (table->capacity == 0) _ht_resize(table, DEFAULT_CAPACITY);

    // Resize when reaching 90% capacity
    // Doubling allows for amortized O(1) insertion
    if (10 * table->count >= 9 * table->capacity || table->count == table->capacity - 1)
        _ht_resize(table, table->capacity * 2);

    void *old = _ht_insert(table->table, table->capacity, (_ht_entry) {
        table->hasher(key), key, value
    }, table->eq);
    if (!old) table->count++;
    return old;
}

size_t ht_size(HashTable *table) {
    return table->count + table->null_present;
}

HT_Entry *ht_entries(HashTable *table) {
    size_t size = ht_size(table);
    HT_Entry *entries = malloc(size * sizeof(HT_Entry));
    size_t index = 0;
    for (size_t i = 0; i < table->capacity; i++) {
        if (table->table[i].key) {
            entries[index++] = (HT_Entry) {
                table->table[i].key,
                table->table[i].value
            };
        }
    }
    if (table->null_present) {
        entries[size - 1] = (HT_Entry) { NULL, table->null };
    }
    return entries;
}

#undef DEFAULT_CAPACITY

#endif // HASHTABLE_IMPL