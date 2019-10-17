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
using namespace std;

#define MAXBUFLEN 100
#define AWS_TCP_PORT 24472 // the port client will be connecting to

// cite from beej
// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET)
    {
        return &(((struct sockaddr_in *)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

int main(int argc, char *argv[])
{
    printf("The client is up and running.\n");
    int sockfd, numbytes;
    char buf[MAXBUFLEN];
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char s[INET6_ADDRSTRLEN];

    if (argc != 4)
    {
        fprintf(stderr, "Input error!\n");
        exit(1);
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

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

    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
              s, sizeof s);
    printf("client: connecting to %s\n", s);

    freeaddrinfo(servinfo); // all done with this structure
    string map(argv[1]);
    string src(argv[2]);
    string fsize(argv[3]);
    if (send(sockfd, (map + " " + src + " " + fsize).c_str(), MAXBUFLEN, 0) == -1)
        perror("send");
    printf("The client has sent query to AWS using TCP over port %d: start vertex %s; map %s; file size %s.\n", AWS_TCP_PORT, argv[1], argv[0], argv[2]);
    printf("The client has received results from AWS:\n");
    printf("-----------------------------------------------------\n");
    printf("Destination\tMin Length\tTt\tTp\tDelay\n");
    close(sockfd);
    return 0;

    numbytes = recv(sockfd, buf, MAXBUFLEN - 1, 0);
    if (numbytes == -1)
    {
        perror("recv");
        exit(1);
    }
    buf[numbytes] = '\0';
    printf("%s", buf);
    printf("-----------------------------------------------------\n");
    close(sockfd);
    return 0;
}
