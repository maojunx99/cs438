/*
** server.c -- a stream socket server demo
*/
#include <iostream>
#include <string>
#include <cstring>
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <sstream>
using namespace  std;
//#define PORT "3490"  // the port users will be connecting to

#define BACKLOG 10	 // how many pending connections queue will hold
#define DATA_BUFFER_SIZE 5000
#define BUFFER_SIZE 1024
void sigchld_handler(int s){
	while(waitpid(-1, NULL, WNOHANG) > 0);
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa){
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}


string return_file(string s,int fd){

    //const char* log = LOG_FILE_DIR;
    std::string command_str= s;
    //temp += log;
    const char *command = command_str.c_str();

    //fp = fopen(command, "r");

    //cout<<"grep_command is :";//9-11
    //cout<<grep_command<<endl;//9-11
    char buffer[BUFFER_SIZE];
    //std::array<char, BUFFER_SIZE> buffer;
    std::string result;
    //std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(command, "r"), pclose);//commented

    //std::cout<<"inside grep_log()";//9-11
    /*if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }*/ // commented

    //cout<<"now start return grep result"<<endl;//9-11
    int ret_val;
    int ret_val_grep_result;
    //while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
    int count=0;
    cout<<"Received data: "<<command<<endl;
    FILE* fp = NULL;

    char space_delimiter = ' ';
    istringstream str_stream(command_str);
    vector<string> words{};
    string t;
    while (getline(str_stream, t, space_delimiter)) {	//这里单引号要注意
        words.push_back(t);
    }

    for (int i=0;i<words.size();i++) {
        cout <<i<<"-th word is: "<< words[i] << endl;
    }

    if(words[0]=="GET"){
        long long byte_count=0;
        cout<<"first word is GET"<<endl;
        string file_directory_str(words[1],1);
        fp = fopen(file_directory_str.c_str(), "rb");
        if (fp == NULL) {
            const char* notfound_msg="HTTP/1.1 404 Not Found\r\n\r\n";
            int notfound_msg_return_result=send(fd,notfound_msg,strlen(notfound_msg),0);
            fprintf(stderr, "can't open %s: %s\n", file_directory_str.c_str(), strerror(errno));
            return strerror(errno);
        }
        const char* ok_msg="HTTP/1.1 200 OK\r\n\r\n";
        cout<<"ok msg is HTTP/1.1 200 OK"<<endl;
        int ok_msg_return_result=send(fd,ok_msg,strlen(ok_msg),0);
        cout<<"ok_msg_return_result="<<ok_msg_return_result<<endl;
        if(ok_msg_return_result==0){
            cout<<"ok msg sent successfully"<<endl;
        }

        //int blank_line_return_result=send(fd,"\r\n",strlen("\r\n"),0);
        /*
        while (fgets(buffer, sizeof(buffer), (FILE*)fp)) {
            ret_val_grep_result=send(fd,buffer,sizeof(buffer),0);
            byte_count+=ret_val_grep_result;
            //cout<<"Sent:"<<buffer<<endl;
            result += buffer;//.data();
            count++;
        }*/
        while(!feof(fp)) {
            size_t ret_value = fread(buffer, sizeof(char), BUFFER_SIZE, fp);
            result+=buffer;
            if(ret_value <= 0) {
                printf("finish reading.\n");
                break;
            } else if(ret_value == BUFFER_SIZE){ // error handling
                printf("reading file\n");
                if (send(fd, buffer, BUFFER_SIZE, 0) == -1){
                    //result+=buffer;
                    perror("send");
                }

            } else {
                printf("reading file\n");
                if (send(fd, buffer, (int)ret_value, 0) == -1){
                    perror("send");
                }

            }
        }


        cout<<"total byte sent:"<<byte_count<<endl;
        cout<<"ret_val_grep_result="<<ret_val_grep_result<<endl;
        if(ret_val_grep_result==0){
            cout<<"request content sent successfully"<<endl;
        }
        /*char null_buffer[2]=" ";
        //sleep(1);
        ret_val_grep_result=send(fd,null_buffer,sizeof(null_buffer),0);
        if(ret_val_grep_result==0){
            cout<<"final byte sent successfully"<<endl;
        }*/
    }else{
        const char* bad_request_msg="400 Not Found\r\n\r\n";
        int bad_request_msg_return_result=send(fd,bad_request_msg,strlen(bad_request_msg),0);
        if(bad_request_msg_return_result==0){
            cout<<"400 Not Found. sent successfully"<<endl;
        }
    }

    fclose(fp);
    cout<<"finish sending whole file"<<endl;
    return result;
}

int main(int argc, char** argv){
    string port_str=argv[1];
    int port=atoi(port_str.c_str());
	int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage their_addr; // connector's address information
	socklen_t sin_size;
	struct sigaction sa;
	int yes=1;
	char s[INET6_ADDRSTRLEN];
	int rv,ret_val;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	if ((rv = getaddrinfo(NULL, port_str.c_str(), &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("server: socket");
			continue;
		}

		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
				sizeof(int)) == -1) {
			perror("setsockopt");
			exit(1);
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("server: bind");
			continue;
		}

		break;
	}

	if (p == NULL)  {
		fprintf(stderr, "server: failed to bind\n");
		return 2;
	}

	freeaddrinfo(servinfo); // all done with this structure

	if (listen(sockfd, BACKLOG) == -1) {
		perror("listen");
		exit(1);
	}

	sa.sa_handler = sigchld_handler; // reap all dead processes
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("sigaction");
		exit(1);
	}

	printf("server: waiting for connections...\n");

	while(1) {  // main accept() loop
		sin_size = sizeof their_addr;
		new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
		if (new_fd == -1) {
			perror("accept");
			continue;
		}

		inet_ntop(their_addr.ss_family,
			get_in_addr((struct sockaddr *)&their_addr),
			s, sizeof s);
		printf("server: got connection from %s\n", s);

		if (!fork()) { // this is the child process
            close(sockfd); // child doesn't need the listener
		    /*********************************added***************************************/
		    char buf[DATA_BUFFER_SIZE];
            ret_val = recv(new_fd, buf, DATA_BUFFER_SIZE, 0);
            if (ret_val == 0) {
                printf("Closing connection for fd:%d\n", new_fd);//9-11
                close(new_fd);
                new_fd = -1; /* Connection is now closed */
            }
            if (ret_val > 0) {
                std::cout<<"buf="<<buf<<endl;
                string ans = return_file(buf,new_fd);
                bzero(buf,DATA_BUFFER_SIZE);
            }
            if (ret_val == -1) {
                printf("recv() failed for fd: %d [%s]\n",new_fd, strerror(errno));
                //break;
            }
            /*********************************above added***************************************/

			/*if (send(new_fd, "Hello, world!", 13, 0) == -1){
                perror("send");
			}*/

			close(new_fd);
			exit(0);
		}
		close(new_fd);  // parent doesn't need this
	}

	return 0;
}

