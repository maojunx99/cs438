/* 
 * File:   sender_main.cpp
 * Author: Pengcheng Xu
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
#include <sys/stat.h>
#include <signal.h>
#include <string.h>
#include <sys/time.h>
#include <netdb.h>
#include <math.h>
#include <iostream>
#include <deque>
#include <chrono>
#include <sstream>
#include <fstream>
#include <unordered_map>
#include "sender.h"

using namespace std;

enum SenderAction {sendNew, resend, waitACK };
struct sockaddr_in si_other;
int s, slen;
string ltos(long l){
    ostringstream os;
    os<<l;
    string result;
    istringstream is(os.str());
    is>>result;
    return result;

}
void diep(char *s) {
    perror(s);
    exit(1);
}
class Packet {
    private:
    int id_, content_length_;
    char content_[CONTENT_SIZE];
    public:
    Packet(int id, int content_length, char* buf) {
        id_ = id;
        content_length_ = content_length;
        memcpy(content_, buf, CONTENT_SIZE);
    }
    int id() {
        return id_;
    }
    void initData(char *buf) {
        memcpy(buf, &id_, PACKET_ID_SIZE);                   // int, 4 byte
        memcpy(buf+PACKET_ID_SIZE, &content_length_, CONTENT_LEN_SIZE);        // int, 4 byte
        memcpy(buf+PACKET_ID_SIZE+CONTENT_LEN_SIZE, content_, CONTENT_SIZE);  // 4088 byte
    }
};

class ReliableSender;

void timeoutAction(ReliableSender *sender);

class State {
    protected:
    ReliableSender *selfSenderInfo_;
    public:
    State();
    State(ReliableSender *sender);
    virtual void dupACK() = 0;
    virtual void newACK(int ackId) = 0;
    void timeout();
    virtual ~State();
};

State::State() {}
State::State(ReliableSender *sender) {
    selfSenderInfo_ = sender;
    
}
void State::timeout() {
    timeoutAction(selfSenderInfo_);
}
State::~State() {};

class FastRecovery : State {
    public:
    FastRecovery(ReliableSender *input);
    void dupACK();
    void newACK(int ackId);
};
class CongestionAvoid : State {
    public:
    CongestionAvoid(ReliableSender *input);
    void dupACK();
    void newACK(int ackId);
};
class SlowStart : State {
    public:
    SlowStart(ReliableSender *input);
    void dupACK();
    void newACK(int ackId);
};


class ReliableSender {
    private:
    int lastReceivedACKId_;

    FILE *fp_;
    unsigned long long remainingBytesToRead_;  // may not equal to size of file
    bool isFileExhausted_;  // true, if the file is exhausted or remainingBytesToRead_ becomes 0 or negative
    
    State *state_;
    deque<Packet> sentAndNotAckPacketsQueue;
    char fileReadBuffer_[CONTENT_SIZE];
    char sendBuffer_[SENDER_BUFFER_SIZE];
    char recvBuffer_[RECV_BUFFER_SIZE];
    int socket_;
    struct addrinfo *receiverInfo_;
    struct timeval timeoutVal_;

    deque<Packet> loadNewPacketsFromFile() {
        deque<Packet> new_deq;
        if (isFileExhausted_) return new_deq;
        int packetIdToAdd;
        if(sentAndNotAckPacketsQueue.size() == 0){
            packetIdToAdd=leftPacketId_;
        }else{
            packetIdToAdd=sentAndNotAckPacketsQueue.back().id() + 1;
        }
        int bytesRead;
        int contentLen;
        int newPacketCount = ((int)ceil(windowSize_)) - sentAndNotAckPacketsQueue.size();
        if (DEBUG_LOAD_PACKET) {
            printf("DEBUG: newPacketCount: %d, windowSize: %d, deq size: %lu\n",
                    newPacketCount, ((int)ceil(windowSize_)), sentAndNotAckPacketsQueue.size());
        }
        while (newPacketCount-- > 0) {
            if (remainingBytesToRead_ == 0) {
                // to create the FIN packet 
                Packet packet(packetIdToAdd++, 0, fileReadBuffer_);
                new_deq.push_back(move(packet));
                if (DEBUG_LOAD_PACKET) {
                    printf("DEBUG: create FIN packet: %d, size: %d\n",
                            packet.id(), 0);
                }
                isFileExhausted_ = true;
                break;
            }
            bytesRead = fread(fileReadBuffer_, 1, CONTENT_SIZE, fp_);
            contentLen = remainingBytesToRead_ >= bytesRead ?
                    bytesRead : remainingBytesToRead_;
            Packet packet(packetIdToAdd++, contentLen, fileReadBuffer_);
            if (DEBUG_LOAD_PACKET) {
                printf("DEBUG: create packet: %d, size: %d, bytes read: %d\n",
                        packet.id(), contentLen, bytesRead);
            }
            remainingBytesToRead_ = remainingBytesToRead_ <= bytesRead ?
                    0 : remainingBytesToRead_ - bytesRead;
            new_deq.push_back(move(packet));

            if (bytesRead == 0) {
                isFileExhausted_ = true;
                break;
            }
        }
        return new_deq;
    }

    static int getMaxACKId(char *buffer, int bytesRead) {
        int MaxACKId = 0,packetId, i = 0;
        while (i + PACKET_ID_SIZE<= bytesRead) {
            memcpy(&packetId, buffer+i,  PACKET_ID_SIZE);
            if (packetId >  MaxACKId) {
                 MaxACKId = packetId;
            }
            i += PACKET_ID_SIZE;
        }
        return  MaxACKId;
    }


    public:
    float windowSize_;
    int ssthresh_;
    int leftPacketId_;  // the left side of the sliding window, should be the next ACK id
    int dupACKCount_;
    long estimatedRTT_;
    long sampleRTT_;
    long devRTT_;
    long timeoutVal_nanosec_;
    SenderAction nextAction_;
    unordered_map<int,long> send_timestamp_;//chrono::high_resolution_clock::time_point
    unordered_map<int,long> recv_timestamp_;
    unordered_map<int,long> RTT_timestamp_;
    ReliableSender(FILE *fp, unsigned long long numBytesToTransfer, int socket,
            struct addrinfo *receiverInfo) {
        fp_ = fp;
        isFileExhausted_ = false;
        remainingBytesToRead_ = numBytesToTransfer;

        windowSize_ = SLOW_START_INIT_SIZE;
        ssthresh_ = SS_THRESHOLD;
        
        if(TO_INIT_RTT){
            estimatedRTT_=1000000*INITIAL_RTT_MILLISEC;
            sampleRTT_=1000000*INITIAL_RTT_MILLISEC;
        }
        devRTT_=0;
        timeoutVal_.tv_sec = 0;
        timeoutVal_.tv_usec = SOCKET_TIMEOUT_MICROSEC;
        
        leftPacketId_ = 0;
        lastReceivedACKId_ = -1;
        dupACKCount_ = 0;
        nextAction_ = sendNew;
        socket_ = socket;
        receiverInfo_ = receiverInfo;
        state_ = nullptr;
        memset(recvBuffer_, 0, RECV_BUFFER_SIZE);
    }

    void changeState(State *state) {
        if (state_ != nullptr) { // delete the older state
            delete state_; 
        }
        state_ = state;
    }

    int sendOnePacket(Packet *packet) {
        int sentBytes;
        if (DEBUG_PACKET_TRAFFIC) {
            printf("Now sending packet %d\n", packet->id());
        }
        packet->initData(sendBuffer_);
        /*record send time*/
        auto sendPacketTime = chrono::high_resolution_clock::now();
        auto sendPacketTime_nanosec = sendPacketTime.time_since_epoch();
        //sendPacketTime_nanosec.count();
        send_timestamp_[packet->id()]=sendPacketTime_nanosec.count();
        //printf("Packet%d send time: %ld",packet->id(),send_timestamp_[packet->id()]);
        if(DEBUG_RTT_LOG){
            cout<<"Packet"<<packet->id()<<"send time: "<<send_timestamp_[packet->id()]<<endl;
        }
        
        sentBytes = sendto(socket_, sendBuffer_, SENDER_BUFFER_SIZE, 0,
                receiverInfo_->ai_addr, receiverInfo_->ai_addrlen);
        if (sentBytes == -1) {
            perror("Fail to send packet");
        }
        
        return sentBytes;
    }

    void sendNewPackets() {
        deque<Packet> newPackets = loadNewPacketsFromFile();
        for (auto it = newPackets.begin(); it != newPackets.end(); it++) {
            // send one packet
            sendOnePacket(&(*it));
            // push packet to the sliding window
            sentAndNotAckPacketsQueue.push_back(*it);
        }
    }

    void resendPacket() {
        sendOnePacket(&sentAndNotAckPacketsQueue[0]);
    }

    void setSocketTimeout() {// set once or set every time before calling recvfrom
        if (setsockopt(socket_, SOL_SOCKET, SO_RCVTIMEO, &timeoutVal_,
                       sizeof(timeoutVal_)) < 0) {
            perror("error: fail to set socket timeout");
        }
    }

    int getACKId() {
        
        int recvBytes = recvfrom(socket_, recvBuffer_, RECV_BUFFER_SIZE, 0, NULL, NULL);
        
        if (recvBytes < 0) {
            return -1;  // timeout
        } else if (recvBytes <  PACKET_ID_SIZE) {
            perror("error: Cannot decode ACK id");
            return lastReceivedACKId_;
        } else {
            return max(getMaxACKId(recvBuffer_, recvBytes), lastReceivedACKId_);
        }
    }

    bool finishSendingAll() {//finishSendingAll
        return isFileExhausted_ && sentAndNotAckPacketsQueue.size() == 0;
    }

    void deleteACKedPacketsFromWindow(int ackId) {
        while (sentAndNotAckPacketsQueue.size() > 0 && sentAndNotAckPacketsQueue[0].id() <= ackId) {
            sentAndNotAckPacketsQueue.pop_front();
        }
    }

    void mainLoop() {
        while (!finishSendingAll()) {
            switch(nextAction_) {
                case sendNew: sendNewPackets();   break;
                case resend: resendPacket();   break;
                case waitACK: break;//get ACK below 
            }
            setSocketTimeout();//set time out
            int ackId = getACKId();
            /*record recv time*/
            auto recvPacketTime = chrono::high_resolution_clock::now();
            auto recvPacketTime_nanosec = recvPacketTime.time_since_epoch();
            //recvacketTime_nanosec.count();
            recv_timestamp_[ackId]=recvPacketTime_nanosec.count();
            //printf("Packet%d recv time: %ld",ackId,recv_timestamp_[ackId]);
            
            
            RTT_timestamp_[ackId]=(recvPacketTime_nanosec.count() - send_timestamp_[ackId]);
            //printf("Packet%d RTT: %ld",ackId,RTT_timestamp_[ackId]);
            

            if(ackId>=0 && RTT_timestamp_[ackId]<1000000*RTT_UPPER_BOUND_MILLI_SECOND){
                /*if(TO_INIT_RTT && RTT_timestamp_[ackId]<1000*RTT_UPPER_BOUND_MICRO_SECOND){
                    sampleRTT_=RTT_timestamp_[ackId];
                }else if (!TO_INIT_RTT && RTT_timestamp_[ackId]<1000*RTT_UPPER_BOUND_MICRO_SECOND){
                    sampleRTT_=RTT_timestamp_[ackId];
                }*/
                sampleRTT_=RTT_timestamp_[ackId];                
            }
            /*estimatedRTT_ = (1-ALPHA)*estimatedRTT_ + ALPHA*sampleRTT_;
            devRTT_ = (1-BETA)*devRTT_ + BETA*abs(sampleRTT_ - estimatedRTT_);
            timeoutVal_nanosec_ = estimatedRTT_ + 4*devRTT_;*/
            estimatedRTT_ = (ALPHA)*estimatedRTT_ + (1-ALPHA)*sampleRTT_;
            devRTT_ = (BETA)*devRTT_ + (1-BETA)*abs(sampleRTT_ - estimatedRTT_);
            
            timeoutVal_nanosec_ = estimatedRTT_ + 4*devRTT_;
            if(timeoutVal_nanosec_/1000000<TIMEOUT_UPPER_BOUND_MILLI_SECOND){
                timeoutVal_.tv_sec = timeoutVal_nanosec_/1000000000;//s
                timeoutVal_.tv_usec =(timeoutVal_nanosec_-timeoutVal_.tv_sec*1000000000)/1000;//us
            }
           
            if(DEBUG_RTT_LOG){
                cout<<"Packet"<<ackId<<"recv time: "<<recv_timestamp_[ackId]<<endl;
                cout<<"Packet"<<ackId<<"RTT time: "<<RTT_timestamp_[ackId]<<endl;
                cout<<"estimatedRTT = (1-ALPHA)*estimatedRTT_ + ALPHA*sampleRTT_="<<estimatedRTT_<<endl;
                cout<<"devRTT_ = (1-BETA)*devRTT_ + BETA*abs(sampleRTT_ - estimatedRTT_)= "<<devRTT_<<endl;
                cout<<"timeoutVal_nanosec_ = estimatedRTT_ + 4*devRTT_= "<<timeoutVal_nanosec_<<endl;
                cout<<"timeoutVal_="<<timeoutVal_.tv_sec<<"s+"<<timeoutVal_.tv_usec<<"us"<<endl;
            }
            
            //timeoutVal_.
            if (DEBUG_LOG) {
                printf("DEBUG: receive ACK %d\n", ackId);
            }
            // printf("window size= %f\n", windowSize_);
            if (ackId == -1) { // timeout
                state_->timeout();
            } else if (ackId == lastReceivedACKId_) {//duplicate ACK
                state_->dupACK();
            } else {  // new ACK arrives
                lastReceivedACKId_ = ackId;
                deleteACKedPacketsFromWindow(ackId);
                state_->newACK(ackId);
            }
        }
    }
};


