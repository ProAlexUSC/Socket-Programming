#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string>
#include <iostream>
using namespace std;

#define MAXBUFLEN 10000
#define AWS_TCP_PORT 24472 // the port client will be connecting to

int main(int argc, char *argv[])
{
    printf("The client is up and running.\n");

    // need below for socket
    int sockfd, numbytes;
    char buf[MAXBUFLEN];
    struct addrinfo hints, *servinfo, *p;
    int rv;

    if (argc != 4)
    {
        fprintf(stderr, "Input error!\n");
        exit(1);
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    // connect to aws
    if ((rv = getaddrinfo("127.0.0.1", to_string(AWS_TCP_PORT).c_str(), &hints, &servinfo)) != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and connect to the first we can
    for (p = servinfo; p != NULL; p = p->ai_next)
    {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                             p->ai_protocol)) == -1)
        {
            perror("client: socket");
            continue;
        }
        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1)
        {
            perror("client: connect");
            close(sockfd);
            continue;
        }

        break;
    }

    if (p == NULL)
    {
        fprintf(stderr, "client: failed to connect\n");
        return 2;
    }
    freeaddrinfo(servinfo); // all done with this structure
    string map(argv[1]);
    string src(argv[2]);
    string fsize(argv[3]);
    // send to aws
    if (send(sockfd, (map + " " + src + " " + fsize).c_str(), MAXBUFLEN, 0) == -1)
        perror("send");
    struct sockaddr getPort;
    socklen_t len = sizeof(getPort);
    getsockname(sockfd, &getPort, &len);
    printf("The client has sent query to AWS using TCP over port %d: start vertex %s; map %s; file size %s.\n", ((struct sockaddr_in *)&getPort)->sin_port, argv[2], argv[1], argv[3]);
    // receive data from aws
    if ((numbytes = recv(sockfd, buf, MAXBUFLEN - 1, 0)) == -1)
    {
        perror("recv");
        exit(1);
    }
    printf("The client has received results from AWS:\n");
    printf("-----------------------------------------------------\n");
    printf("Destination\tMin Length\tTt\tTp\tDelay\n");
    printf("-----------------------------------------------------\n");
    buf[numbytes] = '\0';
    cout << buf;
    printf("-----------------------------------------------------\n");
    close(sockfd); // close socket
    return 0;
}
