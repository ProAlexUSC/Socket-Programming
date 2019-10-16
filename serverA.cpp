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
#define SERVERA_PORT 21472
#define AWS_UDP_PORT 23472
#define MAXBUFLEN 100
#define INF 1000000000

void loadData(string inFileName, map<char, vector<vector<int>>> &data);
void printData(map<char, vector<vector<int>>> &data);
int initialUDPServer();
void dijkstra(vector<vector<int>> input, int src, int *dist);
void printMinDist(int *dist, int src);
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
    inet_ntop(their_addr.ss_family,
              get_in_addr((struct sockaddr *)&their_addr),
              s, sizeof s);

    printf("The Server A is up and running using UDP on port %d.\n", SERVERA_PORT);
    printf("Map ID\tNum Vertices\tNum Edges\n");
    map<char, vector<vector<int>>> data;
    try
    {
        loadData("map.txt", data);
    }
    catch (...)
    {
        fprintf(stderr, "load error!\n");
    }
    printf("The Server A has constructed a list of %lu maps:\n-------------------------------------------\n", data.size());
    printData(data);
    printf("------------------------------------------\n");

    while (true)
    {
        addr_len = sizeof their_addr;
        if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN - 1, 0,
                                 (struct sockaddr *)&their_addr, &addr_len)) == -1)
        {
            perror("recvfrom");
            exit(1);
        }
        buf[numbytes] = '\0';
        char mapID;
        int vertex = 0;
        string input(buf);
        mapID = input.at(0);
        vertex = stoi(input.substr(2, input.find(" ", 2) + 1));
        printf("The Server A has received input for finding shortest paths: starting vertex %d of map %c.\n", vertex, mapID);
        int dist[10];
        dijkstra(data[mapID], vertex, dist);
        printMinDist(dist, vertex);
        sendToAws(dist, vertex);
        printf("The Server A has sent shortest paths to AWS.");
    }
    close(sockfd);
}

// cite from beej
int initialUDPServer()
{
    memset(&hints, 0, sizeof hints);

    hints.ai_family = AF_UNSPEC; // set to AF_INET to force IPv4
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo("0.0.0.0", to_string(SERVERA_PORT).c_str(), &hints, &servinfo)) != 0)
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
            ;
        }

        if (::bind(sockfd, p->ai_addr, p->ai_addrlen) == -1)
        {
            close(sockfd);
            perror("listener: bind");
            continue;
            ;
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

void loadData(string inFileName, map<char, vector<vector<int>>> &data)
{
    ifstream infile;
    infile.open(inFileName);
    string line;
    if (!infile)
    {
        fprintf(stderr, "map.txt doesn't exist!\n");
        throw "map.txt doesn't exist!\n";
    }
    int linenumber = 0;
    char previousMapID;
    pair<int, int> startAndEng;
    set<int> nodeSet;
    int startNodeIndex;
    int endNodeIndex;
    while (getline(infile, line))
    {
        if (isupper(line.at(0))) // load ID
        {
            nodeSet.clear(); // new map IP
            previousMapID = line.at(0);
            vector<vector<int>> init(1, vector<int>(0));
            data[previousMapID] = init;
            continue;
            ;
        }
        if (line.find(" ") == string::npos)
        {
            data[previousMapID][0].push_back(stoi(line)); // prop_speed
            getline(infile, line);
            data[previousMapID][0].push_back(stoi(line)); // trans_speed
            continue;
        }
        vector<int> newvector(3);
        data[previousMapID].push_back(newvector);
        startNodeIndex = line.find(" ");
        data[previousMapID][data[previousMapID].size() - 1][0] = stoi(line.substr(0, startNodeIndex));
        nodeSet.insert(stoi(line.substr(0, startNodeIndex))); // count node
        endNodeIndex = line.find(" ", startNodeIndex + 1);
        data[previousMapID][data[previousMapID].size() - 1][1] = stoi(line.substr(startNodeIndex + 1, endNodeIndex));
        nodeSet.insert(stoi(line.substr(startNodeIndex + 1, endNodeIndex))); // count node
        data[previousMapID][data[previousMapID].size() - 1][2] = stoi(line.substr(endNodeIndex + 1));
        data[previousMapID][0][2] = nodeSet.size(); // count node
    }
    infile.close();
}

void printData(map<char, vector<vector<int>>> &data)
{
    for (auto it = data.begin(); it != data.end(); ++it)
    {
        printf("%c\t%d\t\t%d\n", it->first, it->second[0][2], (int)it->second.size() - 1);
    }
}

void dijkstra(vector<vector<int>> input, int src, int *result)
{
    int edges[10][10]; // at most 10 edges
    for (int i = 0; i < 10; i++)
    {
        for (int j = 0; j < 10; j++)
        {
            edges[i][j] = INF;
        }
    }
    for (int i = 1; i < input.size(); i++)
    {
        edges[input[i][0]][input[i][1]] = input[i][2];
        edges[input[i][1]][input[i][0]] = input[i][2];
    }
    int dist[10];
    for (int i = 0; i < 10; i++)
    {
        dist[i] = INF;
    }
    dist[src] = 0;
    bool st[10];
    for (int i = 0; i < 10; i++)
    {
        int minNum, value = INF;
        for (int j = 0; j < 10; j++)
        {
            if (!st[j] && dist[j] < value)
            {
                value = dist[j];
                minNum = j;
            }
        }
        st[minNum] = true;
        for (int j = 0; j < 10; j++)
        {
            if (j == src)
            {
                continue;
            }
            dist[j] = min(dist[j], dist[minNum] + edges[minNum][j]);
        }
    }
    for (int i = 0; i < 10; i++)
    {
        result[i] = dist[i];
    }
}
void printMinDist(int *dist, int src)
{
    printf("The Server A has identified the following shortest paths:\n------------------------------------------\n");
    printf("Destination\tMin Length\n");
    for (int i = 0; i < 10; i++)
    {
        if (dist[i] == INF || i == src)
        {
            continue;
        }
        printf("%d\t\t%d\n", i, dist[i]);
    }
    printf("------------------------------------------\n");
}

int sendToAws(int *dist, int src)
{
    int sock_udp_to_Aws;
    struct addrinfo *UDPTOAWS;
    int status;
    if ((status = getaddrinfo("0.0.0.0", to_string(AWS_UDP_PORT).c_str(), &hints, &UDPTOAWS)) != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        return 2;
    }
    for (p = UDPTOAWS; p != NULL; p = p->ai_next)
    {
        if ((sock_udp_to_Aws = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
        {
            perror("talker A: socket");
            continue;
        }
        int enable = 1;
        break;
    }
    int enable = 1;
    if (setsockopt(sock_udp_to_Aws, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
        perror("setsockopt(SO_REUSEADDR) failed");

    if (p == NULL)
    {
        fprintf(stderr, "talker A: failed to bind socket\n");
        return 2;
    }

    freeaddrinfo(UDPTOAWS);
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
    addr_len = sizeof their_addr;
    if ((numbytes = sendto(sock_udp_to_Aws, "hello", 5, 0,
                           (struct sockaddr *)&their_addr, addr_len)) == -1)
    {
        perror("sendto");
        exit(1);
    }

    return 0;
}