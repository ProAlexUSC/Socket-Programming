#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fstream>
#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <set>
using namespace std;
#define SERVERB_PORT 22472
#define AWS_UDP_PORT 23472
#define MAXBUFLEN 100

int initialUDPServer();
int sendToAws(int *dist, int src);

// cite from beej
int sockfd;
struct addrinfo hints, *servinfo, *p;
int rv;
int numbytes;
struct sockaddr_storage their_addr;
char buf[MAXBUFLEN];
socklen_t addr_len;
char s[INET6_ADDRSTRLEN];

void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET)
    {
        return &(((struct sockaddr_in *)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

int main(int argc, char const *argv[])
{
    int status;
    if ((status = initialUDPServer()) != 0)
    {
        return status;
    }
    inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof s);
    printf("The Server B is up and running using UDP on port %d.\n", SERVERB_PORT);
    while (true)
    {
        addr_len = sizeof their_addr;
        if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN - 1, 0,
                                 (struct sockaddr *)&their_addr, &addr_len)) == -1)
        {
            perror("recvfrom");
            exit(1);
        }
        printf("The Server B has received data for calculation:\n");
        buf[numbytes] = '\0';
        printf("%s", buf);
        // sendToAws();// TODO
        printf("The Server B has sent shortest paths to AWS.");
    }
    // close(sockfd);
}

// cite from beej
int initialUDPServer()
{
    memset(&hints, 0, sizeof hints);

    hints.ai_family = AF_UNSPEC; // set to AF_INET to force IPv4
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo("127.0.0.1", to_string(SERVERB_PORT).c_str(), &hints, &servinfo)) != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and bind to the first we can
    for (p = servinfo; p != NULL; p = p->ai_next)
    {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                             p->ai_protocol)) == -1)
        {
            perror("listener: socket");
            continue;
        }

        if (::bind(sockfd, p->ai_addr, p->ai_addrlen) == -1)
        {
            close(sockfd);
            perror("listener: bind");
            continue;
        }

        break;
    }

    if (p == NULL)
    {
        fprintf(stderr, "listener: failed to bind socket\n");
        return 2;
    }

    freeaddrinfo(servinfo);
    return 0;
}

int sendToAws(int *dist, int src)
{
    string output;
    for (int i = 0; i < 10; i++)
    {
        if (dist[i] == INF || i == src)
        {
            continue;
        }
        output += to_string(i);
        output += "\t\t";
        output += to_string(dist[i]);
        output += "\n";
    }
    if ((numbytes = sendto(sockfd, output.c_str(), MAXBUFLEN, 0,
                           (struct sockaddr *)&their_addr, addr_len)) == -1)
    {
        perror("sendto");
        exit(1);
    }
    printf("The Server A has sent shortest paths to AWS.\n");
    return 0;
}