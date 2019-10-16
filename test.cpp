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
#define MAXBUFLEN 100

void loadData(string inFileName, map<char, vector<vector<int>>> &data);
void dijkstra(vector<vector<int>> input, int src);

// cite from beej
int sockfd;
struct addrinfo hints, *servinfo, *p;
int rv;
int numbytes;
struct sockaddr_storage their_addr;
char buf[MAXBUFLEN];
socklen_t addr_len;
char s[INET6_ADDRSTRLEN];

int main(int argc, char const *argv[])
{
    map<char, vector<vector<int>>> data;
    loadData("map.txt", data);
    for (auto it = data.begin(); it != data.end(); ++it)
    {
        printf("%c\t%d\t\t%d\n", it->first, it->second[0][2], (int)it->second.size() - 1);
    }
    dijkstra(data['A'], 0);
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
        // cout << (line.find(" ") == string::npos) << endl;
        if (isupper(line.at(0))) // load ID
        {
            nodeSet.clear(); // new map IP
            previousMapID = line.at(0);
            vector<vector<int>> init(1, vector<int>(0));
            data[previousMapID] = init;
            continue;
        }
        if (line.find(" ") == string::npos)
        {
            data[previousMapID][0].push_back(stoi(line)); // prop_speed
            getline(infile, line);
            data[previousMapID][0].push_back(stoi(line)); // trans_speed
            cout << data[previousMapID][0][0] << endl;
            cout << data[previousMapID][0][1] << endl;
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

void dijkstra(vector<vector<int>> input, int src)
{
    int edges[10][10]; // at most 10 edges
    for (int i = 0; i < 10; i++)
    {
        for (int j = 0; j < 10; j++)
        {
            edges[i][j] = 1000000000;
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
        dist[i] = 1000000000;
    }
    dist[src] = 0;
    bool st[10];
    for (int i = 0; i < 10; i++)
    {
        int minNum, value = 1000000000;
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
        printf("%d\n", dist[i]);
    }
}