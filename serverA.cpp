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
#define MAXBUFLEN 10000
#define INF 1000000000

void loadData(string inFileName, map<char, vector<vector<int>>> &data);
void printData(map<char, vector<vector<int>>> &data);
int initialUDPServer();
void dijkstra(vector<vector<int>> input, int src, int *dist);
void printMinDist(int *dist, int src);
int sendToAws(int *dist, int src, int prop, int trans);
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
    printf("The Server A is up and running using UDP on port %d.\n", SERVERA_PORT);

    // use a map to store the map
    // MapId as the key
    // the value is a 2 dimensional vector
    // the first sub vector stores the prop trans and the number of node in order
    // the rest are the edges. start end and length
    map<char, vector<vector<int>>> data;
    try
    {
        loadData("map.txt", data);
    }
    catch (...)
    {
        fprintf(stderr, "load error!\n");
    }
    printf("The Server A has constructed a list of %d maps:\n-------------------------------------------\n", data.size());
    printf("Map ID\tNum Vertices\tNum Edges\n");
    printf("------------------------------------------\n");
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
        if ((status = sendToAws(dist, vertex, data[mapID][0][0], data[mapID][0][1])) != 0)
        {
            perror("Can not send To Aws");
        }
        printf("The Server A has sent shortest paths to AWS.");
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

    if ((rv = getaddrinfo("127.0.0.1", to_string(SERVERA_PORT).c_str(), &hints, &servinfo)) != 0)
    {
        fprintf(stderr, "serverA getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and bind to the first we can
    for (p = servinfo; p != NULL; p = p->ai_next)
    {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                             p->ai_protocol)) == -1)
        {
            perror("serverA listener: socket");
            continue;
        }

        if (::bind(sockfd, p->ai_addr, p->ai_addrlen) == -1)
        {
            close(sockfd);
            perror("serverA listener: bind");
            continue;
        }

        break;
    }

    if (p == NULL)
    {
        fprintf(stderr, "serverA listener: failed to bind socket\n");
        return 2;
    }

    freeaddrinfo(servinfo);
    return 0;
}

// load map Data
void loadData(string inFileName, map<char, vector<vector<int>>> &data)
{
    // open file
    ifstream infile;
    infile.open(inFileName);
    string line;
    if (!infile)
    {
        fprintf(stderr, "map.txt doesn't exist!\n");
        throw "map.txt doesn't exist!\n";
    }

    // parse the Data
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
        data[previousMapID][data[previousMapID].size() - 1][0] = stoi(line.substr(0, startNodeIndex)); // start
        nodeSet.insert(stoi(line.substr(0, startNodeIndex)));                                          // count node
        endNodeIndex = line.find(" ", startNodeIndex + 1);
        data[previousMapID][data[previousMapID].size() - 1][1] = stoi(line.substr(startNodeIndex + 1, endNodeIndex - startNodeIndex - 1)); // end
        nodeSet.insert(stoi(line.substr(startNodeIndex + 1, endNodeIndex - startNodeIndex - 1)));                                          // count node
        data[previousMapID][data[previousMapID].size() - 1][2] = stoi(line.substr(endNodeIndex + 1));                                      // length
        data[previousMapID][0][2] = nodeSet.size();                                                                                        // count node
    }
    infile.close(); // close the file
}

void printData(map<char, vector<vector<int>>> &data)
{
    // traversal the data and print it!
    for (auto it = data.begin(); it != data.end(); ++it)
    {
        printf("%c\t%d\t\t%d\n", it->first, it->second[0][2], (int)it->second.size() - 1);
    }
}

// dijkstra!
// set the output result
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
    // min distance from the src
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

        // find the nearest node now
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
            if (j == src) // ignore src
            {
                continue;
            }
            dist[j] = min(dist[j], dist[minNum] + edges[minNum][j]);
        }
    }
    // set the result
    for (int i = 0; i < 10; i++)
    {
        result[i] = dist[i];
    }
}

// print the mindist
void printMinDist(int *dist, int src)
{
    printf("The Server A has identified the following shortest paths:\n------------------------------------------\n");
    printf("Destination\tMin Length\n");
    printf("------------------------------------------\n");
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

int sendToAws(int *dist, int src, int prop, int trans)
{
    // format: <prop>T<trans><D>path
    string output;
    output += to_string(prop);
    output += "T"; // delimiter
    output += to_string(trans);
    output += "D"; // delimiter

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
    // send the data back
    if ((numbytes = sendto(sockfd, output.c_str(), MAXBUFLEN, 0,
                           (struct sockaddr *)&their_addr, addr_len)) == -1)
    {
        perror("serverA:sendto");
        exit(1);
    }
    printf("The Server A has sent shortest paths to AWS.\n");
    return 0;
}