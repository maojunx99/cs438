#include <iostream>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fstream>
using namespace std;
#define BUFFER_SIZE 1024
struct timeval timeout = { 3,0 };

int main(int argc, char** argv) {
    struct sockaddr_in saddr;
    int sd, ret_val;
    struct hostent* local_host;
    string str = argv[1];
    string port = "80";
    string file;
    string query;
    string ip;
    int num1 = 0;
    int add1 = 0;
    int num2 = 0;
    int add2 = 0;
    for (int i = 0; i < str.length(); i++) {
        if (argv[1][i] == ':') {
            num1++;
            if (num1 == 2) {
                add1 = i;
            }
        }
        if (argv[1][i] == '/') {
            num2++;
            if (num2 == 3) {
                add2 = i;
            }
        }
    }
    if (num1 == 2) {
        port = str.substr(add1 + 1, add2 - add1);
    }
    file = str.substr(add2);
    query = "GET " + file + " HTTP/1.1\r\n\r\n";
    ip = str.substr(7, add2 - 7);

    /* Step1: create a TCP socket */
    sd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sd == -1) {
        fprintf(stderr, "socket failed [%s]\n", strerror(errno));
        return -1;
    }
    printf("Created a socket with sd: %d\n", sd);
  
    const char* host = ip.c_str();
    const char* cquery = query.c_str();

    cout<<"ip is: "<<host<<endl;
    cout<<"port is: "<<port<<endl;
    //const char* cquery = argv[1];
    /* Let us initialize the server address structure */
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(atoi(port.c_str()));
    local_host = gethostbyname(host);
    saddr.sin_addr = *((struct in_addr*)local_host->h_addr);

    /* Step2: connect to the TCP server socket */
    ret_val = connect(sd, (struct sockaddr*)&saddr, sizeof(struct sockaddr_in));
    if (ret_val == -1) {
        fprintf(stderr, "connect failed [%s]\n", strerror(errno));
        close(sd);
        return -1;
    }
    printf("The Socket is now connected\n");
    setsockopt(sd, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(struct timeval));
    printf("Let us sleep before we start sending data\n");
    sleep(1);

    /* Next step: send some data */
    cout<<"sizeof(cquery) is: "<<sizeof(cquery)<<endl;
    cout<<"strlen(cquery) is: "<<strlen(cquery)<<endl;
    ret_val = send(sd, cquery, strlen(cquery), 0);
    printf("Successfully sent data (len %d bytes): %s\n",
        ret_val, cquery);

    int tmp;

    char buffer[BUFFER_SIZE];
    bzero(buffer, BUFFER_SIZE);
    ofstream outfile;
    outfile.open("output");
    FILE* fp = fopen("output", "w");
    while (true) {
        //if (tmp==-1) break;
        tmp = recv(sd, buffer, BUFFER_SIZE, 0);
        if(tmp <= 0) {
            break;
        }
        fwrite(buffer, tmp, 1, fp);
    }
    fclose(fp);
    // Close the file.
    outfile.close();
    /* Last step: close the socket */
    close(sd);
    return 0;
}
