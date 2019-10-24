#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fstream>
#include <iostream>
#include <string>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <sstream>
#include <vector>
using namespace std;

#define AWS_UDP_PORT 23472
#define AWS_TCP_PORT 24472
#define SERVERA_PORT 21472
#define SERVERB_PORT 22472
#define MAXBUFLEN 10000
#define BACKLOG 10

//cite from beej
int sock_tcp_fd, new_tcp_fd; // listen on sock_tcp_fd, new connection on new_tcp_fd
int sock_udp_fd;             // listen on sock_udp_fd
// int sock_udp_toA_fd;
struct addrinfo hints_TCP, hints_UDP, *servinfo_TCP, *servinfo_UDP, *p;
struct sockaddr_storage their_addr; // connector's address information
struct addrinfo *UDP_TO_INFO;       // UDP info for serverA and serverB
// socket need below parameters
socklen_t addr_len;
struct sigaction sa;
int yes = 1;
char s[INET6_ADDRSTRLEN];
int rv;
int numbytes;
char buf[MAXBUFLEN];

// format the message from serverA and serverB
vector<vector<string>> sendToABuf;
stringstream sendToA;

int initialTCPServer();
int initialUDPServer();
int connectWithServerA(string parameters);
int connectWithServerB(string size);

// For socket
void sigchld_handler(int s)
{

    // waitpid() might overwrite errno, so we save and restore it: int saved_errno = errno;
    int saved_errno = errno;
    while (waitpid(-1, NULL, WNOHANG) > 0)
        ;

    errno = saved_errno;
}

