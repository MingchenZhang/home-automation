
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <fcntl.h>
#include <netinet/tcp.h>

#include "radio_level1_gatewayConnector.h"
#include "util_debug.h"
#include "util_tools.h"

int gatewaySocket = -1;

int gateway_conn(char* server, char* port){
	if(gatewaySocket >= 0){
		warn("attempting new connection while connected\n");
		return -1;
	}
	
	struct addrinfo addrConfig, *addrInfo;
	memset(&addrConfig, 0, sizeof(addrConfig));
	addrConfig.ai_family = AF_UNSPEC;
	addrConfig.ai_socktype = SOCK_STREAM;
	
	int ret;
	if( (ret = getaddrinfo(server, port, &addrConfig, &addrInfo)) != 0 ){
		error("fail to get address info, %s\n", gai_strerror(ret));
		return -2;
	}
	
	struct addrinfo* cursor;
	int sock = 0;
	debug("ready to connect to %s\n", server);
	for(cursor = addrInfo; cursor!=NULL; cursor = cursor->ai_next){
		size_t addrStrLen = (cursor->ai_family==AF_INET)?INET_ADDRSTRLEN:INET6_ADDRSTRLEN;
		char printBuffer[addrStrLen];
		//void* addr = (cursor->ai_family==AF_INET)?(((struct sockaddr_in *)cursor)->sin_addr):(((struct sockaddr_in6 *)cursor)->sin6_addr);
		void* addr;
		if(cursor->ai_family==AF_INET)
			addr = &(((struct sockaddr_in *)(cursor->ai_addr))->sin_addr);
		else
			addr = &(((struct sockaddr_in6 *)(cursor->ai_addr))->sin6_addr);
		debug("attempt to connect to %s \n",
				inet_ntop(cursor->ai_family, addr, printBuffer, addrStrLen) );
		fflush(stderr);
		
		if( (sock = socket(cursor->ai_family, cursor->ai_socktype, 0)) <0 ){
			error("connect to %s failed. %s.\n", 
					inet_ntop(cursor->ai_family, addr, printBuffer, addrStrLen), 
					strerror(errno));
			continue;
		}
		
		if( connect(sock, cursor->ai_addr, cursor->ai_addrlen) == -1 ){
			warn("connect to %s failed. %s.\n", 
					inet_ntop(cursor->ai_family, addr, printBuffer, addrStrLen), 
					strerror(errno));
			close(sock);
			continue;
		}
		
		info("connected to [%s]:%s\n",printBuffer, port);
		break;
	}
	
	if(cursor == NULL) {
		error("all possible IP addresses tried and failed. client is unable to "
				"connect to server. \n");
		freeaddrinfo(addrInfo);
		return -2;
	}
	
	freeaddrinfo(addrInfo);
	
	gatewaySocket = sock;
	return 0;
}

