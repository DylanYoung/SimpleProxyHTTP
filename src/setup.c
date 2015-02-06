/*
 =============================================================
 Name        : setup.c
 Author      : Casey Meijer -- A00337010
 Version     : 1.0.0
 License     : MIT
 Description : two functions to setup proxy connections

 =============================================================
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

    // zero out server address
    bzero((char *) &serv_addr, sizeof(serv_addr));

    // create a generic socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");
    portno = atoi(portstr);
    // parameters: standard internet, any address
    //  port num in net byte order
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    // Allow immediate reuse of port on disconnect
    if (setsockopt(sockfd, SOL_SOCKET,
     SO_REUSEADDR, &reuse, sizeof(int)) == -1) {
        close(sockfd);
        error("REUSEADDR failed");
    }
    // bind to localhost:portno
    if (bind(sockfd, (struct sockaddr *) &serv_addr,
            sizeof(serv_addr)) < 0)
        return -1;

    return sockfd;
}

int proxyclient(char ** host, char * portstr)
{
    int websock=0, portno=0;
    struct sockaddr_in host_addr;
    struct hostent *server;
    bzero((char *) &host_addr, sizeof(host_addr));

    // translate the host name
    server = gethostbyname(*host);
    if (server == NULL)
        error("ERROR, no such host\n");
    portno = atoi(portstr);
    // create a generic socket
    websock = socket(AF_INET, SOCK_STREAM, 0);
    if (websock < 0)
        error("ERROR opening socket");
    // set internet type
    host_addr.sin_family = AF_INET;
    // make sure we connect with the IP not name
    bcopy((char *)server->h_addr,
        (char *)&host_addr.sin_addr.s_addr,
        server->h_length);
     // port number in net byte order
    host_addr.sin_port = htons(portno);
    // connect socket to server:portno
    if (connect(websock,
            (struct sockaddr *) &host_addr,
                sizeof(host_addr)) < 0)
        return -1;
    return websock;
}
