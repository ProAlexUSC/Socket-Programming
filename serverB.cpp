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
#define MAXBUFLEN 10000

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

int main(int argc, char const *argv[])
{
    int status;
    if ((status = initialUDPServer()) != 0)
    {
        return status;
    }
    printf("The Server B is up and running using UDP on port %d.\n", SERVERB_PORT);
    while (true)
    {
        addr_len = sizeof their_addr;
        // receive data from aws
        if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN - 1, 0,
                                 (struct sockaddr *)&their_addr, &addr_len)) == -1)
        {
            perror("recvfrom");
            exit(1);
        }
        printf("The Server B has received data for calculation:\n");
        buf[numbytes] = '\0';
        // send data back
        if ((status = sendToAws(buf)) != 0)
        {
            perror("Can not send To Aws");
        }
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
            perror("serverB listener: socket");
            continue;
        }

        if (::bind(sockfd, p->ai_addr, p->ai_addrlen) == -1)
        {
            close(sockfd);
            perror("serverB listener: bind");
            continue;
        }

        break;
    }

    if (p == NULL)
    {
        fprintf(stderr, "serverB listener: failed to bind socket\n");
        return 2;
    }
    freeaddrinfo(servinfo);
    return 0;
}

int sendToAws(char *buf)
{
    // calculate the time
    // format: <size>P<prop>T<trans><D>path
    string input(buf);
    long long size = stoll(input.substr(0, input.find("P")));
    double prop = stod(input.substr(input.find("P") + 1, input.find("T") - input.find("P") - 1));
    printf("* Propagation speed: %s km/s;\n", input.substr(input.find("P") + 1, input.find("T") - input.find("P") - 1).c_str());
    double trans = stod(input.substr(input.find("T") + 1, input.find("D") - input.find("T") - 1));
    printf("* Transmission speed %s Bytes/s;\n", input.substr(input.find("T") + 1, input.find("D") - input.find("T") - 1).c_str());
    double Tt = size / (8 * trans); //Byte to bit

    //calculate line by line
    int indexLineStart = input.find("D");
    int delimiter;
    stringstream aftercalculate;
    stringstream result;
    int vertex;
    int distance;
    while (indexLineStart != (input.length() - 1))
    {
        indexLineStart++;
        delimiter = input.find("\t\t", indexLineStart);
        vertex = stoi(input.substr(indexLineStart, delimiter - indexLineStart));
        distance = stoi(input.substr(delimiter + 2, input.find('\n', indexLineStart) - delimiter - 2));
        printf("* Path length for destination %d:%d;\n", vertex, distance);
        aftercalculate << to_string(vertex);
        aftercalculate << "\t\t";
        aftercalculate << fixed << setprecision(2) << (Tt + distance / prop); // 2 precision
        aftercalculate << "\n";
        result << to_string(vertex);
        result << "\t\t";
        result << fixed << setprecision(2) << Tt; // 2 precision
        result << "\t";
        result << fixed << setprecision(2) << (distance  / prop); // 2 precision
        result << "\t";
        result << fixed << setprecision(2) << (Tt + distance  / prop); // 2 precision
        result << "\n";
        indexLineStart = input.find('\n', indexLineStart);
    }
    printf("The Server B has finished the calculation of the delays:\n");
    printf("------------------------------------------\n");
    printf("Destination\tDelay\n");
    printf("------------------------------------------\n");
    cout << aftercalculate.str();
    printf("------------------------------------------\n");
    // send back to aws
    if ((numbytes = sendto(sockfd, result.str().c_str(), MAXBUFLEN, 0,
                           (struct sockaddr *)&their_addr, addr_len)) == -1)
    {
        perror("sendto");
        exit(1);
    }
    printf("The Server B has finished sending the output to AWS\n");
    return 0;
}