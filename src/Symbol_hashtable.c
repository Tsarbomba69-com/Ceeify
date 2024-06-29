#include "Symbol_hashtable.h"
#include "utils.h"

Symbol_HashTable Symbol_CreateHashTable(size_t capacity) {
    assert(capacity > 0 && "[ERROR]: Capacity should be greater than 0");
    Symbol_HashTable table = {0};
    table.capacity = capacity;
    Symbol_AllocateElementes(&table);
    table.size = 0;
    return table;
}

void Symbol_AllocateElementes(Symbol_HashTable *table) {
    table->buckets = AllocateContext(table->capacity * sizeof(char_Symbol_Pair *));
    if (table->buckets == NULL) {
        fprintf(stderr, "ERROR: Could not allocate memory for \"Symbol*\" table buckets");
        return;
    }

    for (size_t i = 0; i < table->capacity; ++i) {
        table->buckets[i] = AllocateContext(sizeof(char_Symbol_Pair));
        table->buckets[i]->key = NULL;
        table->buckets[i]->value = NULL;
        table->buckets[i]->occupied = false;
    }
}

size_t Symbol_Hash(const char *key) {
    size_t hash = 5381;
    char c;

    while ((c = *key++))
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}

void Symbol_Insert(Symbol_HashTable *table, char const *key, Symbol *value) {
    if ((float) (table->size + 1) / (float) table->capacity > LOAD_FACTOR_THRESHOLD) {
        size_t old_capacity = table->capacity;
        char_Symbol_Pair **old_table = table->buckets;
        table->capacity *= 2;
        Symbol_AllocateElementes(table);

        for (int i = 0; i < old_capacity; ++i) {
            if (old_table[i]->occupied) {
                Symbol_Insert(table, old_table[i]->key, old_table[i]->value);
            }
        }
    }

    size_t hash = Symbol_Hash(key) % table->capacity;
    while (table->buckets[hash]->occupied && strcmp(table->buckets[hash]->key, key) != 0) {
        hash = (hash + 1) % table->capacity; // Linear probing
    }

    if (!table->buckets[hash]->occupied) {
        table->buckets[hash]->key = strdup(key);
        if (table->buckets[hash]->key == NULL) {
            fprintf(stderr, "Memory allocation failed.\n");
            exit(EXIT_FAILURE);
        }
        table->buckets[hash]->occupied = 1;
        table->size++;
    }
    table->buckets[hash]->value = value;
}

Symbol *Symbol_Search(Symbol_HashTable *ht, const char *key) {
    size_t hash = Symbol_Hash(key) % ht->capacity;
    while (ht->buckets[hash]->occupied) {
        if (ht->buckets[hash]->occupied && strcmp(ht->buckets[hash]->key, key) == 0) {
            return ht->buckets[hash]->value;
        }
        hash = (hash + 1) % ht->capacity; // Linear probing
    }
    return NULL; // Key not found
}
