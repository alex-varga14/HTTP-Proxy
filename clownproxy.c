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
#define MAX_MESSAGE_SIZE 1024
#define MAX_USERS 100

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

// Global Variables:
int server_socket, conn_socket, client_socket;
uint16_t port;
char clown_jpg[] = "GET http://pages.cpsc.ucalgary.ca/~carey/CPSC441/ass1/clown1.png HTTP/1.1";

struct clientRequestInfo {
	char* protocol;
    char* content_type;
    char* content_length;
	int body_size;
    char* modified;
    char* dest;
};

struct parsedRequest 
{
	char *receive_ptr;
    char *GET_request;
    char *header;
    char *host;
    char* end_of_line;
    int msg_length;
};
	
	
int main (int argc, char *argv[]){

    // address_server init variables
    struct sockaddr_in server_address;
	char client_request[MAX_MESSAGE_SIZE];
	char server_request[MAX_MESSAGE_SIZE];
	
	struct clientRequestInfo reqInfo;
	struct parsedRequest parsedRed;
	

    //client proxy variables
    struct addrinfo hints;
    struct addrinfo *res;
    struct addrinfo *attempt_connect;
	
	//check for valid input of the form:
	// ./clownproxy <PORT#>
	if(argc != 2)
	{
		fprintf(stderr, "ERROR: Clown Proxy needs a Port Number!\n");
		exit(1);
	}
	port = atoi(argv[1]);
    startProxy(&server_address, &server_socket, port);

  
    if (bind(server_socket, (struct sockaddr *) &server_address, sizeof(struct sockaddr_in)) == -1){
        close(server_socket);
        fprintf(stderr, "ERROR: Bind() call failed!\n");
        exit(1);
    }

    /* Listening/Accept/Main Loop */
    for( ; ; )
	{
        if (listen(server_socket, 100) == -1){
            close(server_socket);
            fprintf(stderr, "ERROR: Listen() call failed!\n");
			exit(1);
        }

        /* accept connection */
        conn_socket = accept(server_socket, NULL, NULL);
        if (conn_socket == -1){
            close(server_socket);
            fprintf(stderr, "ERROR: accept() call failed!\n");
			exit(1);
        }

        printf("accepted connection\n");

        /* Receive GET request */
        parsedRed.receive_ptr = (char*)malloc(1000 * sizeof(char));
        memset(parsedRed.receive_ptr, 0, 1000);
        if (recv(conn_socket, parsedRed.receive_ptr, sizeof(char) * 1000, 0) == -1){
            close(server_socket);
            close(conn_socket);
            fprintf(stderr, "ERROR: recv() call 1 failed!\n");
			exit(1);
        }

        printf("recieved something........ standby\n");
        printf("message received:\n%s", parsedRed.receive_ptr);

        /* Parse GET Request */
        parsedRed.GET_request = (char*)malloc(4000 * sizeof(char));
        memset(parsedRed.GET_request, 0, 4000);
        parsedRed.host = (char*)malloc(100 * sizeof(char));
        memset(parsedRed.host, 0, 100);

        // Only Accept requests in the form of "GET"
       parsedRed. header = strstr(parsedRed.receive_ptr, "GET");
        if (parsedRed.header == NULL){
            close(server_socket);
            close(conn_socket);
            fprintf(stderr, "ERROR: HTTP method invalid!\n");
			exit(1);
        }

        parsedRed.host = parseHost(parsedRed.receive_ptr);

        /* make sure we haven't loaded the page recently */
        char* range;
        reqInfo.modified = NULL; 
        range = NULL;
        reqInfo.modified = strstr(parsedRed.receive_ptr, "If-Modified-Since: ");
        range = strstr(parsedRed.receive_ptr, "Range: ");
        if (reqInfo.modified != NULL){
            strncpy(parsedRed.GET_request, parsedRed.receive_ptr, (reqInfo.modified - parsedRed.receive_ptr));
            reqInfo.modified = strstr(reqInfo.modified, "If-None-Match: ");
            reqInfo.modified = strchr(reqInfo.modified, '\n') + 1;
            strcat(parsedRed.GET_request, reqInfo.modified);
        }
        else if (range != NULL){
            strncpy(parsedRed.GET_request, parsedRed.receive_ptr, (range - parsedRed.receive_ptr));
            range = strstr(range, "If-Range: ");
            range = strchr(range, '\n') + 1;
            strcat(parsedRed.GET_request, range);
        }
        else{
            strcpy(parsedRed.GET_request, parsedRed.receive_ptr);
        }
        parsedRed.GET_request[strlen(parsedRed.GET_request)] = '\0';

        printf("\n%s\n", parsedRed.GET_request);

        // don't need receive pointer anymore
        free(parsedRed.receive_ptr);  
        parsedRed.receive_ptr = NULL;

        /* Pass to client side */
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_flags = AI_PASSIVE;


        if ((getaddrinfo(parsedRed.host, "http", &hints, &res) != 0)){
            close(server_socket);
            close(conn_socket);
            printf("get addr error: \n", getaddrinfo(parsedRed.host, "http", &hints, &res));
            exit(1);
        }
        printf("got address info\n");

        /* Connection Request */
        // loop through results and connect to the first socket we can
        attempt_connect = res;
        for (attempt_connect = res; attempt_connect != NULL; attempt_connect = attempt_connect->ai_next){
            if ((client_socket = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == -1){
                printf("client: socket\n");
                continue;
            }
            if ((connect(client_socket, res->ai_addr, res->ai_addrlen)) == -1){
                close(client_socket);
                printf("client: connect\n");
                continue;
            }
            break;
        }

        if (attempt_connect == NULL){
            close(server_socket);
            close(conn_socket);
            fprintf(stderr, "ERROR: socket() call failed!\n");
			exit(1);
        }

        printf("connected\n");

        /* Send GET */
        parsedRed.msg_length = strlen(parsedRed.GET_request);
        if (send(client_socket, parsedRed.GET_request, parsedRed.msg_length, 0) == -1){
            close(server_socket);
            close(conn_socket);
            close(client_socket);
            fprintf(stderr, "ERROR: send() call failed!\n");
			exit(1);
        }

        /* Receive Response && Handle */
        reqInfo.dest = (char*)malloc(65535 * sizeof(char));
        memset(reqInfo.dest, 0, 65535);
        parsedRed.receive_ptr = (char*)malloc(512 * sizeof(char)); // make sure this is big enough
        memset(parsedRed.receive_ptr, 0, 512);
        if (recv(client_socket, parsedRed.receive_ptr, sizeof(char) * 512, 0) == -1){
            close(server_socket);
            close(conn_socket);
            close(client_socket);
            fprintf(stderr, "ERROR: recv() call 2 failed!\n");
			exit(1);
        }

        printf("recieved something........ standby\n");
        printf("message received:\n%s", parsedRed.receive_ptr);

        /* Parse response and insert errors */
        char* end_of_response = NULL;
        int sanity = 0;
        reqInfo.content_type = NULL;
        reqInfo.content_length = NULL;

        reqInfo.protocol = strstr(parsedRed.receive_ptr, "HTTP");
        reqInfo.content_type = strstr(parsedRed.receive_ptr, "Content-Type: ");
        reqInfo.content_length = strstr(parsedRed.receive_ptr, "Content-Length: ");
        reqInfo.body_size = actualLength(reqInfo.content_length);
        end_of_response = strstr(parsedRed.receive_ptr, "\r\n\r\n") + 4;
        sanity = (end_of_response - parsedRed.receive_ptr);
        strncpy(reqInfo.dest, parsedRed.receive_ptr, sanity);
        if(strstr(reqInfo.protocol, "404 Not Found") == NULL){
            if(strstr(reqInfo.content_type, "image") == NULL){
                strcat(reqInfo.dest, end_of_response);
                reqInfo.body_size -= strlen(parsedRed.receive_ptr);
                reqInfo.body_size += sanity;
                
                while (reqInfo.body_size > 0){
                    if (recv(client_socket, parsedRed.receive_ptr, MIN((sizeof(char) * 512), reqInfo.body_size), 0) == -1){
                        close(server_socket);
                        close(conn_socket);
                        close(client_socket);
                        fprintf(stderr, "ERROR: recv() call 3 failed!\n");
						exit(1);
                    }
                    strncat(reqInfo.dest, parsedRed.receive_ptr, MIN((int)strlen(parsedRed.receive_ptr), reqInfo.body_size));
                    reqInfo.body_size -= MIN((int)strlen(parsedRed.receive_ptr), reqInfo.body_size);
                }


                /* Checking for html type */
                if (strstr(reqInfo.content_type, "html") != NULL){
                    printf("got into html\n");
                    // this function will add the errors
                    handleHTML(reqInfo.dest);
                } 

                /* Check for Plain text type */
                if (strstr(reqInfo.content_type, "plain") != NULL){
                    handleText(reqInfo.dest);
                }
            }
            else{
                /* This is where images would be handled, but it is not working......... */
                 memcpy(reqInfo.dest + sanity, parsedRed.receive_ptr, 512 - sanity);
                sanity += (512 - sanity);
                while (reqInfo.body_size > 0){

                    if (recv(client_socket, parsedRed.receive_ptr, MIN(512, reqInfo.body_size), 0) == -1){
                        close(server_socket);
                        close(conn_socket);
                        close(client_socket);
                        fprintf(stderr, "ERROR: recv() call 4 failed!\n");
						exit(1);
                    }
                    memcpy(reqInfo.dest + sanity, parsedRed.receive_ptr, MIN(512, reqInfo.body_size));
                    sanity += MIN(512, reqInfo.body_size);
                    reqInfo.body_size -= MIN((int)strlen(parsedRed.receive_ptr), reqInfo.body_size);
                } 

            }
        }

        printf("\n\nsending:\n%s\n", reqInfo.dest);

        parsedRed.msg_length = strlen(reqInfo.dest);
        if (send(conn_socket, reqInfo.dest, parsedRed.msg_length, 0) == -1){
            close(server_socket);
            close(conn_socket);
            close(client_socket);
            fprintf(stderr, "ERROR: send() call failed!\n");
			exit(1);
        }

        /* Close superfluous sockets */
        close(client_socket);
        close(conn_socket);


        /* deallocate memory */
        if (reqInfo.modified != NULL)
            printf("%p: modified pointer\n", (void*)reqInfo.modified);{
        }
        printf("%p: receive pointer\n", (void*)parsedRed.receive_ptr);
        free(parsedRed.receive_ptr);
        parsedRed.receive_ptr = NULL;
		
        printf("%p: GET pointer\n", (void*)parsedRed.GET_request);
        strcpy(parsedRed.GET_request, "");
        printf("%p: content pointer\n", (void*)reqInfo.content_type);
        reqInfo.content_type = NULL;
        printf("%p: length pointer\n", (void*)reqInfo.content_length);
        reqInfo.content_length =NULL;
        printf("%p: header pointer\n", (void*)parsedRed.header);
        parsedRed.header = NULL;
        printf("%p: host pointer\n", (void*)parsedRed.host);
        free(parsedRed.host);
    }

    /* Terminate Connection */
    close(server_socket);

    return 0;
}

void startProxy(struct sockaddr_in *address, int *sockfd, int port){

    memset(address, 0, sizeof(&address)); //initialize address_server to zero
    address->sin_family = AF_INET;  //AF_INET == IPv4 protocol
    address->sin_port = htons(port); //convert little endian to big endian
    address->sin_addr.s_addr = htonl(INADDR_ANY); //IADDR_ANY == any local IP

    *sockfd = socket(AF_INET, SOCK_STREAM, 0);
    printf("socket %d opened\n", *sockfd);
    if (*sockfd < 0){
         fprintf(stderr, "ERROR: socket() call failed!\n");
		 exit(1);
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