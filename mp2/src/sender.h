#define PACKET_ID_SIZE 4 
 #define CONTENT_LEN_SIZE 4 
 #define SLOW_START_INIT_SIZE 1 
 #define SENDER_BUFFER_SIZE 1024 
 #define RECV_BUFFER_SIZE 1024 
 #define CONTENT_SIZE 1016 
 #define SOCKET_TIMEOUT_MILLISEC 25 
 #define SOCKET_TIMEOUT_MICROSEC SOCKET_TIMEOUT_MILLISEC * 1000 
 #define SS_THRESHOLD 64 
 #define ADD_WINDOW_SIZE_WHEN_HALF 150 
 #define ALPHA 0.8 //0.125//previous 
 #define BETA 0.8//0.25//previous 
 #define TO_INIT_RTT 1 
 #define INITIAL_RTT_MILLISEC 25 
 long RTT_UPPER_BOUND_MILLI_SECOND=2500; 
 long TIMEOUT_UPPER_BOUND_MILLI_SECOND=1700; 
 #define DEBUG_LOG 0 
 #define DEBUG_LOAD_PACKET 0 && DEBUG_LOG 
 #define DEBUG_PACKET_TRAFFIC 1 && DEBUG_LOG 
 #define DEBUG_RTT_LOG 0 