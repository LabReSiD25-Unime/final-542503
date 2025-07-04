#include "request.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void parse_http_request(const char *raw_request, HttpRequest *req) {

	memset(req, 0, sizeof(HttpRequest));

	sscanf(raw_request, "%7s %255s %15s", req->method, req->path, req->version);

	const char *header_start = strstr(raw_request, "\r\n");
	if (!header_start) return;
	header_start += 2;

	const char *body_start = strstr(raw_request, "\r\n\r\n");

	if (body_start && body_start > header_start) {
		int header_len = (int)(body_start - header_start);
		if (header_len > 1023) header_len = 1023;

		strncpy(req->headers, header_start, header_len);
		req->headers[header_len] = '\0';

		
		int content_length = 0;
		const char *cl = strcasestr(req->headers, "Content-Length:");
		if (cl) {
			cl += strlen("Content-Length:");
			while (*cl == ' ') cl++; 
			content_length = atoi(cl);
		}

		if (content_length > 0 && content_length < sizeof(req->body)) {
			strncpy(req->body, body_start + 4, content_length);
			req->body[content_length] = '\0';
		} else {
			req->body[0] = '\0';
		}

	} else {
		strncpy(req->headers, header_start, 1023);
		req->headers[1023] = '\0';
		req->body[0] = '\0';
	}
}
