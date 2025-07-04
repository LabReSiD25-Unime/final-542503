#ifndef REQUEST_H
#define REQUEST_H

typedef struct {
	char method[8];
	char path[256];
	char version[16];
	char headers[1024];
	char body[2048];
} HttpRequest;

void parse_http_request(const char *raw_request, HttpRequest *req);

#endif
