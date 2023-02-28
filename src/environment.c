#include "monkey/environment.h"

#include <iso646.h>
#include <stdint.h>

#include "monkey/private/stdc.h"

const double MAX_LOAD_FACTOR = 0.75;

const uint64_t FNV_OFFSET_BASIS = UINT64_C(14695981039346656037);
const uint64_t FNV_PRIME = UINT64_C(1099511628211);

ALLOW_UINT_OVERFLOW static uint64_t fnv1a(struct string str) {
    uint64_t hash = FNV_OFFSET_BASIS;
    for (size_t i = 0; i < str.length; i++) {
        hash ^= str.data[i];
        hash *= FNV_PRIME;
    }
    return hash;
}

static struct environment_entry*
find_bucket(struct environment_entry_buf entries, struct string name) {
    uint64_t hash = fnv1a(name);
    size_t index = hash % entries.len;
    while (entries.ptr[index].name.length > 0 and !STRING_EQUAL(entries.ptr[index].name, name)) {
        index = (index + 1) % entries.len;
    }
    return &entries.ptr[index];
}

void environment_init(struct environment* env) {
    env->entries = (struct environment_entry_buf){0};
    env->count = 0;
}

void environment_free(struct environment env) {
    for (size_t i = 0; i < env.entries.len; i++) {
        if (env.entries.ptr[i].name.length > 0) {
            STRING_FREE(env.entries.ptr[i].name);
            object_free(env.entries.ptr[i].value);
        }
    }
    BUF_FREE(env.entries);
}

void environment_set(struct environment* env, struct string name, struct object* value) {
    if (env->count + 1 > env->entries.len * MAX_LOAD_FACTOR) {
        size_t new_len = env->entries.len == 0 ? 8 : env->entries.len * 2;
        struct environment_entry* new_entries_data =
            calloc(new_len, sizeof(struct environment_entry));
        struct environment_entry_buf new_entries =
            BUF_OWNER(struct environment_entry_buf, new_entries_data, new_len);
        for (size_t i = 0; i < env->entries.len; i++) {
            if (env->entries.ptr[i].name.length > 0) {
                struct environment_entry* bucket =
                    find_bucket(new_entries, env->entries.ptr[i].name);
                bucket->name = env->entries.ptr[i].name;
                bucket->value = env->entries.ptr[i].value;
            }
        }
        BUF_FREE(env->entries);
        env->entries = new_entries;
    }
    struct environment_entry* bucket = find_bucket(env->entries, name);
    if (bucket->name.length == 0) {
        env->count++;
    } else {
        STRING_FREE(bucket->name);
        object_free(bucket->value);
    }
    bucket->name = name;
    bucket->value = value;
}

struct object* environment_get(struct environment* env, struct string name) {
    if (env->count == 0) {
        return NULL;
    }
    struct environment_entry* bucket = find_bucket(env->entries, name);
    if (bucket->name.length == 0) {
        return NULL;
    }
    return bucket->value;
}
