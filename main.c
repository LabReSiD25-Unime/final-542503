
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h> 
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <pthread.h>

#define PORT 8090
#define MAX_CLIENTS 20
#define BUFFER_SIZE 10240
#define MAX_EVENTS 10
#define MAX_RESPONSE_SIZE 10240
#define MAX_FILE_SIZE 10240
#define MAX_KV_PAIRS 100

typedef struct {
	char method[8];
	char path[256];
	char version[16];
	char headers[1024];
	char body[2048];
} HttpRequest;

struct KeyValue {
	char key[64];
	char value[256];
};

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


void parse_http_request(const char *raw_request, HttpRequest *req) {

	memset(req, 0, sizeof(HttpRequest));

	sscanf(raw_request, "%7s %255s %15s", req->method, req->path, req->version);

	const char *header_start = strstr(raw_request, "\r\n");

	if (header_start) {
		header_start += 2;
	} else {
		return;
	}

	const char *body_start = strstr(raw_request, "\r\n\r\n");

	if (body_start) {

		int header_len = (int)(body_start - header_start);

		if (header_len > 1023) header_len = 1023;

		strncpy(req->headers, header_start, header_len);
		req->headers[header_len] = '\0';

		strcpy(req->body, body_start + 4);

	} else {

		strncpy(req->headers, header_start, 1023);

	}
}

void write_http_response(char *response_buffer, int status_code,const char *content_type, const char *body) {

	const char *status_text;

	switch (status_code) {
		case 200: status_text = "OK"; break;
		case 201: status_text = "Created"; break;
		case 204: status_text = "No Content"; break;
		case 400: status_text = "Bad Request"; break;
		case 401: status_text = "Unauthorized"; break;
		default: status_text = "Internal Server Error"; break;
	}

	int content_length = body ? strlen(body) : 0;

	sprintf(
			response_buffer,
			"HTTP/1.1 %d %s\r\n"
			"Content-Length: %d\r\n"
			"Content-Type: %s\r\n"
			"Connection: close\r\n"
			"\r\n"
			"%s",
			status_code, status_text, content_length, content_type, body ? body : ""
	       );

}

char *get_html_file(const char* path) {

	int fd;
	char *body;
	int bytes_read, total_read = 0;

	fd = open(path, O_RDONLY);
	if (fd < 0) {
		return strdup("<html><body><h1>404 Not Found</h1></body></html>");
	}

	body = malloc(MAX_FILE_SIZE + 1);

	while (total_read < MAX_FILE_SIZE) {
		bytes_read = read(fd, body + total_read, MAX_FILE_SIZE - total_read);
		if (bytes_read <= 0) break; 
		total_read += bytes_read;
	}
	close(fd);

	body[total_read] = '\0';
	return body;

}

