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
    #ifdef DEBUG
        repl_tween_str(request, "Accept-Encoding: ", "\r\n", "");
    #endif
    char * protocol = strstr(request, "HTTP/1.1");
    if (protocol != NULL)
        *(protocol+7) = '0'; // HTTP 1.0 request   
    return 0;
}

int checkcache(int outsock, char * request, char * response){
    int cache = 0, n = 0;
    cache = proxyclient("localhost", "23456");
    if (cache < 0) return -1;
    n = send(cache, request, 80000, WAIT_ALL);
    if (n < 0) {close(cache);return -1;}
    n = recv(cache, response, 9, WAIT_ALL);
    if (strcmp(response, "FOUND") != 0))
        return cache
    if(relay(cache, outsock, 0, response)<0)
        return -1;
    return 0;


}

int relay(int insock,int outsock,int altsock, char * response){
    while(n = recv(insock,response,79999, 0), n)
    {           
        if (n < 0){
            close(outsock); close(insock);
            if (altsock>0) close(altsock);
            return -1;
        }
        send(outsock, response, n, WAIT_ALL);
        if(altsock > 0) n = send(altsock, response, n, WAIT_ALL)
        #ifdef DEBUG
            printf("Response: \n{BEGIN}\n%s\n{END}\n",response);
        #endif
        return 0;
    }
}

int handlerequest(int newsockfd){
    // Setup
    int websock = 0, cache = 0;
    char request[80000], response[80000], portstr[6];
    char * host;
    int n = 0; 

    //Receive Timeout - 5 sec
    struct timeval tv;
        tv.tv_sec = 5;
        tv.tv_usec = 0;

    // Zero things out
    memset(request,0,80000);
    memset(response,0,80000);
    memset(portstr,0, 6);
    setsockopt(newsockfd, SOL_SOCKET, SO_RCVTIMEO, 
            (char *)&tv,sizeof(struct timeval));
    while((n = recv(newsockfd,request,80000,0)), n) 
    {
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

        #ifdef DEBUG  
            printf("Original Request: \n{BEGIN}\n%s\n{END}\n",request);  
        #endif       


        // Parse request --> host will be malloc'd
        if (parse(request, &host, portstr)<0){
            close(newsockfd);
            error("ERROR: malformed GET request");
        }
        #ifdef DEBUG  
            printf("Host: %s\n", host);
            printf("Port: %s\n", portstr);
            printf("Parsed Request: \n{BEGIN}\n%s\n{END}\n",request);  
        #endif 

        cache = checkcache(newsockfd, request);

        if (cache != 0)
            websock = proxyclient(host, portstr);
            free(host);
            setsockopt(websock, SOL_SOCKET, SO_RCVTIMEO, 
                    (char *)&tv,sizeof(struct timeval));

            if (websock < 0){
                close(newsockfd);
                error("ERROR connecting");

            n = send(websock, request, strlen(request)+1, WAIT_ALL);
            if (n < 0){
                close(newsockfd); close(websock);
                error("ERROR sending request");
            } 
            if(relay(websock, newsockfd, cache, response) < 0)
                error("ERROR getting response");
        }
    }
    close(newsockfd);  
    return(EXIT_SUCCESS);
}

int main(int argc, char *argv[]) {
    // Setup Variables
    int sockfd = 0, newsockfd = 0;
    struct sockaddr_in cli_addr;
    socklen_t clilen = sizeof(cli_addr);
    
    #ifdef DEBUG
        sockfd = proxyserver("12345");
    #else
        // Verify args are present
        if (argc < 2)
            error("ERROR, no port provided");
        sockfd = proxyserver(argv[1]);
    #endif

 
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
            exit(handlerequest(newsockfd));
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
