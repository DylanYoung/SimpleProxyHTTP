/*
 =============================================================
 Name        : proxy.c
 Author      : Casey Meijer -- A00337010
 Version     : 1.0.0
 License     : MIT
 Description : A simple HTTP 1.0 proxy in internet using TCP.
               The port number is passed as an argument.

 =============================================================
 */
#include "setup.h"
#include "strlib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
//#include <fcntl.h>
#include <netinet/in.h>
#include <netdb.h>

int parse(char * request, 
            char ** host, char * portstr) {

    char * hostbegin = strstr(request, "Host:");
    if (
    (strstr(request, "GET") == NULL
        && strstr(request, "POST") == NULL)
        || hostbegin == NULL
            || strstr(hostbegin, "\r\n") == NULL)
        return -1;
     
    *host = get_tween_str(request, "Host: ", "\r\n");
    char * porttemp = strstr(*host, ":");
    if (porttemp == NULL)                
        strcpy(portstr, "80");
    else{ 
        *porttemp = '\0';            
        porttemp += sizeof(char)*1;
        strcpy(portstr,porttemp);
    }
    replace_str(request, "http://", "");
    replace_str(request, *host, "");
    // repl_tween_str(request, "Accept-Encoding: ", "\r\n", "");
    // Simplify the request for debugging
    //repl_tween_str(request, *host, "\r\n\r\n", "");
    char * protocol = strstr(request, "HTTP/1.1");
    if (protocol != NULL)
        *(protocol+7) = '0'; // HTTP 1.0 request   
    return 0;
}

int relay(int newsockfd){
    // Setup
    int websock = 0;
    char request[80000], response[80000], portstr[6];
    char * host;
    int n = 0; 

    //Receive Timeout - 5 sec
    struct timeval tv;
        tv.tv_sec = 5;
        // Not init'ing this can cause strange errors
        tv.tv_usec = 0;

    // Zero things out
    memset(request,0,80000);
    memset(response,0,80000);
    memset(portstr,0, 6);
    setsockopt(newsockfd, SOL_SOCKET, SO_RCVTIMEO, 
            (char *)&tv,sizeof(struct timeval));
    n = recv(newsockfd,request,80000,0); 
    if (n<0){
        close(newsockfd);
        error("ERROR reading from socket");
    }

    // Handle the possibility of split requests
    size_t req_len;
    for(req_len = n
            ; strstr(request,"\r\n\r\n") == NULL
            && req_len < 80000 && n
            ; req_len+=n){
        n = recv(newsockfd,
                    request+req_len,
                        80000-req_len,0);
        if (n<0){
            close(newsockfd);
            error("ERROR reading from socket");
        }
    }
//printf("OriginalRequest: \n[%s]",request); 

    // Parse request
    if (parse(request, &host, portstr)<0){
        close(newsockfd);
        error("ERROR: malformed GET request");
    }

//printf("Host: %s\n", host);
//printf("Port: %s\n", portstr);
//printf("Request: \n[%s]",request);         
    websock = proxyclient(&host, portstr);
    free(host);
    setsockopt(websock, SOL_SOCKET, SO_RCVTIMEO, 
                (char *)&tv,sizeof(struct timeval));

    if (websock < 0){
        close(newsockfd);
        error("ERROR connecting");
    }

    n = write(websock, request, strlen(request)+1);
    if (n < 0){
        close(newsockfd); close(websock);
        error("ERROR sending request");
    } 

    while(n = recv(websock,response,79999, 0), n)
    {           
        if (n < 0){
            close(newsockfd); close(websock);
            error("ERROR getting response");
        }
        //int flags = fcntl(websock, F_GETFL, 0);
        //if (flags == -1)
        //   error("ERROR setting non-blocking");
        //fcntl(websock, F_SETFL, flags | O_NONBLOCK);
        n = write(newsockfd, response, n);
//printf("Response: \n[%s]",response); 
    }
    close(newsockfd);  
    return(EXIT_SUCCESS);
}

int main(int argc, char *argv[]) {
    // Setup Variables
    int sockfd = 0, newsockfd = 0;
    struct sockaddr_in cli_addr;
    socklen_t clilen = sizeof(cli_addr);
    
    // Verify args are present
    if (argc < 2)
        error("ERROR, no port provided");

    sockfd = proxyserver(argv[1]); 
    if (sockfd < 0){
        error("ERROR on binding");
    }

    listen(sockfd,5);
 
    while( (newsockfd = accept(sockfd,                    
                (struct sockaddr *) &cli_addr, &clilen)), 
                newsockfd )
    {           
        if (newsockfd < 0)
            error("ERROR on accept");
        pid_t pID = fork();
////////// Child ////
        if (pID == 0)
        {
            close(sockfd);
            exit(relay(newsockfd));
////////// Error Forking ///////////////////////////
        } else if (pID < 0){
            close(sockfd);            
            error("Error on forking");
////////// Parent //////////////////////////////////
        }else close(newsockfd); 
    }
    close(sockfd);
    exit(EXIT_SUCCESS);
}
