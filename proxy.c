/*
 =====================================================================
 Name        : proxy.c
 Author      : Casey Meijer -- A00337010
 Version     : 1.0.0
 License     : MIT
 Description : A simple HTTP 1.0 proxy in the internet domain using TCP
               The port number is passed as an argument.

 =====================================================================
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

void error(const char *msg) {
    perror(msg);
    exit(1);
}

/* assert strlen(new) <= strlen(old) */
char * replace_str(char * source, char * old, char * new)
{
    char * start = strstr(source, old);
    if ( start == NULL || strlen(new) > strlen(old) )
        return source;
    size_t difference = strlen(old)-strlen(new);
    char * rest = start+strlen(old);
    memcpy(start, new, strlen(new));
    memmove(start+strlen(new),rest,strlen(rest));
    memset(source+strlen(source)-difference, 0, difference);

    return source;
}

char * repl_tween_str(char * source, char * start, char * end, char * new)
{
    start = strstr(source, start) + strlen(start);
    end = strstr(start, end);
    if(start == NULL || end == NULL)
        return source;
    size_t length = end-start;
    size_t difference = length - strlen(new);
    if (strlen(new) > length)
        return source;
    memcpy(start, new, strlen(new));
    memmove(start+strlen(new), end, strlen(end));
    memset(source+strlen(source)-difference, 0, difference);
    return source;
}

/* remember to free(result)!!! */
char * get_tween_str(char * source, char * start, char * end)
{
    start = strstr(source, start)+strlen(start);
    end = strstr(start, end);
    size_t length = end - start;
    char * result = malloc((length+1)*sizeof(char));
    memcpy(result, start, length);
    memset(result+length+1,0,1);
    return result;
}

int proxyserver(char * portstr)
{
    int sockfd = 0;
    int portno = 0;
    int reuse = 1;
    struct sockaddr_in serv_addr;

    bzero((char *) &serv_addr, sizeof(serv_addr));  // zero out server address

    sockfd = socket(AF_INET, SOCK_STREAM, 0);       // create a generic socket
    if (sockfd < 0)                                 // socket create failed
        error("ERROR opening socket");
    portno = atoi(portstr);                         // translate the only parameter into a port number
    
    serv_addr.sin_family = AF_INET;                 // set parameters - standard internet
    serv_addr.sin_addr.s_addr = INADDR_ANY;          
    serv_addr.sin_port = htons(portno);             // and the port number into network byte order
    if (setsockopt(sockfd, SOL_SOCKET,
     SO_REUSEADDR, &reuse, sizeof(int)) == -1) {
        close(sockfd);
        error("REUSEADDR failed");
    }
    if (bind(sockfd, (struct sockaddr *) &serv_addr,// to the (localhost) address to bind to
            sizeof(serv_addr)) < 0)                                    // check response code
        return -1;

    return sockfd;
}

int proxyclient(char ** host, char * portstr)
{
    int websock=0, portno=0, n=0;
    struct sockaddr_in host_addr;
    struct hostent *server;
    bzero((char *) &host_addr, sizeof(host_addr));

    server = gethostbyname(*host);              // translate the host name into an address
    if (server == NULL)
        error("ERROR, no such host\n");
    portno = atoi(portstr);                    // get the port number
    websock = socket(AF_INET, SOCK_STREAM, 0); // create a generic socket
    if (websock < 0)
        error("ERROR opening socket");
    host_addr.sin_family = AF_INET;            // it's an internet type
    bcopy((char *)server->h_addr,              // make sure we connect with the IP not name
        (char *)&host_addr.sin_addr.s_addr,
        server->h_length);
    host_addr.sin_port = htons(portno);        // port numbers should be in network byte order
    if (connect(websock,                       // connect socket to
            (struct sockaddr *) &host_addr,        // the server address (bind)
                sizeof(host_addr)) < 0)
        return -1;
    return websock;
}

int parse(char * request, 
            char ** host, char * portstr) {

    char * hostbegin = strstr(request, "Host:");                                                                                           
    if (strstr(request, "GET") == NULL                //                                  
            || hostbegin == NULL 
            || strstr(hostbegin, "\r\n") == NULL)       //
        return -1;
     
    *host = get_tween_str(request, "Host: ", "\r\n");
    char * porttemp = strstr(*host, ":");         //                                 
    if (porttemp == NULL)                
        strcpy(portstr, "80");                       //                 
    else{                                             //   
        *porttemp = '\0';                                 //                 
        porttemp += sizeof(char)*1; 
        strcpy(portstr,porttemp);                      //               
    }
    replace_str(request, "http://", "");
    replace_str(request, *host, "");
    repl_tween_str(request, "Accept-Encoding: ", "\r\n", "");
    repl_tween_str(request, *host, "\r\n\r\n", "");
    char * protocol = strstr(request, "HTTP/1.1");     //                                            
    if (protocol != NULL)                              //                   
        *(protocol+7) = '0';                           // HTTP 1.0 request   
    return 0;
}

//int transmit(int newsockfd){

//}

int main(int argc, char *argv[]) {
    // Setup Variables
    int sockfd = 0, newsockfd = 0, websock = 0;
    char request[80000], response[80000], portstr[6];
    struct sockaddr_in cli_addr;
    socklen_t clilen = sizeof(cli_addr);
    char * host;
    int n = 0; 
    
    // Zero things out                                                           
    memset(request,0,80000);
    memset(response,0,80000);
    memset(portstr,0, 6);

    // Verify args are present
    if (argc < 2)
        error("ERROR, no port provided");

    sockfd = proxyserver(argv[1]); 
    if (sockfd < 0){
        error("ERROR on binding");
    }

    listen(sockfd,5);
 
    while( (newsockfd = accept(sockfd,                    
            (struct sockaddr *) &cli_addr, &clilen)), newsockfd )
    {           
        if (newsockfd < 0)
            error("ERROR on accept");
        pid_t pID = fork();
////////// Child (move to transmit function??) ////
        if (pID == 0)
        {
            close(sockfd);

            n = read(newsockfd,request,80000); 
                if (n < 0)
                    error("ERROR reading from socket");
                printf("OriginalRequest: \n[%s]",request); 
                if (parse(request, &host, portstr)<0){
                    close(newsockfd);
                    error("ERROR: malformed GET request");     
                }

            printf("Host: %s\n", host);
            printf("Port: %s\n", portstr);
            printf("Request: \n[%s]",request);         
                websock = proxyclient(&host, portstr);
                free(host);

 
               if (websock < 0){
                    close(newsockfd);
                    error("ERROR connecting");
                }

                n = write(websock, request, strlen(request)+1);
                if (n < 0)
                    error("ERROR sending request"); 

                while(n = read(websock,response,79999), n)
                {           
                    if (n < 0){
                        close(newsockfd); close(websock);
                        error("ERROR getting response");
                    }
                
                //response[n]='\0';
                    n = write(newsockfd, response, n);
                    printf("Response: \n[%s]",response); 
                }
                close(sockfd);                 
                close(newsockfd);  

            exit(EXIT_SUCCESS);

////////// Error Forking ////////////////////////////////                                                  
        } else if (pID < 0){
            close(sockfd);            
            error("Error on forking");
////////// Parent //////////////////////////////////////
        }else close(newsockfd); 
    }
    close(sockfd);
    exit(EXIT_SUCCESS);
}
