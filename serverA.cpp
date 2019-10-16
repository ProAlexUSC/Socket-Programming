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
#define SERVERA_PORT "21472"
#define MAXBUFLEN 100

void loadData(std::string inFileName, std::map<char, std::vector<std::vector<std::string>>> &data);
void printData(std::map<char, std::vector<std::vector<std::string>>> data);
int initialUDPServer();

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

    printf("The Server A is up and running using UDP on port %s.\n", SERVERA_PORT);
    printf("The Server A has constructed a list of <number> maps:\n-------------------------------------------\n");
    printf("Map ID\tNum Vertices\tNum Edges\n");
    std::map<char, std::vector<std::vector<std::string>>> data;
    try
    {
        loadData("map.txt", data);
    }
    catch (...)
    {
        fprintf(stderr, "load error!\n");
    }
    printData(data);

    while (true)
    {
        addr_len = sizeof their_addr;
        if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN - 1, 0,
                                 (struct sockaddr *)&their_addr, &addr_len)) == -1)
        {
            perror("recvfrom");
            exit(1);
        }
        printf("listener: got packet from %s\n",
               inet_ntop(their_addr.ss_family,
                         get_in_addr((struct sockaddr *)&their_addr),
                         s, sizeof s));
        printf("listener: packet is %d bytes long\n", numbytes);
        buf[numbytes] = '\0';
        printf("listener: packet contains \"%s\"\n", buf);
        close(sockfd);
    }
}

void loadData(std::string inFileName, std::map<char, std::vector<std::vector<std::string>>> &data)
{
    std::ifstream infile;
    infile.open(inFileName);
    std::string line;
    if (!infile)
    {
        fprintf(stderr, "map.txt doesn't exist!\n");
        throw "map.txt doesn't exist!\n";
    }
    int linenumber = 0;
    char previousMapID;
    std::pair<int, int> startAndEng;
    std::set<int> nodeSet;
    int startNodeIndex;
    int endNodeIndex;
    while (getline(infile, line))
    {
        if (isupper(line.at(0))) // load ID
        {
            nodeSet.clear(); // new map IP
            previousMapID = line.at(0);
            std::vector<std::vector<std::string>> mapdata;
            std::vector<std::string> prop_trans_nodenum(3);
            mapdata.push_back(prop_trans_nodenum);
            data[previousMapID] = mapdata;
            continue;
        }
        if (line.find(" ") == std::string::npos)
        {
            data[previousMapID][0].push_back(line); // prop_speed
            getline(infile, line);
            data[previousMapID][0].push_back((line)); // trans_speed
            continue;
        }
        std::vector<std::string> start_end_long(3);
        startNodeIndex = line.find(" ");
        start_end_long[0] = line.substr(0, startNodeIndex);
        nodeSet.insert(std::stoi(line.substr(0, startNodeIndex))); // count node
        endNodeIndex = line.find(" ", startNodeIndex + 1);
        start_end_long[1] = line.substr(startNodeIndex + 1, endNodeIndex);
        nodeSet.insert(std::stoi(line.substr(startNodeIndex + 1, endNodeIndex))); // count node
        start_end_long[2] = std::stoi(line.substr(endNodeIndex + 1, endNodeIndex));
        data[previousMapID].push_back(start_end_long);
        data[previousMapID][0][2] = std::to_string(nodeSet.size()); // count node
    }
    infile.close();
}
int initialUDPServer()
{
    memset(&hints, 0, sizeof hints);

    hints.ai_family = AF_UNSPEC; // set to AF_INET to force IPv4
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo(NULL, SERVERA_PORT, &hints, &servinfo)) != 0)
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

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1)
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

void printData(std::map<char, std::vector<std::vector<std::string>>> data)
{
    for (auto it = data.begin(); it != data.end(); ++it)
    {
        printf("%c\t%d\t\t%d\n", it->first, std::stoi(it->second[0][2]), (int)it->second.size() - 1);
    }
}
