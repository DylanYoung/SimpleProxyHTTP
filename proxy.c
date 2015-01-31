/*
 =====================================================================
 Name        : proxy.c
 Author      : open source
 Version     : 0.0.1
 License     : MIT
 Description : A simple proxy in the internet domain using TCP
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

int proxyclient(char * host, char * portstr)
{
    int websock=0, portno=0, n=0;
    struct sockaddr_in host_addr;
    struct hostent *server;
    bzero((char *) &host_addr, sizeof(host_addr));

    server = gethostbyname(host);              // translate the host name into an address
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
        sizeof(host_addr))                     // where the server address structure is this length
        < 0)
        error("ERROR connecting");
    return websock;
}

int main(int argc, char *argv[]) {
////// Setup Proxy to Listen for Connections /////////////
    int sockfd = 0;
    int newsockfd = 0;
    int websock = 0;
    int portno = 0;
    socklen_t clilen;
    struct sockaddr_in serv_addr;
    struct sockaddr_in cli_addr;
    char inbuffer[80000];
    char outbuffer[80000];
    int n = 0; 
                                                                  
    bzero(inbuffer,80000);                           // zero out input buffer
    bzero(outbuffer,80000);                         // set up response buffer
    bzero((char *) &serv_addr, sizeof(serv_addr));  // zero out server address

    if (argc < 2)
        error("ERROR, no port provided\n");
    sockfd = socket(AF_INET, SOCK_STREAM, 0);       // create a generic socket
    if (sockfd < 0)                                 // socket create failed
        error("ERROR opening socket");
    portno = atoi(argv[1]);                         // translate the only parameter into a port number
    
    serv_addr.sin_family = AF_INET;                 // set parameters - standard internet
    serv_addr.sin_addr.s_addr = INADDR_ANY;          
    serv_addr.sin_port = htons(portno);             // and the port number into network byte order
    if (bind(sockfd, (struct sockaddr *) &serv_addr,// to the (localhost) address to bind to
            sizeof(serv_addr))                      // size of the address structure
            < 0)                                    // check response code
        error("ERROR on binding");
////// End Setup /////////////////////////////////////////
    listen(sockfd,5);
    while(1)
    {
                                                       // block and wait for a new connection from a client
        clilen = sizeof(cli_addr);                     // get address of client
        newsockfd = accept(sockfd,                     // translate the connection request into a new socket
                        (struct sockaddr *) &cli_addr, 
                        &clilen);
        pid_t pID = fork();
        if (pID == 0)
        {
            close(sockfd);
            if (newsockfd < 0)
                 error("ERROR on accept");
            n = read(newsockfd,inbuffer,80000); 
            if (n < 0)
                error("ERROR reading from socket");
        
////////////// Parsing //////////////////////////////////////////                                                 
            char * hostbegin = strstr(inbuffer, "Host:")+ 6;   //                                              
            char * hostend = strstr(hostbegin, "\r\n");        //                                                                                           
            if (strstr(inbuffer, "GET") == NULL                //                                  
                || hostbegin == NULL || hostend == NULL)       //
            {       //                                         // 
                close(newsockfd);                              //
                error("ERROR: malformed GET request");         //
            }         
            //*(hostend+2) = 0;                                         //                                        
            int length = (hostend - hostbegin) + 2;            //                                     
            char * host = malloc(sizeof(char)*(length));       //                                          
            bzero(host,length);                                //
                                                               //
                                                               //                 
            strlcpy(host, hostbegin, length-1);                //                                 
            char * portstr = strstr(host, ":");                //                                 
            if (portstr == NULL)
            {
                portstr = malloc(sizeof(char)*3);              //                  
                strcpy(portstr, "80\0");                       //                 
            }else{                                        //   
                *portstr = 0;                                  //                 
                portstr += sizeof(char)*1; 
                char * porttemp = malloc(
                    sizeof(char)*(host+length-portstr)
                    );
                strcpy(porttemp,portstr);
                portstr = porttemp;                                //               
            }                                                  //
            char * toRemove = strstr(inbuffer, "http");        //                                         
            hostend = strstr(inbuffer, host) + length - 2;     //                                            
            for(toRemove; toRemove<hostend; toRemove++)        //                                         
                *toRemove = 0;                                 //                
            length = strlen(inbuffer)+                         //                        
                strlen(toRemove)+1;                            //                     
            char * request = malloc(80000);                    //                              
            bzero(request, 80000);                             //                     
            strcpy(request, inbuffer);                         //                        
            strcat(request, toRemove);                         //                        
            //strcat(request, "\r\n");                         //                      
            char * protocol = strstr(request, "HTTP/1.1");     //                                            
            if (protocol != NULL)                              //                   
                *(protocol+7) = '0';                           // HTTP 1.0 request                      
                                                               //
////////////// End Parsing //////////////////////////////////////                                                
            
            printf("Host: %s\n", host);
            printf("Port: %s\n", portstr);
            printf("Request: \n[%s]",request); 

            websock = proxyclient(host, portstr);
            free(portstr);
            free(host);

            n = write(websock, request, strlen(request)+1);
            free(request); 
            if (n < 0)
                error("ERROR sending request"); 

            char outbuffer[80000];
            bzero(outbuffer, 80000);
            n = read(websock,outbuffer,80000);            
            if (n < 0)
                error("ERROR getting response");
            close(websock); 

            n = write(newsockfd, outbuffer, 80000); 
            bzero(outbuffer, 80000);                  
            close(newsockfd);                                                                        
            return EXIT_SUCCESS;                                                   
        } else if (pID < 0)
            error("Error on forking");
////////// Parent ////////////////////
        else close(newsockfd);      //
//////////////////////////////////////    
    }
}