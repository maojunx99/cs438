#include <algorithm>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h> 
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <iterator>
#include <sstream>

using namespace std;

int nodesNum;
int pktSize;
int maxAttempt;
int randomRange;
int maxrandomRange;
double totalTime;
double usageTime;
bool isOccupied;
vector<int> countdown;
vector<int> randomList;

void parseInput(char* path){
    ifstream in;
    in.open(path);
    string str;
    int val;
    for(int i = 0; i < 5; i ++){
        getline(in, str);
        int index = str.find(" ");
        string type = str.substr(0, index);
        if(type == "R"){
            int secondIndex = str.find(" ", index + 1);
            if(secondIndex != -1){
                val = atoi(str.substr(index + 1, secondIndex - index).c_str());
                randomRange = val;
                continue;
            }
        }else{
            val = atoi(str.substr(index + 1, str.length() - index - 1).c_str());
        }
        if(type == "N"){
            nodesNum =  val;
        }else if(type == "L"){
            pktSize = val;
        }else if(type == "M"){
            maxAttempt = val;
        }else if(type == "T"){
            totalTime = val;
        }
    }
    maxrandomRange = randomRange;
    for(int i = 0; i < maxAttempt - 1; i ++){
        maxrandomRange = maxrandomRange * 2;
    }
}

int random(int id, int count){
    return (id + count)%(randomList[id]);
}

void simulation(){
    for(int i = 0; i < nodesNum; i ++){
        randomList.push_back(randomRange);
        int r = random(i, 0);
        countdown.push_back(r);
    }
    int rounds = 0;
    while(rounds < totalTime){
        int min = INT16_MAX;
        vector<int> minNumList;
        for(int j = 0; j < nodesNum; j ++){
            if(countdown[j] < min){
                min = countdown[j];
            }
            if(countdown[j] == 0){
                minNumList.push_back(j);
            }
        }
        if(min > 0){
            for(int k = 0; k < nodesNum; k ++){
                countdown[k]--;
            }
        }else if(min == 0){
            if(minNumList.size() == 1){
                rounds += pktSize;
                usageTime += pktSize;
                countdown[minNumList[0]] = random(minNumList[0], rounds);
                continue;
            }
            for(int l : minNumList){
                if(randomList[l] == maxrandomRange){
                    randomList[l] = randomRange;
                }else{
                    randomList[l] = randomList[l] * 2;
                }
                countdown[l] = random(l, rounds + 1);
            }
        }
        rounds++;
    }
}

int main(int argc, char** argv) {
    //printf("Number of arguments: %d", argc);
    if (argc != 2) {
        printf("Usage: ./csma input.txt\n");
        return -1;
    }
    parseInput(argv[1]);
    simulation();
    FILE *fpOut;
    fpOut = fopen("output.txt", "w");
    fprintf(fpOut, "%.2f\n", usageTime/totalTime);
    fclose(fpOut);
    return 0;
}

