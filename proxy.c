/*
 ============================================================================
 Name        : server.c
 Author      : open source
 Version     :
 Copyright   : no copyright
 Description : A simple server in the internet domain using TCP
               The port number is passed as an argument.

 ============================================================================
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

int proxyclient(char * host, char * portstr)
{
    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    bzero((char *) &serv_addr, sizeof(serv_addr));

    server = gethostbyname(host);                                     // translate the host name into an address
    if (server == NULL)
        error("ERROR, no such host\n");
    portno = atoi(portstr);                                              // get the port number
    sockfd = socket(AF_INET, SOCK_STREAM, 0);                            // create a generic socket
    if (sockfd < 0)
        error("ERROR opening socket");
    serv_addr.sin_family = AF_INET;                                      // it's an internet type
    bcopy((char *)server->h_addr,                                        // make sure we connect with the IP not name
        (char *)&serv_addr.sin_addr.s_addr,
        server->h_length);
    serv_addr.sin_port = htons(portno);                                  // port numbers should be in network byte order
    if (connect(sockfd,                                                  // connect socket to
        (struct sockaddr *) &serv_addr,                                  // the server address (bind)
        sizeof(serv_addr))                                               // where the server address structure is this length
        < 0)
        error("ERROR connecting");
    return sockfd;
}

int main(int argc, char *argv[]) {
    int sockfd = 0;
    int newsockfd = 0;
    int websocket = 0;
    int portno = 0;
    socklen_t clilen;
    struct sockaddr_in serv_addr;
    struct sockaddr_in cli_addr;
    char inbuffer[2048];
    char outbuffer[10240];
    int n;
    n = 0;                                                              // in C you should initialize all vars for safety
    bzero(inbuffer,2048);                                                // zero out input buffer
    bzero(outbuffer,256);                                      // set up response buffer
    bzero((char *) &serv_addr, sizeof(serv_addr));                      // zero out server address
    if (argc < 2)
        error("ERROR, no port provided\n");
    sockfd = socket(AF_INET, SOCK_STREAM, 0);                           // create a generic socket
    if (sockfd < 0)                                                     // socket create failed
        error("ERROR opening socket");
    portno = atoi(argv[1]);                                             // translate the only parameter into a port number
    serv_addr.sin_family = AF_INET;                                     // set parameters - standard internet
    serv_addr.sin_addr.s_addr = INADDR_ANY;                             //
    serv_addr.sin_port = htons(portno);                                 // and the port number into network byte order
    if (bind(sockfd,                                                    // connect up the new socket
            (struct sockaddr *) &serv_addr,                             // to the (localhost) address to bind to
            sizeof(serv_addr))                                          // size of the address structure
            < 0)                                                        // check response code
        error("ERROR on binding");
    while(1)
    {
        listen(sockfd,5);
                                                  // block and wait for a new connection from a client

        clilen = sizeof(cli_addr);                                          // get address of client
        newsockfd = accept(sockfd,                                          // translate the connection request into a new socket
                        (struct sockaddr *) &cli_addr,                          // you would normally leave the listener listening
                        &clilen);
        pid_t pID = fork();
        if (pID == 0)
        {
            close(sockfd);
            if (newsockfd < 0)
                 error("ERROR on accept");
            //char * temp = malloc(sizeof(char)*2048);
            //while (strstr(temp, "\r\n\r\n") == NULL)
            //{
                n = read(newsockfd,inbuffer,2048); 
                //strcat(temp, inbuffer);                                  // get the string sent from the client
                if (n < 0)
                    error("ERROR reading from socket");
            //}
            //strcpy(inbuffer, temp);
            //free(temp);
            // Parsing //////////////////////////////////////////                                                 
            char * hostbegin = strstr(inbuffer, "Host:")+ 6;   //                                              
            char * hostend = strstr(hostbegin, "\r\n");        //                                         
            //printf("Begin: %s\nEnd: %s", hostbegin, hostend);//                                                  
            if (strstr(inbuffer, "GET") == NULL                //                                  
                || hostbegin == NULL || hostend == NULL)       //                                          
                error("ERROR: malformed GET request");         //                                        
            int length = (hostend - hostbegin) + 2;            //                                     
            char * host = malloc(sizeof(char)*(length));       //                                          
            bzero(host,length);                                //                 
            strlcpy(host, hostbegin, length-1);                //                                 
            char * portstr = strstr(host, ":");                //                                 
            if (portstr == NULL)                               //                  
                portstr = "80";                                //                 
            else{                                              //   
                portstr[0] = 0;                                //                 
                portstr += 1;                                  //               
            } 
            char * toRemove = strstr(inbuffer, "http");
            hostend = strstr(inbuffer, host) + length - 2;
            for(toRemove; toRemove<hostend; toRemove++)
                *toRemove = 0;
            length = strlen(inbuffer)+
                strlen(toRemove)+1;
            char * request = malloc(strlen(inbuffer)+
                strlen(toRemove)+1);
            bzero(request, length);
            strcpy(request, inbuffer);
            strcat(request, toRemove);

                                                             //
            // End Parsing //////////////////////////////////////                                                
            
            printf("Host: %s\n", host);
            printf("Port: %s\n", portstr);
            printf("Request: [%s]\n",request); 
            websocket = proxyclient(host, portstr); 
            n = write(websocket, request, strlen(request+1)); 
            //websocket = proxyclient("www.google.com", "80");
            //n = write(websocket, "GET / HTTP/1.1", 14);
            if (n < 0)
                error("ERROR sending request"); 
            //while(1)
            //{                            // and dump it to string
            n = read(websocket,outbuffer,strlen(outbuffer+1));                 // then send it back
            if (n < 0)
                error("ERROR getting response");
            printf("Received: [%s]\n",outbuffer);
            //}                               // dump back what we are just send back
            close(newsockfd);                                                   // close accepted socket                                                     // close listener socket
            return EXIT_SUCCESS;                                                   // then get out
        } else if (pID < 0)
            error("Error on forking");
        else close(newsockfd);
    }
}