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
#include <time.h>

/* Global manifest constants */
#define MAX_MESSAGE_LENGTH 1024
#define MAX_USERS 100
#define PORT 80

#define MAX(a,b)((a>b)?a:b)
#define MIN(a,b)((a<b)?a:b)

/*

	if()
		printf("That is a JPG URl that needs specific handling!\n");
	else printf("That URL looks fine to me!\n");
	
	bzero(PATH, 500); //to clear junk at the beginning of this buffer
	for(; i < strlen(URL); i++)
	{
		strcat(PATH, &URL[i]); //copy out the path
		break;
	}
	
	printf("First half: %s\n", HOST); //firstHalf is the hostname
	printf("Second half: %s\n", PATH); //secondalf is the path
	if( baddie )
	{
		printf("That is a request that needs substituition!\n"); 
		strcpy(PATH, "/~carey/CPSC441/ass1/pansage.gif");
		strcpy(PATH, "/~carey/CPSC441/ass1/clown1.gif");
		printf("Revised Second half: %s\n", PATH); //secondalf is the path
	}
	
	//connect to the Web server using TCP and send the HTTP request
	tcpSocket = makeTCPconn(HOST, HTTP_PORT);
	if( tcpSocket > 0)
		sendHTTPrequest(tcpSocket, HOST, PATH, 0, -1);
	else printf("Failed to create TCP socket!\n");
	
	bzero(header, MAX_MESSAGE_SIZE);
	headersize = 0;
	inHeader = 1;
	readBytes = 0;
	
	*/

