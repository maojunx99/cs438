#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include <limits.h>
#include <stdint.h>

#include <iostream>
#include <sstream>
#include <fstream>

#include <map>
#include <queue>
#include <vector>
#include <string>
#include <unordered_set>
#include <unordered_map>
using namespace std;
unordered_map<int, unordered_map<int, int>> costMap;//costMap[node1][node2] = cost of edge (node1<->node2)
unordered_set<int> nodeSet;//nodeId set of all nodes

typedef unordered_map<int, pair<int, int> > vec_type;//first: minimum distance; second: minimum node
typedef unordered_map<int, vec_type> vecs_type;
unordered_map<int, unordered_map<int, unordered_map<int, pair<int, int> > >> VecList;
queue<pair<int, vec_type> > messageQueue;//first: message source, second: content
unordered_map<int, unordered_map<int, pair<int, int> > > f_tables;

void printTable(int node) {
	printf("Table for node %d:\n", node);
	for (int i = 1; i <= nodeSet.size(); i++) {
		for (int j = 1; j <= nodeSet.size(); j++) {
			printf("%d ", VecList[node][i][j].first);
		}
		printf("\n");
	}
	printf("Finish printing table\n");
}


int readTopoFile(char* topoFileName){
    ifstream in_fstream;
	string line;
	in_fstream.open(topoFileName);
	if (in_fstream.is_open()) {
		while (getline(in_fstream, line)) {
			stringstream ss(line);
			string node1, node2, cost;
			ss >> node1>> node2>> cost;
			//ss >> node2;
			//ss >> cost;
			int node1Id = atoi(node1.c_str());
			int node2Id = atoi(node2.c_str());
			if (node1Id == 0 || node2Id == 0) {
				continue;
			}

			nodeSet.insert(node1Id);
			nodeSet.insert(node2Id);

			costMap[node1Id][node2Id] = atoi(cost.c_str());
			costMap[node2Id][node1Id] = atoi(cost.c_str());
		
		}
		in_fstream.close();
	} else {
		exit(1);
	}
    return 0;
}


void costMap2VecList() {
	//int numNodes = nodeSet.size();
	for (int node : nodeSet) {
		vecs_type vecs;

		vec_type myVec;
		for (int v : nodeSet) {//for each node in nodeSet, assign value to map (node_i,node,j -> cost)
			if (v == node) {
				myVec[v] = make_pair(0, v);
				continue;
			}

			if (costMap[node].count(v) > 0) {
	    		myVec[v] = make_pair(costMap[node][v], v);
	    	} else {
	    		myVec[v] = make_pair(INT_MAX, INT_MAX);
	    	}
		}
		messageQueue.push(make_pair(node, myVec));
		vecs[node] = myVec;

		for (pair<int, int> neighbor: costMap[node]) {
    		int n_node = neighbor.first;
    		vec_type neighborVec;

    		//int n_cost = neighbor.second;
    		for (int v : nodeSet) {
    			neighborVec[v] = make_pair(INT_MAX, INT_MAX);
    		}
    		vecs[n_node] = neighborVec;
    	}
    	VecList[node] = vecs;
    	printTable(node);
	}
}

void WriteTableToOutput() {
	FILE * fpout;
	fpout = fopen("output.txt", "a");

	int numNodes = nodeSet.size();
	for (int i = 1; i <= numNodes; i++) {
    	for (int j = 1; j <= numNodes; j++) {
    		if (nodeSet.count(i) <= 0 || nodeSet.count(j) <= 0) {
    			continue;
    		}
    		if (f_tables[i][j].second == INT_MAX) {
    			continue;
    		}
    		if (i == j) {
    			fprintf(fpout, "%d %d %d\n", j, j, 0);
    			continue;
    		}
    		fprintf(fpout, "%d %d %d\n", j, f_tables[i][j].first, f_tables[i][j].second);
    	}
    	fprintf(fpout, "\n");
    }
    fclose(fpout);
}

