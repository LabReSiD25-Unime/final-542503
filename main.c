
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/epoll.h>

#define PORT 8090
#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024
#define MAX_EVENTS 10

int webserver_fd, incoming_fd, epoll_fd;

struct sockaddr_in web_address;
int addrlen = sizeof(web_address);

struct epoll_event event, events[MAX_EVENTS];
int nevent;

char buffer[BUFFER_SIZE] = {0};

typedef struct {
	char method[8];
	char path[256];
	char version[16];
	char headers[1024];
	char body[2048];
} HttpRequest;

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

void write_http_response(char *response_buffer, int status_code, const char *body) {

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
			"Content-Type: text/plain\r\n"
			"Connection: close\r\n"
			"\r\n"
			"%s",
			status_code, status_text, content_length, body ? body : ""
	       );

}

void send_http_response(int client_fd, const char *response_buffer) {
    send(client_fd, response_buffer, strlen(response_buffer), 0);
}

void handle_event(struct epoll_event ev) {

	if (ev.data.fd == webserver_fd) {

		if ((incoming_fd = accept(webserver_fd, (struct sockaddr *)&web_address, &addrlen)) < 0) {
			perror("client connection accept failed");
			close(webserver_fd);
			exit(EXIT_FAILURE);
		}

		ev.events = EPOLLIN;
		ev.data.fd = incoming_fd;
		if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, incoming_fd, &event) == -1) {
			perror("epoll client connection add failed");
			close(webserver_fd);
			exit(EXIT_FAILURE);
		}

		printf("Connection accepted: %s:%d\n", inet_ntoa(web_address.sin_addr), ntohs(web_address.sin_port));

	} else {


		int isd = ev.data.fd;
		int readv = read(isd, buffer, BUFFER_SIZE - 1);

		if (readv == 0) {


			getpeername(isd, (struct sockaddr *)&web_address, &addrlen);
			printf("Client Disconnected: %s:%d\n", inet_ntoa(web_address.sin_addr),ntohs(web_address.sin_port));
			epoll_ctl(epoll_fd, EPOLL_CTL_DEL, isd, NULL);
			close(isd);

		} else {

			buffer[readv] = '\0';
			printf("Message: %s",buffer);
			send(isd, "ACK\n",4,0);
		}

	}
}

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

	if (listen(webserver_fd, 3) < 0) {
		perror("connection listen failed");
		close(webserver_fd);
		exit(EXIT_FAILURE);
	}

	if ((epoll_fd = epoll_create1(0)) == -1) {
		perror("epoll initialization failed");
		close(webserver_fd);
		exit (EXIT_FAILURE);
	}

	event.events = EPOLLIN;
	event.data.fd = webserver_fd;
	if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, webserver_fd, &event) == -1) {
		perror("epoll webserver add failed");
		close(webserver_fd);
		exit(EXIT_FAILURE);
	}

	while(1) {

		nevent = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);

		if (nevent == -1) {
			perror("epoll wait failed");
			close(webserver_fd);
			exit(EXIT_FAILURE);
		}

		for(int i = 0; i < nevent; i++) {
			handle_event(events[i]);
		}
	}


	return 0;
}


