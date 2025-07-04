#ifndef KEYVALUE_H
#define KEYVALUE_H

#define MAX_KV_PAIRS 100

#include <pthread.h>

struct KeyValue {
    char key[64];
    char value[256];
};

extern struct KeyValue store[MAX_KV_PAIRS];
extern int store_count;
extern pthread_mutex_t store_mutex;

void add_key_value(const char *key, const char *value);
int is_kv_stored(const char *key, const char *value);
int delete_kv_pair(const char *key, const char *value);
char *append_kv_to_html(const char *html);

#endif 

