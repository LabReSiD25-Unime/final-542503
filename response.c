#include "response.h"
#include "keyvalue.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h> 

char *get_html_file(const char* path) {

	//legge i contenuti di un documento html 

	int fd;
	char *body;
	int bytes_read, total_read = 0;

	fd = open(path, O_RDONLY);

	//se non trova il documento, ritorna del testo html minimo per segnalare il problema 
	if (fd < 0) {
		return strdup("<html><body><h1>404 Not Found</h1></body></html>");
	}

	body = malloc(MAX_FILE_SIZE + 1);

	//leggi il documento a pezzi
	while (total_read < MAX_FILE_SIZE) {
		bytes_read = read(fd, body + total_read, MAX_FILE_SIZE - total_read);
		if (bytes_read <= 0) break; 
		total_read += bytes_read;
	}
	close(fd);

	body[total_read] = '\0';
	return body;

}

int parse_form_data(const char *body, const int op) {

	//leggi i dati mandati da una richiesta POST, PUT o DELETE
	//esegui su di essi l'operazione corrispondente in base al parametro op

	char *p = strdup(body); 
	char *tok = strtok(p, "&");

	//flag usata per segnalare lo stato dell'operazione, 0 se non e stata completata, altrimenti 1
	int flag = 0;

	while (tok != NULL) {

		char *eq = strchr(tok, '=');

		if (eq) {

			*eq = '\0';
			const char *key = tok;
			const char *value = eq + 1;
			if (op == 1){
				//corrisponde al POST
				add_key_value(key, value);
				flag = 1;
			} else if(op == 0 && !is_kv_stored(key,value)) {
				//corrisponde al PUT
				add_key_value(key,value);
				flag = 1;
			} else if(op==2){
				//corrisponde al DELETE
				if (delete_kv_pair(key, value) == 1) {
					flag = 1;
				}
			}
		}

		tok = strtok(NULL, "&");
	}

	free(p);
	return flag;
}

void write_http_response(char *response_buffer, int status_code,const char *content_type, const char *body) {

	//scrivi il testo corrispondente all'operazione
	const char *status_text;

	switch (status_code) {
		case 200: status_text = "OK"; break;
		case 201: status_text = "Created"; break;
		case 204: status_text = "No Content"; break;
		case 400: status_text = "Bad Request"; break;
		default: status_text = "Internal Server Error"; break;
	}

	//determina la dimensione del body, se presente
	int content_length = body ? strlen(body) : 0;

	//scrivi la richiesta
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

char *handle_get(HttpRequest *req) {
        
        //trova il documento HTML corrispondente al path della richiesta
        //se il path e vuoto, ritorna index.html
	char fullpath[512];

	if (strcmp(req->path, "/") == 0) {
		snprintf(fullpath, sizeof(fullpath), "./www/index.html");
	} else {
		snprintf(fullpath, sizeof(fullpath), "./www%s", req->path);
	}

	return get_html_file(fullpath);	

}

char *handle_post(HttpRequest *req) {
        
        //parsa i dati del form, e aggiungi una coppia chiave-valore
	parse_form_data(req->body,1);
	
        //uguale al get
	char fullpath[512];

	if (strcmp(req->path, "/") == 0) {
		snprintf(fullpath, sizeof(fullpath), "./www/index.html");
	} else {
		snprintf(fullpath, sizeof(fullpath), "./www%s", req->path);
	}

	char *html = get_html_file(fullpath);   
        
        //aggiungi le coppie chiave-valore al documento html per visualizzarle
	char *response = append_kv_to_html(html);

	free(html);

	return response;

}

char *handle_put(HttpRequest *req,int* flag) {

        //aggiungi una coppia chiave-valore, ma solo se non e gia presente
	*flag = parse_form_data(req->body, 0);
	
	//se la chiave coppia-valore e gia presente, non continuare
	if (!*flag) {
		return NULL;	
	}
      
	char fullpath[524];
        
        //prendi solo il path al documento HTML
	char path_only[512];
	int i = 0;
	while (req->path[i] != '\0' && req->path[i] != '?' && i < sizeof(path_only) - 1) {
		path_only[i] = req->path[i];
		i++;
	}
	path_only[i] = '\0';
        
        //uguale al get
	if (strcmp(path_only, "/") == 0) {
		snprintf(fullpath, sizeof(fullpath), "./www/index.html");
	} else {
		snprintf(fullpath, sizeof(fullpath), "./www%s", path_only);
	}

	char *html = get_html_file(fullpath);

        //aggiungi le coppie chiave-valore al documento html per visualizzarle
	char *response = append_kv_to_html(html);

	free(html);

	return response;
}

char *handle_delete(HttpRequest *req,int* flag) {

        //cancella una coppia chiave valore
	*flag = parse_form_data(req->body,2);
	
        //se la chiave coppia-valore non era presente, non continuare
	if (!*flag) {
		return NULL;	
	}
        
        //uguale al get
	char fullpath[512];

	if (strcmp(req->path, "/") == 0) {
		snprintf(fullpath, sizeof(fullpath), "./www/index.html");
	} else {
		snprintf(fullpath, sizeof(fullpath), "./www%s", req->path);
	}

	char *html = get_html_file(fullpath);   
        
        //aggiungi le coppie chiave-valore al documento html per visualizzarle
	char *response = append_kv_to_html(html);

	free(html);

	return response;

}



char *handle_request(HttpRequest *req) {
        
	char *body = NULL;
	static char response[MAX_RESPONSE_SIZE];
        
        //determina quale metodo e stato richiesto, e chiama la corrispondente funzione che lo gestisce
        //poi scrivi una risposta
	if (strcmp(req->method, "GET") == 0) {

		body = handle_get(req);
		write_http_response(response, 200,"text/html",body);

	} else if (strcmp(req->method,"POST") == 0) {
    
		body = handle_post(req);
		write_http_response(response,201,"text/html",body);

	} else if (strcmp(req->method,"PUT") == 0) {
		
		int flg;
		
		body = handle_put(req,&flg);
		write_http_response(response,flg ? 201  : 204,"text/html",body);

	} else if (strcmp(req->method,"DELETE") == 0) {
		
		int flg;

		body = handle_delete(req,&flg);
		write_http_response(response,flg ? 201  : 204,"text/html",body);
	
	} else {

		body = strdup("<html><body><h1>400 Bad Request</h1></body></html>");
		write_http_response(response, 400,"text/html", body);

	}
	
	if (body != NULL) {
		free(body);
	}
	
	return response;
}
