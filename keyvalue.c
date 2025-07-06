#include "keyvalue.h"
#include <bits/pthreadtypes.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define MAX_KV_PAIRS 100

struct KeyValue store[MAX_KV_PAIRS];
int store_count = 0;
pthread_mutex_t store_mutex = PTHREAD_MUTEX_INITIALIZER;

void add_key_value(const char *key, const char *value) {
	
	pthread_mutex_lock(&store_mutex);
        
        //aggiungi una coppia chiave-valore, se possibile
	if (store_count >= MAX_KV_PAIRS) {
	                pthread_mutex_unlock(&store_mutex);  
	                return;
        }

	strncpy(store[store_count].key, key, sizeof(store[store_count].key) - 1);
	strncpy(store[store_count].value, value, sizeof(store[store_count].value) - 1);
	store_count++;

	pthread_mutex_unlock(&store_mutex);

}

int is_kv_stored(const char *key, const char *value) {
	
	pthread_mutex_lock(&store_mutex);
        
        //cerca una coppia chiave-valore nell'array
	for (int i = 0; i < store_count; i++) {

		if (strcmp(store[i].key, key) == 0 && strcmp(store[i].value, value) == 0) {
		        pthread_mutex_unlock(&store_mutex);
			return 1;  
		}

	}

	pthread_mutex_unlock(&store_mutex);

	return 0; 
}

int delete_kv_pair(const char *key, const char *value) {
  
	pthread_mutex_lock(&store_mutex);
        
        //trovauna coppia chiave-valore e muovi le coppie successive verso sinistra per cancellarla
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

	pthread_mutex_unlock(&store_mutex);

	return found;
}

char *append_kv_to_html(const char *html) {
        
        //aggiungi a un documento html una lista che mostra le coppie chiave-valore
        
	char kv_html[2048];
	strcpy(kv_html, "<h2>Saved Pairs:</h2><ul>");
	
	pthread_mutex_lock(&store_mutex);
        
        //leggi la lista e aggiungi ogni coppia
	for (int i = 0; i < store_count; i++) {

		strcat(kv_html, "<li>");
		strcat(kv_html, store[i].key);
		strcat(kv_html, ": ");
		strcat(kv_html, store[i].value);
		strcat(kv_html, "</li>");

	}
	
	pthread_mutex_unlock(&store_mutex);
        
	strcat(kv_html, "</ul>");
        
        //genera il documento finalizzato
	char *final = malloc(strlen(html) + strlen(kv_html) + 1);
	strcpy(final, html);
	strcat(final, kv_html);

	return final;
}