CongestionAvoid::CongestionAvoid(ReliableSender *input) : State(input){}
void CongestionAvoid::newACK(int ackId) {
    selfSenderInfo_->dupACKCount_ = 0;
        int step = ackId - selfSenderInfo_->leftPacketId_ + 1;
        while (step-- > 0) {
            selfSenderInfo_->windowSize_ =
                    selfSenderInfo_->windowSize_ + SLOW_START_INIT_SIZE * 2 * (SLOW_START_INIT_SIZE / floor(selfSenderInfo_->windowSize_));
        }
        selfSenderInfo_->nextAction_ = sendNew;
        selfSenderInfo_->leftPacketId_ = ackId + 1;
}
void CongestionAvoid::dupACK() {
        int dupACK_threshold=3;
        selfSenderInfo_->dupACKCount_++;
        selfSenderInfo_->nextAction_ = waitACK;
        if (selfSenderInfo_->dupACKCount_ >= dupACK_threshold) {
            selfSenderInfo_->ssthresh_ = round(selfSenderInfo_->windowSize_ ) + ADD_WINDOW_SIZE_WHEN_HALF;
            selfSenderInfo_->windowSize_ = selfSenderInfo_->ssthresh_ + dupACK_threshold;
            selfSenderInfo_->nextAction_ = resend;
            FastRecovery *fastRecoveryState = new FastRecovery(selfSenderInfo_);
            selfSenderInfo_->changeState((State *) fastRecoveryState);
        }
}