void handleText(char* resp);
void handleHTML(char* resp);
char* parseHost(char* request);
int actualLength(char* content_length);
void startProxy(struct sockaddr_in *address, int *prox_socket, int port);

			
int main(int argc, char *argv[])
{
	// address_server init variables
    struct sockaddr_in server_address;
	//struct sockaddr_in client_address;
    uint16_t port;
	// socket vars
    int server_socket;
	int accept_socket;
    int client_socket;
	char* messageIn;
	char* messageout;
	char* GET_req;
	char* host;
	char* header;
	
	
	struct addrinfo client_address;
	struct addrinfo *res;
	struct addrinfo *tmp;
	int addr_status;
	
	int msg_length;
	int bytes_sent;
	int bytes_rcvd;
	
	int conn_status;
	
	//send vars
	char* content_type;
    char* content_length;
    char* sent_protocol;
    char* modified;
    char* send_ptr;
    int body_length;
	
	
	//check for valid input of the form:
	// ./clownproxy <PORT#>
	// if(argc != 2)
	// {
		// fprintf(stderr, "ERROR: Clown Proxy needs a Port Number!\n");
		// exit(1);
	// }
	port = 6666;
	startProxy(&server_address, &server_socket, port);
	/*
	memset(&server_address, 0, sizeof(&server_address)); 
    server_address.sin_family = AF_INET; 
    server_address.sin_port = htons(PORT);
    server_address.sin_addr.s_addr = htonl(INADDR_ANY); //IADDR_ANY to local IP
	
	// Create client socket, check for errors 
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    printf("Socket %d Created\n", server_socket);
    if (server_socket < 0)
	{
       fprintf(stderr, "ERROR: Socket() call failed!\n");
       exit(1);
    } */
	
	/* Binding Sockets on Server Side */
    if (bind(server_socket, (struct sockaddr *) &server_address, sizeof(struct sockaddr_in)) == -1){
        close(server_socket);
        fprintf(stderr, "ERROR: Bind() call failed!\n");
        exit(1);
    }
	//listen on socket
	if(listen(server_socket, MAX_USERS) == -1 ) 
	{
		fprintf(stderr, "ERROR: Listen() call failed!\n");
		exit(1);
	}
	//listen for requests
	
	socklen_t client_address_size = sizeof(client_address);
	
	for( ; ;)
	{
		//not sure of secondary parameters
		client_socket = accept(server_socket, NULL, NULL);
		//client_socket = accept(server_socket, &client_address, &client_address_size);
		if(client_socket == -1 ) // up to 100 participants can connect
		{
			close(server_socket);
			fprintf(stderr, "ERROR: Listen() call failed!\n");
			exit(1);
		}
		
		// int pid = fork();
		
		//receiver browser message
		// if(pid == 0)
		// {
			// clientData((void*)&client_socket);
			// close(client_socket);
			// exit(0);
		// }
		// else
			// close(client_socket);
		
		// receive GET request
		messageIn = (char*)malloc(1000 * sizeof(char));
		memset(messageIn, 0, 1000);
		if(recv(client_socket, messageIn, sizeof(messageIn) * 1000, 0) == -1)
		{
			close(server_socket);
			close(client_socket);
			fprintf(stderr, "ERROR: recv() call 1 failed!\n");
			exit(1);
		}
		
		//parse GET request
		GET_req = (char*) malloc(4000 * sizeof(char));
		memset(GET_req, 0, 4000);
		host = (char*) malloc(100 * sizeof(char));
		memset(host, 0, 100);
		
		// Only accept requests in the form of GET
		header = strstr(messageIn, "GET");
		if(header == NULL)
		{
			close(server_socket);
			close(client_socket);
			fprintf(stderr, "ERROR: HTTP method invalid!\n");
			exit(1);
		}
		
		host = parseHost(messageIn);
		
		/* make sure we haven't loaded the page recently */
        char* range;
        modified = NULL; 
        range = NULL;
        modified = strstr(messageIn, "If-Modified-Since: ");
        range = strstr(messageIn, "Range: ");
        if (modified != NULL){
            strncpy(GET_req, messageIn, (modified - messageIn));
            modified = strstr(modified, "If-None-Match: ");
            modified = strchr(modified, '\n') + 1;
            strcat(GET_req, modified);
        }
        else if (range != NULL){
            strncpy(GET_req, messageIn, (range - messageIn));
            range = strstr(range, "If-Range: ");
            range = strchr(range, '\n') + 1;
            strcat(GET_req, range);
        }
        else{
            strcpy(GET_req, messageIn);
        }
        GET_req[strlen(GET_req)] = '\0';

        printf("\n%s\n", GET_req);

        // don't need receive pointer anymore
        free(messageIn);  
        messageIn = NULL;

        /* Pass to client side */
        memset(&client_address, 0, sizeof(client_address));
        client_address.ai_family = AF_INET;
        client_address.ai_socktype = SOCK_STREAM;
        client_address.ai_flags = AI_PASSIVE;

        addr_status = getaddrinfo(host, "http", &client_address, &res);
        if (addr_status != 0){
            close(server_socket);
            close(client_socket);
            printf("get addr error: %s\n", gai_strerror(addr_status));
            exit(1);
        }
        printf("got address info\n");

        /* Connection Request */
        // loop through results and connect to the first socket we can
        tmp = res;
        for (tmp = res; tmp != NULL; tmp = tmp->ai_next){
            if ((accept_socket = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == -1){
                printf("client: socket\n");
                continue;
            }
            if ((conn_status = connect(accept_socket, res->ai_addr, res->ai_addrlen)) == -1){
                close(accept_socket);
                printf("client: connect\n");
                continue;
            }
            break;
        }

        if (tmp == NULL){
            close(server_socket);
            close(client_socket);
            printf("get addr error: %s\n", gai_strerror(addr_status));
            exit(1);
        }
		printf("connected\n");

        /* Send GET */
        msg_length = strlen(GET_req);
        bytes_sent = send(accept_socket, GET_req, msg_length, 0);
        if (bytes_sent == -1){
            close(server_socket);
			close(client_socket);
            close(accept_socket);
            fprintf(stderr, "ERROR: send() call failed!\n");
			exit(1);
        }

        /* Receive Response && Handle */
        send_ptr = (char*)malloc(65535 * sizeof(char));
        memset(send_ptr, 0, 65535);
        messageIn = (char*)malloc(512 * sizeof(char)); // make sure this is big enough
        memset(messageIn, 0, 512);
        bytes_rcvd = recv(client_socket, messageIn, sizeof(char) * 512, 0);
        if (bytes_rcvd == -1){
            close(server_socket);
            close(accept_socket);
            close(client_socket);
            fprintf(stderr, "ERROR: recv() call 2 failed!\n");
			exit(1);
        }

        printf("recieved something........ standby\n");
        printf("message received:\n%s", messageIn);

        /* Parse response and insert errors */
        char* end_of_response = NULL;
        int sanity = 0;
        content_type = NULL;
        content_length = NULL;

        sent_protocol = strstr(messageIn, "HTTP");
        content_type = strstr(messageIn, "Content-Type: ");
        content_length = strstr(messageIn, "Content-Length: ");
        body_length = actualLength(content_length);
        end_of_response = strstr(messageIn, "\r\n\r\n") + 4;
        sanity = (end_of_response - messageIn);
        strncpy(send_ptr, messageIn, sanity);
		 printf("message received222222222:\n%s", messageIn);
        if(strstr(sent_protocol, "404 Not Found") == NULL){
            if(strstr(content_type, "image") == NULL){
                strcat(send_ptr, end_of_response);
                body_length -= strlen(messageIn);
                body_length += sanity;
                
                while (body_length > 0){
                    bytes_rcvd = recv(accept_socket, messageIn, MIN((sizeof(char) * 512), body_length), 0);
                    if (bytes_rcvd == -1){
						close(client_socket);
                        close(server_socket);
                        close(accept_socket);
                        fprintf(stderr, "ERROR: recv() call 3 failed!\n");
						exit(1);
                    }
                    strncat(send_ptr, messageIn, MIN((int)strlen(messageIn), body_length));
                    body_length -= MIN((int)strlen(messageIn), body_length);
                }


                /* Checking for html type */
                if (strstr(content_type, "html") != NULL){
                    printf("got into html\n");
                    // this function will add the errors
                    handleHTML(send_ptr);
                } 

                /* Check for Plain text type */
                if (strstr(content_type, "plain") != NULL){
                    handleText(send_ptr);
                }
            }
            else{
                /* This is where images would be handled, but it is not working......... */
                 memcpy(send_ptr + sanity, messageIn, 512 - sanity);
                sanity += (512 - sanity);
                while (body_length > 0){
                    bytes_rcvd = recv(accept_socket, messageIn, MIN(512, body_length), 0);
                    if (bytes_rcvd == -1){
                        close(server_socket);
                        close(accept_socket);
                        close(client_socket);
                        fprintf(stderr, "ERROR: recv() call 4 failed!\n");
						exit(1);
                    }
                    memcpy(send_ptr + sanity, messageIn, MIN(512, body_length));
                    sanity += MIN(512, body_length);
                    body_length -= MIN((int)strlen(messageIn), body_length);
                } 

            }
        }

        printf("\n\nsending:\n%s\n", send_ptr);

        msg_length = strlen(send_ptr);
        bytes_sent = send(client_socket, send_ptr, msg_length, 0);
        if (bytes_sent == -1){
            close(server_socket);
			close(client_socket);
            close(accept_socket);
            fprintf(stderr, "ERROR: send() call failed!\n");
			exit(1);
        }

        /* Close superfluous sockets */
		   close(accept_socket);
        close(client_socket);


        /* deallocate memory */
        if (modified != NULL)
            printf("%p: modified pointer\n", (void*)modified);{
        }
        printf("%p: receive pointer\n", (void*)messageIn);
        free(messageIn);
        messageIn = NULL;
        printf("%p: GET pointer\n", (void*)GET_req);
        strcpy(GET_req, "");
        printf("%p: content pointer\n", (void*)content_type);
        content_type = NULL;
        printf("%p: length pointer\n", (void*)content_length);
        content_length =NULL;
        printf("%p: header pointer\n", (void*)header);
        header = NULL;
        printf("%p: host pointer\n", (void*)host);
        free(host);
	}
	close(server_socket);
}

void startProxy(struct sockaddr_in *address, int *prox_socket, int port){

    memset(address, 0, sizeof(&address)); //initialize address_server to zero
    address->sin_family = AF_INET;  //AF_INET == IPv4 protocol
    address->sin_port = htons(port); //convert little endian to big endian
    address->sin_addr.s_addr = htonl(INADDR_ANY); //IADDR_ANY == any local IP

    /* Create client socket, check for errors */
    *prox_socket = socket(AF_INET, SOCK_STREAM, 0);
    printf("socket %d opened\n", *prox_socket);
    if (*prox_socket == -1){
         fprintf(stderr, "ERROR: Socket() call failed!\n");
		 exit(1);
    }
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
		char* image = strstr(it, "<img src=\""); //next image tag
		char* link = strstr(it, "<a href=\""); //next link tag
		char* end = image; //end set to image
		
		//set end to link if link comes before image
		if(link < image)
			end = link;
		
		// ensure end if not NULL unless link and image are NULL
		if(link == NULL && image == NULL)
			end = NULL;
		else if(link == NULL)
			end = image;
		else
			end = link;
		
		//last image found
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
		//more images in filelength
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

char* parseHost(char* request){
    // create pass back variable
    char* host;

    // create helper vars
    char* GET_line;
    char* after_host;
    char* second_line;

    /* Find the Host: name */
    GET_line = strchr(request, '\n');
    after_host = strchr(GET_line, ' ') + 1;
    second_line = strchr(GET_line, 13); // carriage return == 13

    // allocate memory for the host
    host = (char*)malloc((second_line - after_host) * sizeof(char));

    // copy everythinginto the return
    strncpy(host, after_host, (second_line - after_host));

    // add NULL terminator
    host[strlen(host)] = '\0';

    return host;
}

int actualLength(char* content_length){

    // create pass back
    int actual;

    // helper variables
    char* start;
    char* end;
    char* length;

    // find the ' ' and the CR
    start = strchr(content_length, ' ') + 1;
    end = strchr(content_length, '\r');

    // allocate
    length = (char*)malloc((end - start)*sizeof(char));

    // copy the line
    strncpy(length, start, (end - start));

    // add NULL terminator
    actual = atoi(length);

    return(actual);

}


			
		
		
		
		
		