static pthread_mutex_t gateway_conn_in_thread_lock = PTHREAD_MUTEX_INITIALIZER;
static void* _gateway_conn_in_thread(void* address_info){
	struct sigaction newAction;
	newAction.sa_handler = handler_do_nothing;
	sigemptyset(&newAction.sa_mask);
	newAction.sa_flags = 0;
	sigaction(SIGUSR1, &newAction, NULL);
	
	pthread_detach(pthread_self());
	char* server = ((char**)address_info)[0];
	char* port = ((char**)address_info)[1];
	free(address_info);
	
	struct addrinfo addrConfig, *addrInfo;
	memset(&addrConfig, 0, sizeof(addrConfig));
	addrConfig.ai_family = AF_UNSPEC;
	addrConfig.ai_socktype = SOCK_STREAM;
	
	int ret;
	if( (ret = getaddrinfo(server, port, &addrConfig, &addrInfo)) != 0 ){
		error("fail to get address info, %s\n", gai_strerror(ret));
		return NULL;
	}
	
	struct addrinfo* cursor;
	int sock = 0;
	debug("ready to connect to %s\n", server);
	for(cursor = addrInfo; cursor!=NULL; cursor = cursor->ai_next){
		size_t addrStrLen = (cursor->ai_family==AF_INET)?INET_ADDRSTRLEN:INET6_ADDRSTRLEN;
		char printBuffer[addrStrLen];
		//void* addr = (cursor->ai_family==AF_INET)?(((struct sockaddr_in *)cursor)->sin_addr):(((struct sockaddr_in6 *)cursor)->sin6_addr);
		void* addr;
		if(cursor->ai_family==AF_INET)
			addr = &(((struct sockaddr_in *)(cursor->ai_addr))->sin_addr);
		else
			addr = &(((struct sockaddr_in6 *)(cursor->ai_addr))->sin6_addr);
		debug("attempt to connect to %s \n",
				inet_ntop(cursor->ai_family, addr, printBuffer, addrStrLen) );
		fflush(stderr);
		
		if( (sock = socket(cursor->ai_family, cursor->ai_socktype, 0)) <0 ){
			error("connect to %s failed. %s.\n", 
					inet_ntop(cursor->ai_family, addr, printBuffer, addrStrLen), 
					strerror(errno));
			return NULL;
		}
		
		if( connect(sock, cursor->ai_addr, cursor->ai_addrlen) == -1 ){
			if(errno == EINTR) {
				warn("connctting attempt timeout\n");
				close(sock);
				freeaddrinfo(addrInfo);
				return NULL;
			}else{
				error("connect to %s failed. %s.\n", 
						inet_ntop(cursor->ai_family, addr, printBuffer, addrStrLen), 
						strerror(errno));
				close(sock);
				continue;
			}
		}
		
		info("connected to [%s]:%s\n",printBuffer, port);
		break;
	}
	
	if(cursor == NULL) {
		error("all possible IP addresses tried and failed. client is unable to "
				"connect to server. \n");
		freeaddrinfo(addrInfo);
		return NULL;
	}
	
	freeaddrinfo(addrInfo);
	
	fcntl(sock, F_SETFL, O_NONBLOCK);
	int flag = 1;
	int result = setsockopt(sock,            /* socket affected */
							 IPPROTO_TCP,     /* set option at TCP level */
							 TCP_NODELAY,     /* name of option */
							 (char *) &flag,  /* the cast is historical cruft */
							 sizeof(int));    /* length of option value */
	if(result<0){
		perror("setsockopt error");
		exit(EXIT_FAILURE);
	}
	pthread_mutex_lock(&gateway_conn_in_thread_lock);
	if(gatewaySocket<0) gatewaySocket = sock;
	else close(sock);
	pthread_mutex_unlock(&gateway_conn_in_thread_lock);
	return 0;
}
static pthread_t gateway_conn_in_thread_thread = -1;
void gateway_conn_in_thread(char* server, char* port){
	if(gateway_conn_in_thread_thread != -1)
		pthread_kill(gateway_conn_in_thread_thread, SIGUSR1);
	if(gateway_getFd() >= 0){
		warn("attempting new connection while connected\n");
		return;
	}
	char** address_info = malloc(2*sizeof(char*));
	address_info[0] = server;// TODO: server and port may not be available when accessed
	address_info[1] = port;
	clock_t start = clock();
	pthread_create(&gateway_conn_in_thread_thread, NULL, _gateway_conn_in_thread, address_info);
	debug("spawning new thread time spend: %d micro-second\n", (int)(clock()-start));
	return;
}

int gateway_getFd(){
	pthread_mutex_lock(&gateway_conn_in_thread_lock);
	int result = gatewaySocket;
	pthread_mutex_unlock(&gateway_conn_in_thread_lock);
	return result;
}

int receiveBuffer = -1;
int gateway_receive(){
	if(gatewaySocket < 0){
		return -1;
	}
	int message = getUtillNewLineNonBlock(gatewaySocket, &receiveBuffer);
	if(message >= 0){// new message arrived
		return message;
	}else if(message == -1){// connection terminated
		close(gatewaySocket);
		gatewaySocket = -1;
		return -1;
	}else if(message == -2){// partial message received
		return -2;
	}
	return -1;
}

static int sendall(int s, char *buf, int len, int more){
    int total = 0;        // how many bytes we've sent
    int bytesleft = len; // how many we have left to send
    int n;

    while(total < len) {
        n = send(s, buf+total, bytesleft, more?MSG_MORE:0);
        if (n == -1) { break; }
        total += n;
        bytesleft -= n;
    }

    len = total; // return number actually sent here

    return n==-1?-1:0; // return -1 on failure, 0 on success
} 
int gateway_send(char* content){
	if(content == NULL) return -1;
	size_t len = strlen(content);
	if( gatewaySocket >=0 && 
			sendall(gatewaySocket, content, len, 1)!=-1 && 
			sendall(gatewaySocket, "\n", 1, 0)!=-1){
		return 0;
	}else{
		return -1;
	}
}