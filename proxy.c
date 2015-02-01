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
                sizeof(host_addr)) < 0)
        return -1;
    return websock;
}

int parse(char * request, 
            char ** host, char * portstr) {

    char * hostbegin = strstr(request, "Host:")+ 6;   //                                              
    char * hostend = strstr(hostbegin, "\r\n");        //                                                                                           
    if (strstr(request, "GET") == NULL                //                                  
            || hostbegin == NULL || hostend == NULL)       //
        return -1;      
    int length = (hostend - hostbegin) + 2;            //                                     
    *host = malloc(sizeof(char)*(length));       //                                          
    bzero(*host,length);                                //
                                                       //                                                       //                 
    strncpy(*host, hostbegin, length-1);                //                                 
    char * porttemp = strstr(*host, ":");         //                                 
    if (porttemp == NULL)                
        strcpy(portstr, "80\0");                       //                 
    else{                                             //   
        *porttemp = 0;                                 //                 
        porttemp += sizeof(char)*1; 
        strcpy(portstr,porttemp);                      //               
    }                                                  //
    char * toRemove = strstr(request, "http");        //                                         
    hostend = strstr(request, *host) + length - 2;     //                                            
    for(; toRemove<hostend; toRemove++)        //                                         
        *toRemove = '\0'; 
    char * temp = malloc(
            sizeof(char)*strlen(toRemove)+1);                              //                                          //                                                                               //                                             //                        
    strcpy(temp, toRemove);                            //                                          //                                                                               //                                             //                        
    strcat(request, temp);
    free(temp);                       //                                                //                      
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
    bzero(request,80000);
    bzero(response,80000);
    bzero(portstr, 6);

    // Verify args are present
    if (argc < 2)
        error("ERROR, no port provided");

    sockfd = proxyserver(argv[1]); 
    if (sockfd < 0){
        error("ERROR on binding");
    }

    listen(sockfd,5);
 
    while(newsockfd>=0)
    {           
        newsockfd = accept(sockfd,                     // translate the connection request into a new socket
                        (struct sockaddr *) &cli_addr, 
                        &clilen);

        pid_t pID = fork();
////////// Child (move to transmit function??) ////
        if (pID == 0)
        {
            close(sockfd);
            if (newsockfd < 0)
                 error("ERROR on accept");
            n = read(newsockfd,request,80000); 
            if (n < 0)
                error("ERROR reading from socket");
        
            if (parse(request, &host, portstr)<0){
                close(newsockfd);
                error("ERROR: malformed GET request");     
            }   
            
            //printf("Host: %s\n", host);
            //printf("Port: %s\n", portstr);
            //printf("Request: \n[%s]",request); 

            websock = proxyclient(host, portstr);
            free(host);
            if (websock < 0){
                error("ERROR connecting");
            }

            n = write(websock, request, strlen(request)+1);

            if (n < 0)
                error("ERROR sending request"); 

            n = read(websock,response,80000);           
            if (n < 0)
                error("ERROR getting response");
            close(websock); 
 
            n = write(newsockfd, response, 80000);                 
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