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

struct sockaddr_in web_address, client_address;
socklen_t addrlenw = sizeof(web_address);
socklen_t addrlenc = sizeof(client_address);

pthread_t thread;

void* handle_client(void *arg) {
	
	//gestisci la richiesta di un client
	int client_fd = *(int *)arg;
	free(arg);
	
	//leggi il messaggio del client ed inseriscilo nel buffer
	char buffer[BUFFER_SIZE];

        ssize_t total = 0, n;
        while ((n = read(client_fd, buffer + total, BUFFER_SIZE - 1 - total)) > 0) {
            total += n;
            if (strstr(buffer, "\r\n\r\n")) break;
        }
        buffer[total] = '\0';

	
	printf("--- RAW ------------ \n%s\n",buffer);

	//parsing della richiesta
	HttpRequest nreq;
	parse_http_request(buffer,&nreq);
	printf("--- REQUEST-------------- \n%s\n%s\n%s\n%s\n%s\n",nreq.method,nreq.path,nreq.version,nreq.headers,nreq.body);

	//gestisci la richiesta e scrivi la risposta
	char* resp = handle_request(&nreq);
	printf("--- RESPONSE -------------- \n%s\n",resp);
	
	//invia la risposta al client
	if (send(client_fd,resp,strlen(resp),0) == -1) {
		perror("Response send error");
		close(client_fd);
		exit(EXIT_FAILURE);
	}
	
	//chiudi la connessione
	close(client_fd);
	pthread_exit(NULL);
	printf("Response sent, connection closed \n");

	return NULL;
}

int main() {
	
	//Configurazione della connessione

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
	

	//Accetta connessioni in arrivo, e crea un thread per gestirle.
	while (1) {

		int* incoming_fd = malloc(sizeof(int)); 
		addrlenc = sizeof(client_address);

		if ((*incoming_fd = accept(webserver_fd, (struct sockaddr *)&client_address, &addrlenc)) < 0) {
			perror("client connection accept failed");
			close(webserver_fd);
			exit(EXIT_FAILURE);
		}

		printf("Connection accepted: %s:%d\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));

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


