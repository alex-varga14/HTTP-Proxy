#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <signal.h>
#include <netdb.h>
/* Global manifest constants */
#define MAX_MESSAGE_SIZE 1024
#define MAX_USERS 100
//Function prorotypes
void handleText(char* resp);
void handleHTML(char* resp);

// Global Variables:
int server_socket, conn_socket, client_socket;
uint16_t port;
char clown_jpg[] = "GET http://pages.cpsc.ucalgary.ca/~carey/CPSC441/ass1/clown1.png HTTP/1.1";
char clown_jpgg[] = "http://pages.cpsc.ucalgary.ca/~carey/CPSC441/ass1/clown1.png";
char clown_jpgg2[] = "http://pages.cpsc.ucalgary.ca/~carey/CPSC441/ass1/clown1.png\" width=\"200\" height=\"300\">\r\n\n<p>\nYour Web browser should be able to display this page just fine,\n but the contents of the image might change when using the proxy.\n\n</p>\n\n</body>\n\n</html>";
char path[MAX_MESSAGE_SIZE];
char url[MAX_MESSAGE_SIZE];

int main(int argc, char *argv[])
{
	// socket init variables
    struct sockaddr_in server_address;
	struct addrinfo client_address;
	struct addrinfo *hostent;
	struct addrinfo *tmp;
	char* client_request;
	char* server_request;
	//HTTP header vars
	char* host;
	char* header;
	char* content_type;
    char* content_length;
	//int flag = 0;
	
	//check for valid input of the form:
	// ./clownproxy <PORT#>
	if(argc != 2)
	{
		fprintf(stderr, "ERROR: Clown Proxy needs a Port Number!\n");
		exit(1);
	}
	// Initialize server sockaddr structure
	port = atoi(argv[1]);
	memset(&server_address, 0, sizeof(&server_address)); 
    server_address.sin_family = AF_INET; 
    server_address.sin_port = htons(port);
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);//local IP
    //create server socket 
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0)
	{
       fprintf(stderr, "ERROR: Socket() call failed!\n");
       exit(1);
    }
	//bind specific address and port to the end point
    if (bind(server_socket, (struct sockaddr *) &server_address, sizeof(struct sockaddr_in)) == -1){
        fprintf(stderr, "ERROR: Bind() call failed!\n");
        exit(1);
    }
	// start listenting for connections from clients
	if(listen(server_socket, 100) == -1 ) // up to 100 participants can connect
	{
		fprintf(stderr, "ERROR: Listen() call failed!\n");
		exit(1);
	}
	// listen forever
    for( ; ; )
	{
		//flag = 0;
		// accept a connection
        conn_socket = accept(server_socket, NULL, NULL);
        if (conn_socket == -1){
            //close(server_socket);
            fprintf(stderr, "ERROR: accept() call failed!\n");
			exit(1);
        }
		//Allocate dynamic memory to heap for request
		client_request =  (char*)malloc(MAX_MESSAGE_SIZE * sizeof(char));
		memset(client_request, 0, MAX_MESSAGE_SIZE);
		//Obtain the message from this client 
		if (recv(conn_socket, client_request, sizeof(char) * MAX_MESSAGE_SIZE, 0) == -1){
            fprintf(stderr, "ERROR: recv() call 1 failed!\n");
			exit(1);
        }
		host = (char*) malloc(MAX_MESSAGE_SIZE * sizeof(char));
        memset(host, 0, MAX_MESSAGE_SIZE);
		char cpy[MAX_MESSAGE_SIZE];
		strcpy(cpy, client_request);
		char* http = strtok(cpy, "\r\n");
		header = strstr(client_request, "GET");
		
		//locate and replace the image
		int len = strlen(http);
		char* image = strstr(http, ".jpg");
		printf("REQ 1: %s\n", client_request);
		if(image != NULL)
		{
			strncpy(client_request, clown_jpg, len); //replace image with clown_jpg
			strncpy(http, clown_jpg, len);
			//flag = 1;
		}
		//establish URL
		//printf("HTTP request:%s\n", http);
		sscanf(http, "GET http://%s", url);
		//loop through the url to copy hostname
		int index;
		for(index = 0; index < strlen(url); index++)
		{
			if(url[index] == '/')
			{
				strncpy(host, url, index); //copy hostname
				host[index] = '\0'; //space at the end of host
				break;
			}
		}
		// char re[MAX_MESSAGE_SIZE]={0};
		// sprintf(re,"%s\r\nHost: %s\r\nContent-Type: image/png\r\nConnection: keep-alive\r\n\r\n", http, host); 
		
		//establish path
		memset(path, 0, MAX_MESSAGE_SIZE);
		strcat(path, &url[index]);
		server_request = (char*) malloc(MAX_MESSAGE_SIZE * sizeof(char));
		memset(server_request, 0, MAX_MESSAGE_SIZE);
		// Initialize sockaddr structure
        memset(&client_address, 0, sizeof(client_address));
        client_address.ai_family = AF_INET;
        client_address.ai_socktype = SOCK_STREAM;
        client_address.ai_flags = AI_PASSIVE;

		//get address info for host
        if (getaddrinfo(host, "http", &client_address, &hostent) != 0){
            fprintf(stderr, "ERROR: could not read host info!\n");
            exit(1);
        }
		//TCP socket for connection to web server
        if ((client_socket = socket(hostent->ai_family, hostent->ai_socktype, hostent->ai_protocol)) == -1){
            fprintf(stderr, "ERROR: Socket() call failed!\n");
		    exit(1);
        }
		//connect to server-side socket 
		if ((connect(client_socket, hostent->ai_addr, hostent->ai_addrlen)) == -1){
			fprintf(stderr, "ERROR: connect() call failed!\n");
			exit(1);
		} 
		//Send out message
		// if(flag == 1)
		// {
			// if(send(client_socket, re, strlen(re), 0) == -1)
			// {
				// fprintf(stderr, "ERROR: send() call failed!\n");
				// exit(1);
			// }
		// }
		// else {
			if(send(client_socket, client_request, strlen(client_request), 0) == -1)
			{
				fprintf(stderr, "ERROR: send() call failed!\n");
				exit(1);
			}
		//}
		char* w_message_in = (char*) malloc(MAX_MESSAGE_SIZE * sizeof(char));
		memset(w_message_in, 0, MAX_MESSAGE_SIZE);
		//receive HTTP reponse from server
		if(recv(client_socket, w_message_in, sizeof(char) * MAX_MESSAGE_SIZE, 0) == -1)
		{
			fprintf(stderr, "ERROR: recv() call 1 failed!\n");
			exit(1);
		}

        content_type = NULL;
        content_length = NULL;
        content_type = strstr(w_message_in, "Content-Type: ");
		content_length = strstr(w_message_in, "Content-Length: ");
		// char* img = strstr(content_type, ".jpg");
		// if(img != NULL)
		// {
			// if(strstr(content_type, "<a href=\"") == NULL)
			// {
				// char* cont = strstr(content_type, "Your");
				// char* imgs = strstr(content_type, "http://");
				// strncpy(imgs, clown_jpgg, strlen(clown_jpgg));
				// char* aa = strstr(imgs, "t=\"");
				// strncpy((aa+3), clown_jpgg2, strlen(clown_jpgg2));
			// }
			// else{ flag = 0; }
				
		// }
		// printf("WMESG: %s\n", w_message_in);
        strncpy(server_request, w_message_in, strlen(w_message_in));
		//printf("SERV REQ: %s\n", server_request);
		if(strstr(content_type, "image") == NULL)
		{
			if (strstr(content_type, "html") != NULL)
				handleHTML(server_request);
			if (strstr(content_type, "plain") != NULL)
				handleText(server_request);
		}
		//printf("SERV REQ: %s\n", server_request);
		//send reponse to web server
		if(send(conn_socket, server_request, strlen(server_request), 0) == -1)
		{
			fprintf(stderr, "ERROR: send() call failed!\n");
			exit(1);
		}
		// close all sockets except server-side socket
		close(conn_socket);
        close(client_socket);
        // Destroy all memory allocated on heap
		free(w_message_in);
		free(server_request);
		free(host);
        w_message_in = NULL;
        content_type = NULL;
        content_length =NULL;
        header = NULL;
	}
	close(server_socket);
}