SlowStart::SlowStart(ReliableSender *input) : State(input){}
void SlowStart::newACK(int ackId) {
    selfSenderInfo_->dupACKCount_ = 0;
    int step = ackId - selfSenderInfo_->leftPacketId_ + 1;
    selfSenderInfo_->windowSize_ += SLOW_START_INIT_SIZE * step;
    selfSenderInfo_->nextAction_ = sendNew;
    selfSenderInfo_->leftPacketId_ = ackId + 1;
    if (selfSenderInfo_->windowSize_ >= selfSenderInfo_->ssthresh_) {
        CongestionAvoid *congestionAvoidState = new CongestionAvoid(selfSenderInfo_);
        selfSenderInfo_->changeState((State *) congestionAvoidState);
    }
}
void SlowStart::dupACK() {
    selfSenderInfo_->dupACKCount_++;
    selfSenderInfo_->nextAction_ = waitACK;
}

FastRecovery::FastRecovery(ReliableSender *input) : State(input){}
void FastRecovery::newACK(int ackId) {
    selfSenderInfo_->dupACKCount_ = 0;
    selfSenderInfo_->windowSize_ = selfSenderInfo_->ssthresh_;
    selfSenderInfo_->nextAction_ = sendNew;
    CongestionAvoid *congestionAvoidState = new CongestionAvoid(selfSenderInfo_);
    selfSenderInfo_->changeState((State *) congestionAvoidState);
    selfSenderInfo_->leftPacketId_ = ackId + 1;
}
void FastRecovery::dupACK() {
    selfSenderInfo_->windowSize_ += SLOW_START_INIT_SIZE;
    selfSenderInfo_->nextAction_ = sendNew;
}



