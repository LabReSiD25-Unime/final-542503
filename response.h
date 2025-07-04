#ifndef RESPONSE_H
#define RESPONSE_H

#include "request.h"  

#define MAX_RESPONSE_SIZE 10240
#define MAX_FILE_SIZE 10240

char *get_html_file(const char* path);
int parse_form_data(const char *body, int op);
void write_http_response(char *response_buffer, int status_code,const char *content_type, const char *body);

char *handle_get(HttpRequest *req);
char *handle_post(HttpRequest *req);
char *handle_put(HttpRequest *req, int *flag);
char *handle_delete(HttpRequest *req, int *flag);

char *handle_request(HttpRequest *req);

#endif 

