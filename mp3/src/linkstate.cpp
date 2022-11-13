#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <queue>
#include <vector>
#include <string>
#include <algorithm>
using namespace std;
const int Ni = 10000;
const int MAX_INTEGER = 1 << 30;

struct node
{
    int vertice, cost;

    node(int a, int b)
    {
        vertice = a;
        cost = b;
    }

    bool operator<(const node &a) const
    {
        if (cost == a.cost)
        {
            return vertice < a.vertice;
        }
        else
        {
            return cost > a.cost;
        }
    }
};

vector<node> e[Ni];
int dis[Ni], n;
priority_queue<node> q;
vector<int> linkstate(int start, int end, int prev[]);
void linkstate(int start, int prev[]);
void dijkstra(int start, int end, int prev[]);
void output(int src, int dst, char* message);
void readMessage(char* messagefile);
FILE * fpout;

int main(int argc, char** argv) {
    //printf("Number of arguments: %d", argc);
    if (argc != 4) {
        printf("Usage: ./linkstate topofile messagefile changesfile\n");
        return -1;
    }

    char buffer[256];
    ifstream topofile (argv[1]);
    ifstream changesfile(argv[3]);

    if(!topofile){
        printf("Unable to open topofile");
        exit(1); // terminate with error
    }
    if(!changesfile){
        printf("Unable to open changesfile");
        exit(1); // terminate with error
    }
    fpout = fopen("output.txt", "a");
    int a,b,c,d,f;
    int i=0,j=0;
    string temp;
    while (getline(topofile, temp)) {
			int message_len = temp.length();
			char message[message_len];
			int start = -1, end = -1;
			sscanf(temp.c_str(), "%d %d %d", &a, &b, &c);
            n = max(n, max(a, b));
            if(c <= 0) continue;
            e[a].push_back(node(b, c));
            e[b].push_back(node(a, c));
            // printf("%d%d%d", a, b, c);
            // printf("\n");
			if (start == -1 || end == -1) {
				continue;
			}
	}
    // while (! topofile.eof() )
    // {   
    //     topofile >> a;
    //     topofile >> b;
    //     topofile >> c;
    //     n = max(n, max(a, b));

    //     if(c <= 0) continue;
    //     e[a].push_back(node(b, c));
    //     e[b].push_back(node(a, c));
    // }
    int index = 1;
    int prev[n + 1];
    memset(prev, 0, sizeof(prev));
    // while(! messagefile.eof())
    // {
    //    messagefile >> d;
    //    messagefile >> f;
    //    message[index][0] = d;
    //    message[index][1] = f;
    //    messagefile >> d;
    //    messagefile >> f;
    //    printf("%d", d);
    //    index ++;
    // }
    // printf("%d", index);
    for(int i = 1; i <= n; i++){
        linkstate(i, prev);
        memset(prev, 0, sizeof(prev));
    }
    topofile.close();
    readMessage(argv[2]);
    temp.clear();
    while (getline(changesfile, temp))
    {   
        int message_len = temp.length();
		char message[message_len];
		int start = -1, end = -1;
		sscanf(temp.c_str(), "%d %d %d", &a, &b, &c);
        n = max(n, max(a, b));
        // printf("%d%d%d", a, b, c);
        // printf("\n");
        for(int i = 0; i < e[a].size(); i++){
            if(e[a][i].vertice == b){
                e[a].erase(e[a].begin() + i);
            }
        }
        for(int j = 0; j < e[b].size(); j++){
            // printf("%s%d%d\n","e[b]:", e[b][j].vertice, a);
            if(e[b][j].vertice == a){
                // printf("%s%d\n","index:", j);
                e[b].erase(e[b].begin() + j);
            }
        }
        if(c > 0){
            e[a].push_back(node(b, c));
            e[b].push_back(node(a, c));
        }
        for(int i = 1; i <= n; i++){
            linkstate(i, prev);
            memset(prev, 0, sizeof(prev));
        }
        // for(node nn : e[a]){
        //     printf("%s%d\n","e[a]:", nn.vertice);
        // }
        // for(node nn : e[b]){
        //     printf("%s%d\n","e[b]:", nn.vertice);
        // }
        readMessage(argv[2]);
    }
    fclose(fpout);
    return 0;
}
    vector<int> linkstate(int start, int end, int prev[])
{
        memset(dis, 0x3f, sizeof(dis));
        dis[start] = 0;
        vector<int> hops;
    

        dijkstra(start, end, prev);
        if(prev[end == 0])
        {
            return hops;
        }
        int temp = end;
        while(prev[temp] != start && prev[temp] != 0)
        {
            hops.push_back(prev[temp]);
            temp = prev[temp];
        }
        if(prev[temp] == 0){
            return hops;
        }
        hops.push_back(start);
        reverse(hops.begin(), hops.end());
        return hops;
        // for (int i = 1; i <= n; i++)
        // {
        //     printf("%d", dis[i]);
        // }
}

