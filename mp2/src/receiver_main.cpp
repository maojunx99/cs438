/* 
 * File:   receiver_main.c
 * Author: 
 *
 * Created on
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>
#include <fstream>
#include <errno.h>
#include <unordered_map>
#include "sender.h"
using namespace std;

//#define DATA_BUFFER_SIZE 1450
//#define SEQ_SIZE 4
//#define DATA_LENGTH 4

struct sockaddr_in si_me, si_other;
int s, slen;

void diep(char *s) {
    perror(s);
    exit(1);
}

typedef struct {
    unsigned long int seqNum;
    int finalflag;
    unsigned int dataLen;
    char data[DATA_BUFFER_SIZE];
} Packet;


void reliablyReceive(unsigned short int myUDPport, char* destinationFile) {
    unordered_map<unsigned long int, Packet*> hashMap;
    int sock_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(sock_fd < 0)  
    {  
        perror("socket");  
        exit(1);  
    }  
     
    //int len;  
    memset(&si_me, 0, sizeof(struct sockaddr_in));
    si_me.sin_family = AF_INET;
    si_me.sin_port = htons(myUDPport);
    si_me.sin_addr.s_addr = htonl(INADDR_ANY); 
    //len = sizeof(si_me);  
        
    if(bind(sock_fd, (struct sockaddr *)&si_me, sizeof(si_me)) < 0)  
    {  
        perror("bind error:");  
        exit(1);  
    }  
    int recv_num;
    int send_num;
    //char send_buf[20] = "ack";  
    char recv_buf[DATA_BUFFER_SIZE];  
	/* Now receive data and send acknowledgements */    
    printf("server: waiting for connections...\n");
    struct sockaddr_storage otherAdd;
    socklen_t otherAddLen;
    otherAddLen = sizeof otherAdd;
    long int nextPacketNum = 0;
    long int ackNum = - 1;
    bool endFlag = false;
    unsigned int lastSeqNum = 0;
    //ofstream outfile;  
    FILE* fp = fopen(destinationFile, "wb");
    //outfile.open(destinationFile);
    //int seqNum, dataLen;
    //char data[DATA_BUFFER_SIZE];
    char ackNum_char[20];
	while(1) { 
        Packet* curPacket = (Packet*) malloc(sizeof(Packet));
        recv_num = recvfrom(sock_fd, recv_buf, sizeof(recv_buf), 0, (struct sockaddr*) &otherAdd, &otherAddLen);  
        //printf("recvnum=%d",recv_num);
        if(recv_num < 0) {  
            perror("recvfrom error:");  
            exit(1);  
        }  

        memcpy(curPacket, recv_buf, sizeof(Packet));
        //printf("recvnum=%s",curPacket->data);
        //printf("recvnum=%s",curPacket->data);
        //printf("recvnum=%u",curPacket->seqNum);
        if (curPacket->finalflag==1 || curPacket->dataLen == 0) {
            endFlag = true;
            lastSeqNum = curPacket->seqNum;
        }

        if (curPacket->seqNum >= nextPacketNum
                && hashMap.find(curPacket->seqNum) == hashMap.end()) {
            hashMap.insert({curPacket->seqNum, curPacket});
        }else {
            free(curPacket);
        }
        curPacket = nullptr; 

        while(hashMap.find(nextPacketNum) != hashMap.end()){
            Packet *tempPacket = hashMap.at(nextPacketNum);
            fwrite(tempPacket->data, 1, tempPacket->dataLen, fp);
            hashMap.erase(nextPacketNum);
            nextPacketNum++;
            free(tempPacket);
        }
        ackNum=(nextPacketNum-1);
        memcpy(ackNum_char, &(ackNum), sizeof(ackNum));
        send_num = sendto(sock_fd, ackNum_char, sizeof(ackNum) , 0, (struct sockaddr *)&otherAdd, otherAddLen);  
      
        if(send_num < 0) {  
            perror("sendto error:");  
            exit(1);  
        }  

        if(endFlag && lastSeqNum == nextPacketNum-1){
            break;
        }
        /*if(curPacket->finalflag==1){
            break;
        }*/
    }
    fclose(fp);
    // Close the file.
    close(sock_fd);
	printf("%s received.", destinationFile);
    return;

}

/*
 * 
 */
int main(int argc, char** argv) {

    unsigned short int udpPort;

    if (argc != 3) {
        fprintf(stderr, "usage: %s UDP_port filename_to_write\n\n", argv[0]);
        exit(1);
    }

    udpPort = (unsigned short int) atoi(argv[1]);

    reliablyReceive(udpPort, argv[2]);
}

