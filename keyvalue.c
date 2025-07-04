#include "keyvalue.h"
#include <string.h>
#include <stdlib.h>
#include <string.h>

#define MAX_KV_PAIRS 100

struct KeyValue store[MAX_KV_PAIRS];
int store_count = 0;

void add_key_value(const char *key, const char *value) {

	if (store_count >= MAX_KV_PAIRS) return;

	strncpy(store[store_count].key, key, sizeof(store[store_count].key) - 1);
	strncpy(store[store_count].value, value, sizeof(store[store_count].value) - 1);
	store_count++;

}

int is_kv_stored(const char *key, const char *value) {

	for (int i = 0; i < store_count; i++) {

		if (strcmp(store[i].key, key) == 0 && strcmp(store[i].value, value) == 0) {
			return 1;  
		}

	}

	return 0; 
}

int delete_kv_pair(const char *key, const char *value) {

	int found = 0;

	for (int i = 0; i < store_count; i++) {
		if (strcmp(store[i].key, key) == 0 && strcmp(store[i].value, value) == 0) {
			for (int j = i; j < store_count - 1; j++) {
				store[j] = store[j + 1];
			}
			store_count--;
			found = 1;
			break;
		}
	}

	return found;
}

char *append_kv_to_html(const char *html) {

	char kv_html[2048];
	strcpy(kv_html, "<h2>Saved Pairs:</h2><ul>");

	for (int i = 0; i < store_count; i++) {

		strcat(kv_html, "<li>");
		strcat(kv_html, store[i].key);
		strcat(kv_html, ": ");
		strcat(kv_html, store[i].value);
		strcat(kv_html, "</li>");

	}

	strcat(kv_html, "</ul>");

	char *final = malloc(strlen(html) + strlen(kv_html) + 1);
	strcpy(final, html);
	strcat(final, kv_html);

	return final;
}