void linkstate(int start, int prev[])
{
        memset(dis, 0x3f, sizeof(dis));
        dis[start] = 0;
        vector<int> hops;
        int ttemp;
        dijkstra(start, 1, prev);
        for(int j = 1; j <= n; j++){
            if(dis[j] == 1061109567){
                ttemp = -999;
            }else{
                ttemp = dis[j];
            }
            if(j == start){
                fprintf(fpout, "%d %d %d\n", j, j, ttemp);
                continue;
            }
            int temp = j;
            while(prev[temp] != start && prev[temp] != 0)
            {
                temp = prev[temp];
            }
            fprintf(fpout, "%d %d %d\n", j, temp, ttemp);
        }
        fprintf(fpout, "\n");
}

void dijkstra(int start, int end, int prev[])
{   
    q.push(node(start, dis[start]));
    while (!q.empty())
    {
        node x = q.top();
        q.pop();
        if(x.cost > dis[x.vertice])
        {
            // printf("%s%d", "continue:", x.vertice);
            continue;
        }
        // printf("%d", x.vertice);
        for (int i = 0; i < e[x.vertice].size(); i++)
        {
            node y = e[x.vertice][i];
            if (dis[y.vertice] > dis[x.vertice] + y.cost)
            {   
                dis[y.vertice] = dis[x.vertice] + y.cost;
                q.push(node(y.vertice, dis[y.vertice]));
                // printf("%s%d", "y:", y.vertice);
                prev[y.vertice] = x.vertice;
            }else if(dis[y.vertice] == dis[x.vertice] + y.cost && prev[y.vertice] > x.vertice)
            {
                prev[y.vertice] = x.vertice;
            }
        }
        // printf("\n");
    }
    priority_queue <node> empty;
    swap(empty,q);
}

void output(int start, int end, char* message) {
	// FILE * fpout;

	// fpout = fopen("output.txt", "a");

	// if (f_tables[src][dst].second == INT_MAX || 
	// 	all_nodes.count(src) <= 0 ||
	// 	all_nodes.count(dst) <=0) {
	// 	fprintf(fpout, "from %d to %d cost infinite hops unreachable message %s\n", src, dst, message);
	// 	fclose(fpout);
	// 	return;//or continue
	// }
    vector<int> hops;
    int prev[n+1];
    memset(prev, 0, sizeof(prev));
    hops = linkstate(start, end, prev);
    if (hops.empty()) {
		fprintf(fpout, "from %d to %d cost infinite hops unreachable message %s\n", start, end, message);
		fclose(fpout);
		return;//or continue
	}
	fprintf(fpout, "from %d to %d cost %d hops ", start, end, dis[end]);
	for (int hop : hops) {
		fprintf(fpout, "%d ", hop);
	}
	if (hops.empty()) {
		fprintf(fpout, " ");
	}
	fprintf(fpout, "message %s\n", message);
	// fprintf(fpout, "\n");
	// fclose(fpout);
}

void readMessage(char* messagefile) {
	//messagefile: 2 1 send this message from 2 to 1
	ifstream in;
	string line;
	in.open(messagefile);

	if (in.is_open()) {
		while (getline(in, line)) {
			int message_len = line.length();
			char message[message_len];
			int start = -1, end = -1;
			sscanf(line.c_str(), "%d %d %[^\n]", &start, &end, message);
			if (start == -1 || end == -1) {
				continue;
			}
			// printf("start: %d end: %d ", start, end);
			// printf("message: %s\n", message);
			output(start, end, message);
            fprintf(fpout, "\n");
		}
		in.close();
	} else {
		exit(2);
	}
}
