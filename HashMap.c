//
// Created by Raz on 03/12/2020.
//

#include "HashMap.h"
#define HASH(func, key, capacity) func(key) & (capacity-1)

Vector **InitBuckets(size_t size, VectorElemCpy cpy_func, VectorElemCmp cmp_func,
                      VectorElemFree free_func);
void FreeBuckets(Vector** buckets, size_t size);
Vector** ReHashing(HashMap *hash_map, size_t new_cap);
int IncreaseTable(HashMap *hash_map, size_t new_cap, Pair* pair);
int DecreaseTable(HashMap *hash_map, size_t new_cap);
int GetPairIndexByKey(Vector * vec, KeyT key);

/**
 * Allocates dynamically new hash map element.
 * @param hash_func a function which "hashes" keys.
 * @param pair_cpy a function which copies pairs.
 * @param pair_cmp a function which compares pairs.
 * @param pair_free a function which frees pairs.
 * @return pointer to dynamically allocated HashMap.
 * @if_fail return NULL.
 */
HashMap *HashMapAlloc(HashFunc hash_func, HashMapPairCpy pair_cpy,
        HashMapPairCmp pair_cmp, HashMapPairFree pair_free){
    if (!hash_func || !pair_cpy || !pair_cmp || !pair_free) return NULL;
    HashMap * new_hash_map = malloc(sizeof(HashMap));
    if (!new_hash_map) return NULL;
    new_hash_map->buckets = InitBuckets(HASH_MAP_INITIAL_CAP, pair_cpy, pair_cmp,
                                        pair_free);
    if (!new_hash_map->buckets){
        free(new_hash_map);
        return NULL;
    }
    new_hash_map->capacity = HASH_MAP_INITIAL_CAP;
    new_hash_map->size = 0;
    new_hash_map->hash_func = hash_func;
    new_hash_map->pair_cpy = pair_cpy;
    new_hash_map->pair_cmp = pair_cmp;
    new_hash_map->pair_free = pair_free;
    return new_hash_map;
}

/**
 * Inserts a new pair to the hash map.
 * The function inserts *new*, *copied*, *dynamically allocated* pair,
 * NOT the pair it receives as a parameter.
 * @param hash_map the hash map to be inserted with new element.
 * @param pair a pair the hash map would contain.
 * @return returns 1 for successful insertion, 0 otherwise.
 */
int HashMapInsert(HashMap *hash_map, Pair *pair){
    if (!hash_map || !pair) return 0;
    size_t vector_index = HASH(hash_map->hash_func, pair->key, hash_map->capacity);
    int pair_index = GetPairIndexByKey(hash_map->buckets[vector_index], pair->key);
    if (pair_index != -1){
        hash_map->pair_free(&hash_map->buckets[vector_index]->data[pair_index]);
        hash_map->buckets[vector_index]->data[pair_index] = hash_map->pair_cpy(pair);
        return 1;
    }
    if (hash_map->capacity * HASH_MAP_MAX_LOAD_FACTOR < (double) hash_map->size + 1){
        if (IncreaseTable(hash_map, hash_map->capacity * HASH_MAP_GROWTH_FACTOR, pair) == 0){
            return 0;
        }
    }
    else {
        size_t ind = HASH(hash_map->hash_func, pair->key, hash_map->capacity);
        if (VectorPushBack(hash_map->buckets[ind], pair) == 0){
            return 0;
        }
    }
    hash_map->size++;
    return 1;
}

/**
 * The function checks if the given key exists in the hash map.
 * @param hash_map a hash map.
 * @param key the key to be checked.
 * @return 1 if the key is in the hash map, 0 otherwise.
 */
int HashMapContainsKey(HashMap *hash_map, KeyT key){
    if (!hash_map || !key) return 0;
    size_t vector_index = HASH(hash_map->hash_func, key, hash_map->capacity);
    int pair_index = GetPairIndexByKey(hash_map->buckets[vector_index], key);
    if (pair_index != -1) return 1;
    return 0;
}

/**
 * The function returns the value associated with the given key.
 * @param hash_map a hash map.
 * @param key the key to be checked.
 * @return the value associated with key if exists, NULL otherwise.
 */
ValueT HashMapAt(HashMap *hash_map, KeyT key){
    if (!hash_map || !key) return NULL;
    size_t vector_index = HASH(hash_map->hash_func, key, hash_map->capacity);
    int pair_index = GetPairIndexByKey(hash_map->buckets[vector_index], key);
    if (pair_index < 0) return NULL;
    Pair *pair = (Pair*) VectorAt(hash_map->buckets[vector_index], pair_index);
    if (!pair) return NULL;
    return pair->value;
}

/**
 * The function checks if the given value exists in the hash map.
 * @param hash_map a hash map.
 * @param value the value to be checked.
 * @return 1 if the value is in the hash map, 0 otherwise.
 */
int HashMapContainsValue(HashMap *hash_map, ValueT value){
    if (!hash_map || !value || hash_map->size == 0){
        return 0;
    }
    for (size_t i = 0; i < hash_map->capacity; ++i) {
        for (size_t j = 0; j < hash_map->buckets[i]->size; ++j) {
            Pair *pair = (Pair*) hash_map->buckets[i]->data[j];
            if (pair->value_cmp(pair->value, value) == 1){
                return 1;
            }
        }
    }
    return 0;
}

/**
 * This function returns the load factor of the hash map.
 * @param hash_map a hash map.
 * @return the hash map's load factor, -1 if the function failed.
 */
double HashMapGetLoadFactor(HashMap *hash_map){
    if (!hash_map) return -1;
    return (double) hash_map->size / (double) hash_map->capacity;
}