void parse_form_data(const char *body, const int duplicates) {

	char pair[512];
	char *p = strdup(body); 
	char *tok = strtok(p, "&");

	while (tok != NULL) {

		char *eq = strchr(tok, '=');

		if (eq) {

			*eq = '\0';
			const char *key = tok;
			const char *value = eq + 1;
			if (duplicates){
				add_key_value(key, value);
			} else if(!is_kv_stored(key,value)) {
				add_key_value(key,value);
			}
		}

		tok = strtok(NULL, "&");
	}

	free(p);
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

char *handle_get(HttpRequest *req) {

	char fullpath[512];

	if (strcmp(req->path, "/") == 0) {
		snprintf(fullpath, sizeof(fullpath), "./www/index.html");
	} else {
		snprintf(fullpath, sizeof(fullpath), "./www%s", req->path);
	}

	return get_html_file(fullpath);	

}

char *handle_post(HttpRequest *req) {

	parse_form_data(req->body,1);

	char fullpath[512];

	if (strcmp(req->path, "/") == 0) {
		snprintf(fullpath, sizeof(fullpath), "./www/index.html");
	} else {
		snprintf(fullpath, sizeof(fullpath), "./www%s", req->path);
	}

	char *html = get_html_file(fullpath);   

	char *response = append_kv_to_html(html);

	free(html);

	return response;

}

char *handle_put(HttpRequest *req) {

	parse_form_data(req->body, 0);

	char fullpath[524];

	char path_only[512];
	int i = 0;
	while (req->path[i] != '\0' && req->path[i] != '?' && i < sizeof(path_only) - 1) {
		path_only[i] = req->path[i];
		i++;
	}
	path_only[i] = '\0';

	printf("---PRINT: %s \n",fullpath);

	if (strcmp(path_only, "/") == 0) {
		snprintf(fullpath, sizeof(fullpath), "./www/index.html");
	} else {
		snprintf(fullpath, sizeof(fullpath), "./www%s", path_only);
	}

	char *html = get_html_file(fullpath);

	char *response = append_kv_to_html(html);

	free(html);

	return response;
}


char *handle_request(HttpRequest *req) {

	char *body = NULL;
	static char response[MAX_RESPONSE_SIZE];

	if (strcmp(req->method, "GET") == 0) {

		body = handle_get(req);
		write_http_response(response, 200,"text/html",body);

	} else if (strcmp(req->method,"POST") == 0) {

		body = handle_post(req);
		write_http_response(response,200,"text/html",body);

	} else if (strcmp(req->method,"PUT") == 0) {

		body = handle_put(req);
		write_http_response(response,200,"text/html",body);

	} else {

		body = strdup("<html><body><h1>400 Bad Request</h1></body></html>");
		write_http_response(response, 400,"text/html", body);

	}

	free(body);
	return response;
}

void* handle_client(void *arg) {

	int client_fd = *(int *)arg;
	free(arg);

	char buffer[BUFFER_SIZE];

	if (read(client_fd,buffer,BUFFER_SIZE-1) == -1) {
		perror("client read error");
		exit(EXIT_FAILURE);
	}

	HttpRequest nreq;
	parse_http_request(buffer,&nreq);
	printf("--- REQUEST-------------- \n%s\n%s\n%s\n%s\n",nreq.headers,nreq.method,nreq.path,nreq.body);

	char* resp = handle_request(&nreq);
	printf("--- RESPONSE -------------- \n%s\n",resp);

	if (send(client_fd,resp,strlen(resp),0) == -1) {
		perror("Response send error");
		close(client_fd);
		exit(EXIT_FAILURE);
	}

	printf("Response sent, connection closed \n");
	close(client_fd);

}

int webserver_fd;

struct sockaddr_in web_address;
int addrlen = sizeof(web_address);

pthread_t thread;

int main() {


	if ((webserver_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
		perror("webserver socket creation failed");
		exit(EXIT_FAILURE);
	}

	web_address.sin_family = AF_INET;
	web_address.sin_addr.s_addr = INADDR_ANY;
	web_address.sin_port = htons(PORT);

	if (bind(webserver_fd, (struct sockaddr *)&web_address, sizeof(web_address)) < 0) {
		perror("webserver socket binding failed");
		close(webserver_fd);
		exit(EXIT_FAILURE);
	}

	if (listen(webserver_fd, MAX_CLIENTS) < 0) {
		perror("connection listen failed");
		close(webserver_fd);
		exit(EXIT_FAILURE);
	}

	printf("Listening on: %d \n",PORT);

	while (1) {

		int* incoming_fd = malloc(sizeof(int)); 

		if ((*incoming_fd = accept(webserver_fd, (struct sockaddr *)&web_address, &addrlen)) < 0) {
			perror("client connection accept failed");
			close(webserver_fd);
			exit(EXIT_FAILURE);
		}

		printf("Connection accepted: %s:%d\n", inet_ntoa(web_address.sin_addr), ntohs(web_address.sin_port));

		if(pthread_create(&thread,NULL,handle_client,incoming_fd)){
			perror("Thread creation failed");
			close(webserver_fd);
			exit(EXIT_FAILURE);
		}

		pthread_detach(thread);

	}

	close(webserver_fd);
	return 0;

}