void updateDistanceVector() {
    costMap2VecList();
	//int numNodes = nodeSet.size();
	f_tables.clear();

	while (!messageQueue.empty()) {
		pair<int, vec_type> message = messageQueue.front();
		messageQueue.pop();
		int messageSource = message.first;
		vec_type content = message.second;
		printf("Now message source = %d\n", messageSource);
		for (pair<int, int> neighbor: costMap[messageSource]) {
			int nodeId = neighbor.first;
			vector<int> updates;
			for (int node : nodeSet) {
				if (content[node].first < 
					VecList[nodeId][messageSource][node].first) {
					updates.push_back(node);
				}
			}	
			if (updates.size() == 0) {
				continue;
			}
			//update distance vector
			VecList[nodeId][messageSource] = content;
			printTable(nodeId);
			
			for (int i = 0; i < updates.size(); i++) {
				int update = updates[i];
				//update has to be node's neighbor
				if (costMap[nodeId][messageSource] + content[update].first <= VecList[nodeId][nodeId][update].first) {
					VecList[nodeId][nodeId][update].first = costMap[nodeId][messageSource] + content[update].first;
					VecList[nodeId][nodeId][update].second = min(messageSource, VecList[nodeId][nodeId][update].second);
				}
			}
			messageQueue.push(make_pair(nodeId, VecList[nodeId][nodeId]));
		}
	}

	for (int src : nodeSet) {
		unordered_map<int, pair<int, int> > f_table;
		printf("Printing f_table\n");
		for (int dst : nodeSet) {
			f_table[dst].first = VecList[src][src][dst].second;
			f_table[dst].second = VecList[src][src][dst].first;
			printf("From %d to %d next_hop= %d, cost= %d\n", src, dst, f_table[dst].first, f_table[dst].second);
 		}
 		printf("Finish printing f_table\n");
 		f_tables[src] = f_table;
	}
	WriteTableToOutput();
}



void WriteMessageToOutput(int src, int dst, char* message) {
	FILE * fpout;
	fpout = fopen("output.txt", "a");

	if (f_tables[src][dst].second == INT_MAX || 
	    nodeSet.count(src) <= 0 ||
		nodeSet.count(dst) <=0) {
		fprintf(fpout, "from %d to %d cost infinite hops unreachable message %s\n", src, dst, message);
		fclose(fpout);
		return;// or continue
	}

	vector<int> hops;
	int cur_hop = src;
	while (cur_hop != dst) {
		hops.push_back(cur_hop);
		cur_hop = f_tables[cur_hop][dst].first;	
	}
	fprintf(fpout, "from %d to %d cost %d hops ", src, dst, f_tables[src][dst].second);
	for (int hop : hops) {
		fprintf(fpout, "%d ", hop);
	}
	if (hops.empty()) {
		fprintf(fpout, " ");
	}
	fprintf(fpout, "message %s\n", message);
	fprintf(fpout, "\n");
	fclose(fpout);

}

void sendMessage(char* messageFileName) {
	//messagefile example: 2 1 send this message from 2 to 1
	ifstream in_fstream;
	string line;
	in_fstream.open(messageFileName);

	if (in_fstream.is_open()) {
		while (getline(in_fstream, line)) {
			int messageLength = line.length();
			char message[messageLength];
			int src = -1, dst = -1;
			sscanf(line.c_str(), "%d %d %[^\n]", &src, &dst, message);
			if (src == -1 || dst == -1) {
				continue;
			}
			printf("src= %d dst= %d ", src, dst);
			printf("message= %s\n", message);
			WriteMessageToOutput(src, dst, message);
		}
		in_fstream.close();
	} else {
		exit(2);
	}
}

void readChange(char* changeFileName, char* messageFileName) {
	ifstream in_fstream;
	string line;
	in_fstream.open(changeFileName);

	if (in_fstream.is_open()) {
		while (getline(in_fstream, line)) {
			stringstream ss(line);
			string node1, node2, cost;
			ss >> node1 >> node2 >> cost;
			//ss >> node2;
			//ss >> cost;
			int node1Id = atoi(node1.c_str());
			int node2Id = atoi(node2.c_str());
			int new_cost = atoi(cost.c_str());
			if (node1Id == 0 || node2Id == 0 || new_cost == 0) {
				continue;
			}
			if (new_cost == -999) {
				costMap[node1Id].erase(node2Id);
				costMap[node2Id].erase(node1Id);
				if (costMap.count(node1Id) <= 0) {
					nodeSet.erase(node1Id);
				}
				if (costMap.count(node2Id) <= 0) {
					nodeSet.erase(node2Id);
				}
			} else {
				nodeSet.insert(node1Id);
				nodeSet.insert(node2Id);
				costMap[node1Id][node2Id] = new_cost;
				costMap[node2Id][node1Id] = new_cost;
			}
			updateDistanceVector();
			sendMessage(messageFileName);
		}
		in_fstream.close();
	} else {
		exit(1);
	}
}
int main(int argc, char** argv) {
    //printf("Number of arguments: %d", argc);
    if (argc != 4) {//argv[1]:topofile,[2]:messagefile ,[3]:changesfile
        printf("Usage: ./distvec topofile messagefile changesfile\n");
        return -1;
    }

    
    readTopoFile(argv[1]);
    //costMap2VecList();
    updateDistanceVector();
    sendMessage(argv[2]);
	readChange(argv[3], argv[2]);
/*
    FILE *fpOut;
    fpOut = fopen("output.txt", "w");
    fclose(fpOut);
*/
    return 0;
}

