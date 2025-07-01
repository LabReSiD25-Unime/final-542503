
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

//sockets 
int webserver_fd, incoming_fd, epoll_fd;

//web server address
struct sockaddr_in web_address;
int addrlen = sizeof(web_address);

//epoll
struct epoll_event event, events[MAX_EVENTS];
int nevent;

//buffer
char buffer[BUFFER_SIZE] = {0};


void handle_event(struct epoll_event ev) {

	//if the event is on the webserver fd, accept the new connection
	if (ev.data.fd == webserver_fd) {

		if ((incoming_fd = accept(webserver_fd, (struct sockaddr *)&web_address, &addrlen)) < 0) {
			perror("client connection accept failed");
			close(webserver_fd);
			exit(EXIT_FAILURE);
		}

		//add client connection to epoll
		ev.events = EPOLLIN;
		ev.data.fd = incoming_fd;
		if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, incoming_fd, &event) == -1) {
			perror("epoll client connection add failed");
			close(webserver_fd);
			exit(EXIT_FAILURE);
		}

		printf("Connection accepted: %s:%d\n", inet_ntoa(web_address.sin_addr), ntohs(web_address.sin_port));

	} else {

		//event is on a client fd

		int isd = ev.data.fd;
		int readv = read(isd, buffer, BUFFER_SIZE - 1);

		if (readv == 0) {

			//if the read is empty, close connection

			getpeername(isd, (struct sockaddr *)&web_address, &addrlen);
			printf("Client Disconnected: %s:%d\n", inet_ntoa(web_address.sin_addr),ntohs(web_address.sin_port));
			epoll_ctl(epoll_fd, EPOLL_CTL_DEL, isd, NULL);
			close(isd);

		} else {

			//if the read is not empty, read message

			buffer[readv] = '\0';
			printf("Message: %s",buffer);
			send(isd, "ACK\n",4,0);
		}

	}
}

int main() {

	// Create webserver socket
	if ((webserver_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
		perror("webserver socket creation failed");
		exit(EXIT_FAILURE);
	}

	// Configure web_address
	web_address.sin_family = AF_INET;
	web_address.sin_addr.s_addr = INADDR_ANY;
	web_address.sin_port = htons(PORT);

	// Binding del socket
	if (bind(webserver_fd, (struct sockaddr *)&web_address, sizeof(web_address)) < 0) {
		perror("webserver socket binding failed");
		close(webserver_fd);
		exit(EXIT_FAILURE);
	}

	// listen for connections
	if (listen(webserver_fd, 3) < 0) {
		perror("connection listen failed");
		close(webserver_fd);
		exit(EXIT_FAILURE);
	}

	// epoll initialization
	if ((epoll_fd = epoll_create1(0)) == -1) {
		perror("epoll initialization failed");
		close(webserver_fd);
		exit (EXIT_FAILURE);
	}

	// add webserver to epoll
	event.events = EPOLLIN;
	event.data.fd = webserver_fd;
	if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, webserver_fd, &event) == -1) {
		perror("epoll webserver add failed");
		close(webserver_fd);
		exit(EXIT_FAILURE);
	}

	//event loop
	while(1) {

		nevent = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);

		//verify eopoll value
		if (nevent == -1) {
			perror("epoll wait failed");
			close(webserver_fd);
			exit(EXIT_FAILURE);
		}

		//iterate on events
		for(int i = 0; i < nevent; i++) {
			handle_event(events[i]);
		}
	}


	return 0;
}