/**
 * Frees a vector and the elements the vector itself allocated.
 * @param p_hash_map pointer to dynamically allocated pointer to hash_map.
 */
void HashMapFree(HashMap **p_hash_map){
    if (!p_hash_map || !(*p_hash_map)){
        return;
    }
    for (size_t i = 0; i < (*p_hash_map)->capacity; ++i) {
        VectorFree(&(*p_hash_map)->buckets[i]);
    }
    free((*p_hash_map)->buckets);
    (*p_hash_map)->buckets = NULL;
    free(*p_hash_map);
    *p_hash_map = NULL;
}

/**
 * This function deletes all the elements in the hash map.
 * @param hash_map a hash map to be cleared.
 */
void HashMapClear(HashMap *hash_map){
    if (!hash_map || hash_map->size == 0) return;
    for (long i = (long) hash_map->capacity - 1; i >= 0; --i) {
        for (long j = (long) hash_map->buckets[i]->size - 1; j >= 0; --j) {
            Pair* pair = (Pair*) hash_map->buckets[i]->data[j];
            if (HashMapErase(hash_map, pair->key) == 0) return;
        }
    }
}

/**
 * The function erases the pair associated with key.
 * @param hash_map a hash map.
 * @param key a key of the pair to be erased.
 * @return 1 if the erasing was done successfully, 0 otherwise.
 */
int HashMapErase(HashMap *hash_map, KeyT key){
    if (!hash_map || !key) return 0;
    if (HashMapContainsKey(hash_map, key) == 0) return 0;
    size_t vector_index = HASH(hash_map->hash_func, key, hash_map->capacity);
    int pair_index = GetPairIndexByKey(hash_map->buckets[vector_index], key);
    if (pair_index == -1) return 0;
    if (VectorErase(hash_map->buckets[vector_index], pair_index) == 0) return 0;
    --hash_map->size;
    return DecreaseTable(hash_map, hash_map->capacity / HASH_MAP_GROWTH_FACTOR);
}

/*
 * This function creates buckets and allocates its vectors. Returns the buckets
 * and NULL for failure
 */
Vector ** InitBuckets(size_t size, VectorElemCpy cpy_func, VectorElemCmp cmp_func,
                      VectorElemFree free_func){
    Vector **buckets = calloc(1, size * sizeof(Vector*));
    if (!buckets){
        return NULL;
    }
    for (size_t i = 0; i < size; ++i) {
        buckets[i] = VectorAlloc(cpy_func, cmp_func, free_func);
        if (!buckets[i]){
            FreeBuckets(buckets, size);
            free(buckets);
            return NULL;
        }
    }
    return buckets;
}

/*
 * This function gets a key to find in the input vector. It returns the index of
 * that key in the vector. If not found returns -1 for.
 */
int GetPairIndexByKey(Vector * vec, KeyT key){
    if (!vec || !key) return -1;
    for (size_t i = 0; i < vec->size; ++i) {
        Pair *p = (Pair*) vec->data[i];
        if (p->key_cmp(p->key, key) == 1){
            return i;
        }
    }
    return -1;
}

/*
 * This function free the vectors in the input buckets
 */
void FreeBuckets(Vector** buckets, size_t size){
    for (size_t i = 0; i < size; ++i) {
        VectorFree(&buckets[i]);
        //buckets[i] = NULL;
    }
}

/*
 * This function adds a pair to the hashmap when needed to increase the buckets
 * first. When increased it also rehash all items again and frees the old
 * buckets. Return 1 for success, 0 for failure
 */
int IncreaseTable(HashMap *hash_map, size_t new_cap, Pair* pair){
    Vector **temp = ReHashing(hash_map, new_cap);
    if (!temp){
        return 0;
    }
    size_t ind = HASH(hash_map->hash_func, pair->key, new_cap);
    if (VectorPushBack(temp[ind], pair) == 0){
        FreeBuckets(temp, new_cap);
        free(temp);
        return 0;
    }
    FreeBuckets(hash_map->buckets, hash_map->capacity);
    free(hash_map->buckets);
    hash_map->capacity = new_cap;
    hash_map->buckets = temp;
    return 1;
}

/*
 * This function decrease the buckets if needed. It rehash all items again and
 * frees the old buckets. Return 1 for success, 0 for failure
 */
int DecreaseTable(HashMap *hash_map, size_t new_cap){
    if (HashMapGetLoadFactor(hash_map) >= HASH_MAP_MIN_LOAD_FACTOR) return 1;
    Vector **temp = ReHashing(hash_map, new_cap);
    if (!temp){
        return 0;
    }
    FreeBuckets(hash_map->buckets, hash_map->capacity);
    free(hash_map->buckets);
    hash_map->capacity = new_cap;
    hash_map->buckets = temp;
    return 1;
}

/*
 * This function rehash the buckets of the input hashmap to a new buckets with
 * the input size (either increase or decrease). It returns the new buckets.
 */
Vector **ReHashing(HashMap *hash_map, size_t new_cap){
    Vector **temp = InitBuckets(new_cap, hash_map->pair_cpy, hash_map->pair_cmp, hash_map->pair_free);
    if (!temp){
        return NULL;
    }
    for (size_t i = 0; i < hash_map->capacity; ++i) {
        for (size_t j = 0; j < hash_map->buckets[i]->size; ++j) {
            Pair* exist_pair = (Pair *) hash_map->buckets[i]->data[j];
            size_t new_ind = HASH(hash_map->hash_func, exist_pair->key, new_cap);
            if(VectorPushBack(temp[new_ind], exist_pair) == 0){
                FreeBuckets(temp, new_cap);
                free(temp);
                return NULL;
            }
        }
    }
    return temp;
}