void handleText(char* resp)
{
	char* r = strstr(resp, "Happy");
	//replace all "Happy" with "Silly"
	while(r != NULL)
	{
		strncpy(r, "Silly", 5);
		r = strstr(resp, "Happy");
	}
	r = strstr(resp, "happy");
	//replace all "happy" with "silly"
	while(r != NULL)
	{
		strncpy(r, "silly", 5);
		r = strstr(resp, "happy");
	}
}

void handleHTML(char* resp)
{
	char* it = resp;
	//parse through server HTTP response
	while(it != NULL)
	{
		char* img = strstr(it, "<img src=\""); //image tag
		char* link = strstr(it, "<a href=\""); //link tag
		char* end = img; //set end to image
		//set end to link if link comes before image
		if(link < img)
			end = link;
		if(link == NULL && img == NULL)
			end = NULL;
		else if(link == NULL)
			end = img;
		else
			end = link;
		//last image 
		if(end == NULL)
		{
			char* r = strstr(resp, "Happy");
			//replace all "Happy" with "Silly"
			while(r != NULL)
			{
				strncpy(r, "Silly", 5);
				r = strstr(resp, "Happy");
			}
			r = strstr(resp, "happy");
			//replace all "happy" with "silly"
			while(r != NULL)
			{
				strncpy(r, "silly", 5);
				r = strstr(resp, "happy");
			}
			break;
		}
		//more images
		else
		{
			int l = strcspn(end, ">");
			//replace all occures (before image) of "Happy" with "Silly"
			char* u_ptr = strstr(resp, "Happy");
			while(u_ptr != NULL && it < end)
			{
				strncpy(u_ptr, "Silly", 5);
				u_ptr = strstr(resp, "Happy");
			}
			char* l_ptr = strstr(resp, "happy");
			while(l_ptr != NULL && it < end)
			{
				strncpy(l_ptr, "slly", 5);
				l_ptr = strstr(resp, "happy");
			}
			it = end + l;
		}
	}
}