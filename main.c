#include "request.h"
#include "response.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h> 
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#define PORT 8090
#define MAX_CLIENTS 20
#define BUFFER_SIZE 10240

int webserver_fd;

struct sockaddr_in web_address;
socklen_t addrlen = sizeof(web_address);

pthread_t thread;

void* handle_client(void *arg) {

	int client_fd = *(int *)arg;
	free(arg);

	char buffer[BUFFER_SIZE];

	if (read(client_fd,buffer,BUFFER_SIZE-1) == -1) {
		perror("client read error");
		exit(EXIT_FAILURE);
	}

	printf("--- RAW ------------ \n%s\n",buffer);

	HttpRequest nreq;
	parse_http_request(buffer,&nreq);
	printf("--- REQUEST-------------- \n%s\n%s\n%s\n%s\n%s\n",nreq.method,nreq.path,nreq.version,nreq.headers,nreq.body);

	char* resp = handle_request(&nreq);
	printf("--- RESPONSE -------------- \n%s\n",resp);

	if (send(client_fd,resp,strlen(resp),0) == -1) {
		perror("Response send error");
		close(client_fd);
		exit(EXIT_FAILURE);
	}

	printf("Response sent, connection closed \n");
	close(client_fd);

	return NULL;
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


