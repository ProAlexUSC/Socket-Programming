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
#include <sstream>
#include <iomanip>
using namespace std;
#define SERVERB_PORT 22472
#define AWS_UDP_PORT 23472
#define MAXBUFLEN 100

int initialUDPServer();
int sendToAws(char *);

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
        if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN, 0,
                                 (struct sockaddr *)&their_addr, &addr_len)) == -1)
        {
            perror("recvfrom");
            exit(1);
        }
        printf("The Server B has received data for calculation:\n");
        buf[numbytes] = '\0';
        sendToAws(buf);
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

int sendToAws(char *buf)
{
    // calculate
    string input(buf);
    // cout<<input<<endl;
    int size = stoi(input.substr(0, input.find("P")));
    int prop = stoi(input.substr(input.find("P") + 1, input.find("T")));
    printf("* Propagation speed: %d km/s;\n", prop);
    int trans = stoi(input.substr(input.find("T") + 1, input.find("D")));
    printf("* Transmission speed %d Bytes/s;\n", trans);
    double Tt = 1000 * size / (8.0 * trans);

    //calculate line by line
    int indexLineStart = input.find("D");
    int delimiter;
    stringstream aftercalculate;
    stringstream result;
    int vertex;
    int distance;
    while (indexLineStart != (input.length()-1))
    {
        indexLineStart++;
        delimiter = input.find("\t\t",indexLineStart);
        vertex = stoi(input.substr(indexLineStart, delimiter-indexLineStart));
        distance = stoi(input.substr(delimiter + 2, input.find('\n',indexLineStart)-delimiter-2));
        printf("* Path length for destination %d:%d\n", vertex, distance);
        aftercalculate << to_string(vertex);
        aftercalculate << "\t\t";
        aftercalculate << fixed << setprecision(2) << (Tt + distance * 1000.0 / prop);
        aftercalculate << "\n";
        result<<to_string(vertex);
        result<<"\t\t";
        result<<fixed << setprecision(2) << Tt;
        result<<"\t\t";
        result << fixed << setprecision(2) << (distance * 1000.0 / prop);
        result<<"\t\t";
        result << fixed << setprecision(2) << (Tt + distance * 1000.0 / prop);
        result << "\n";
        indexLineStart = input.find('\n',indexLineStart);
    }
    printf("The Server B has finished the calculation of the delays:\n");
    printf("------------------------------------------\n");
    printf("Destination\tDelay\n");
    cout << aftercalculate.str() << endl;
    printf("------------------------------------------\n");
    if ((numbytes = sendto(sockfd, result.str().c_str(), MAXBUFLEN, 0,
                           (struct sockaddr *)&their_addr, addr_len)) == -1)
    {
        perror("sendto");
        exit(1);
    }
    printf("The Server B has finished sending the output to AWS\n");
    return 0;
}