void timeoutAction(ReliableSender *selfSenderInfo_) {
    SlowStart *slowStartState = new SlowStart(selfSenderInfo_);
    selfSenderInfo_->changeState((State *) slowStartState);
    selfSenderInfo_->ssthresh_ = round(selfSenderInfo_->windowSize_ / 2) + 1;
    selfSenderInfo_->windowSize_ = SLOW_START_INIT_SIZE;
    selfSenderInfo_->dupACKCount_ = 0;
    selfSenderInfo_->nextAction_ = resend;
}


void reliablyTransfer(char* hostname,char*  hostUDPport /*unsigned short int hostUDPport*/, char* filename, unsigned long long int bytesToTransfer) {
    struct addrinfo info, *serverInfo;
    //Open the file
    FILE *fp;
    fp = fopen(filename, "rb");
    if (fp == NULL) {
        printf("Could not open file to send.");
        exit(1);
    }

	/* Determine how many bytes to transfer */

    slen = sizeof (si_other);
    memset(&info, 0, sizeof(info));
    info.ai_family = AF_INET;
    info.ai_socktype = SOCK_DGRAM;
    //const char* hostUDPport_char=htons(hostUDPport);
    if (getaddrinfo(hostname, hostUDPport, &info, &serverInfo) != 0) {
        fprintf(stderr, "getaddrinfo(hostname, hostUDPport, &info, &serverInfo) failed\n");
        return;
    }

    if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1){
        char *errorMsg =const_cast<char*>("Socket Error");
        diep(errorMsg);
    }
    
    /*
    memset((char *) &si_other, 0, sizeof (si_other));
    si_other.sin_family = AF_INET;
    si_other.sin_port = htons(hostUDPport);

    if (inet_aton(hostname, &si_other.sin_addr) == 0) {
        fprintf(stderr, "inet_aton() failed\n");
        exit(1);
    }*/

    
    //ofstream fout;           //create ofstream
    char* logFileName=strcat(filename, "output.log" );
    //fout.open(filename);   //connect a file
    ofstream file(logFileName);
    streambuf *x = cout.rdbuf( file.rdbuf( ) );
    
    ReliableSender sender(fp, bytesToTransfer, s, serverInfo);
    SlowStart *currentState = new SlowStart(&sender);
    sender.changeState((State *) currentState);
    sender.mainLoop();
    
    cout.rdbuf(x);
    //fout.close(); 
    freeaddrinfo(serverInfo);
	/* Send data and receive acknowledgements on s*/

    printf("Closing the socket\n");
    close(s);
    return;

}


/*
 * 
 */
int main(int argc, char** argv) {

    //unsigned short int udpPort;
    unsigned long long int numBytes;

    if (argc != 5) {
        fprintf(stderr, "usage: %s receiver_hostname receiver_port filename_to_xfer bytes_to_xfer\n\n", argv[0]);
        exit(1);
    }
    //udpPort = (unsigned short int) atoi(argv[2]);
    numBytes = atoll(argv[4]);

    reliablyTransfer(argv[1], argv[2], argv[3], numBytes);

    return (EXIT_SUCCESS);
}