int main(void)
{
    int status;
    if ((status = initialTCPServer()) != 0)
    {
        return status;
    }
    printf("The AWS is up and running.\n");
    // if ((status = initialUDPServer()) != 0)
    // {
    //     printf("server: UDP start failed!\n");
    //     return status;
    // }

    while (true)
    {
        addr_len = sizeof their_addr;
        new_tcp_fd = accept(sock_tcp_fd, (struct sockaddr *)&their_addr, &addr_len);

        if (new_tcp_fd == -1)
        {
            perror("accept");
            continue;
        }

        // printf("server: got connection from %s\n", s);

        if (!fork())
        {
            close(sock_tcp_fd); // child doesn't need this
            if ((status = initialUDPServer()) != 0)
            {
                printf("server: UDP start failed!\n");
                return status;
            }
            // receive data from client
            if ((numbytes = recv(new_tcp_fd, buf, MAXBUFLEN - 1, 0)) == -1)
            {
                perror("recv");
                exit(1);
            }
            buf[numbytes] = '\0';
            // parse the data
            char mapID = 0;
            int vertex = 0;
            string size;
            int index = 0;
            string input(buf);
            index = input.find(" ", 2);
            mapID = input.at(0);               // One char mapID
            vertex = stoi(input.substr(2, index-2)); // Only one digit because vertex number is no more than 10
            size = input.substr(index + 1);
            printf("The AWS has received map ID %c, start vertex %d and file size %s from the client using TCP over port %d\n", mapID, vertex, size.c_str(), AWS_TCP_PORT);
            connectWithServerA(input);
            char fromServerB[MAXBUFLEN];
            connectWithServerB(size);
            sendToA.str(std::string()); // newTCP, newOutput!
            // parse the buffer
            for (auto it = sendToABuf.begin(); it != sendToABuf.end(); ++it)
            {
                for (auto it2 = it->begin(); it2 != it->end(); ++it2)
                {
                    sendToA << *it2;
                }
                sendToA << "\n";
            }
            // send data to client
            if ((send(new_tcp_fd, sendToA.str().c_str(), MAXBUFLEN, 0) == -1))
            {
                perror("recv");
                exit(1);
            }
            printf("The AWS has sent calculated delay to client using TCP over port %d.\n", AWS_TCP_PORT);
            // close socket
            close(new_tcp_fd);
            close(sock_udp_fd);
            exit(0);
        }
        close(new_tcp_fd); // parent doesn't need this
    }
}
// cite from beej
int initialTCPServer()
{
    memset(&hints_TCP, 0, sizeof hints_TCP);
    hints_TCP.ai_family = AF_UNSPEC;
    hints_TCP.ai_socktype = SOCK_STREAM; // TCP
    hints_TCP.ai_flags = AI_PASSIVE;     // my ip

    if ((rv = getaddrinfo("127.0.0.1", to_string(AWS_TCP_PORT).c_str(), &hints_TCP, &servinfo_TCP)) != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    for (p = servinfo_TCP; p != NULL; p = p->ai_next)
    {
        if ((sock_tcp_fd = socket(p->ai_family, p->ai_socktype,
                                  p->ai_protocol)) == -1)
        {
            perror("server: socket");
            continue;
        }

        if (setsockopt(sock_tcp_fd, SOL_SOCKET, SO_REUSEADDR, &yes,
                       sizeof(int)) == -1)
        {
            perror("setsockopt");
            exit(1);
        }

        if (::bind(sock_tcp_fd, p->ai_addr, p->ai_addrlen) == -1)
        {
            close(sock_tcp_fd);
            perror("server: bind");
            continue;
        }

        break;
    }

    if (p == NULL)
    {
        fprintf(stderr, "server: failed to bind\n");
        return 2;
    }

    freeaddrinfo(servinfo_TCP);

    if (listen(sock_tcp_fd, BACKLOG) == -1)
    {
        perror("listen");
        exit(1);
    }

    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    if (sigaction(SIGCHLD, &sa, NULL) == -1)
    {
        perror("sigaction");
        exit(1);
    }
    return 0;
}

int initialUDPServer()
{
    memset(&hints_UDP, 0, sizeof hints_UDP);

    hints_UDP.ai_family = AF_UNSPEC; // set to AF_INET to force IPv4
    hints_UDP.ai_socktype = SOCK_DGRAM;
    hints_UDP.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo("127.0.0.1", to_string(AWS_UDP_PORT).c_str(), &hints_UDP, &servinfo_UDP)) != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and bind to the first we can
    for (p = servinfo_UDP; p != NULL; p = p->ai_next)
    {
        if ((sock_udp_fd = socket(p->ai_family, p->ai_socktype,
                                  p->ai_protocol)) == -1)
        {
            perror("aws listener: socket");
            continue;
        }
        // int enable = 1;
        // if (setsockopt(sock_udp_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
        //     perror("setsockopt(SO_REUSEADDR) failed");

        if (::bind(sock_udp_fd, p->ai_addr, p->ai_addrlen) == -1)
        {
            close(sock_udp_fd);
            perror("aws listener: bind");
            continue;
        }

        break;
    }

    if (p == NULL)
    {
        fprintf(stderr, "aws listener: failed to bind socket\n");
        return 2;
    }

    freeaddrinfo(servinfo_UDP);
    return 0;
}
int connectWithServerA(string parameters)
{
    sendToABuf.clear(); // new buf for sendToA
    addr_len = sizeof their_addr;
    // get serverA address
    int status;
    if ((status = getaddrinfo("127.0.0.1", to_string(SERVERA_PORT).c_str(), &hints_UDP, &UDP_TO_INFO)) != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        return 2;
    }
    // send to serverA
    if ((numbytes = sendto(sock_udp_fd, parameters.c_str(), MAXBUFLEN, 0,
                           UDP_TO_INFO->ai_addr, UDP_TO_INFO->ai_addrlen)) == -1)
    {
        perror("talker aws: sendto");
        exit(1);
    }
    printf("The AWS has sent map ID and starting vertex to server A using UDP over port %d\n", AWS_UDP_PORT);

    // get result from serverA
    if ((numbytes = recvfrom(sock_udp_fd, buf, MAXBUFLEN - 1, 0,
                             (struct sockaddr *)&their_addr, &addr_len)) == -1)
    {
        perror("recvfrom");
        exit(1);
    }
    // output the result
    // I use D as a delimiter
    buf[numbytes] = '\n';
    printf("The AWS has received shortest path from server A:\n------------------------------------------\n");
    printf("Destination\tMin Length\n");
    printf("------------------------------------------\n");
    string dist(buf);
    dist = dist.substr(dist.find("D") + 1); // D is the delimiter
    printf("%s", dist.c_str());
    printf("------------------------------------------\n");

    // For output
    stringstream stream(dist);
    string line;
    while (getline(stream, line))
    {
        vector<string> temp;
        temp.push_back(line + "\t");
        sendToABuf.push_back(temp);
    }
    return 0;
}
int connectWithServerB(string size)
{
    // send the distance from A and the file size to B
    addr_len = sizeof their_addr;
    string output;
    output = string(size);
    output += "P"; // P is the delimiter for the prop
    output += string(buf);
    // get serverB address
    int status;
    if ((status = getaddrinfo("127.0.0.1", to_string(SERVERB_PORT).c_str(), &hints_UDP, &UDP_TO_INFO)) != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        return 2;
    }
    // send to serverB
    if ((numbytes = sendto(sock_udp_fd, output.c_str(), MAXBUFLEN, 0,
                           UDP_TO_INFO->ai_addr, UDP_TO_INFO->ai_addrlen)) == -1)
    {
        perror("talker aws: sendto");
        exit(1);
    }
    printf("The AWS has sent path length, propagation speed and transmission speed to server B using UDP over port %d.\n", AWS_UDP_PORT);
    // receive data from serverB
    if ((numbytes = recvfrom(sock_udp_fd, buf, MAXBUFLEN - 1, 0,
                             (struct sockaddr *)&their_addr, &addr_len)) == -1)
    {
        perror("recvfrom");
        exit(1);
    }
    buf[numbytes] = '\0';
    printf("The AWS has received delays from server B:\n------------------------------------------\n");
    printf("Destination\tTt\tTp\tDelay\n");
    printf("------------------------------------------\n");
    printf("%s", buf);
    printf("------------------------------------------\n");

    // For output
    // we only need the Tt Tp and Delay!
    stringstream stream(buf);
    string line;
    int vertexIndex = 0;
    while (getline(stream, line))
    {
        sendToABuf[vertexIndex].push_back(line.substr(line.find("\t\t") + 1));
        vertexIndex++;
    }
    return 0;
}