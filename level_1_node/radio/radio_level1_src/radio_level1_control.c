/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "util_debug.h"
#include "radio_level1_rf24Radio.h"
#include "radio_level1_forwarding.h"
#include "radio_level1_gatewayConnector.h"
#include "util_array_list.h"
#include "radio_level1_init.h"
#include "util_tools.h"
#include "radio_level1_localReceive.h"
#include "util_address_referer.h"

int control_loop(int no_stdin){
	info("program initialized\n");
	fd_set watching;
	time_t lastConnToGateway = 0;
	while(1){
		FD_ZERO(&watching);
		int gatewaySocket = gateway_getFd();
		if(gatewaySocket>=0) FD_SET(gatewaySocket, &watching);
		else{
			if(time(NULL) - lastConnToGateway > 20){// if disconnected, reconnect every 20 second
				lastConnToGateway = time(NULL);
				gateway_conn_in_thread(gatewayHostname, gatewayPort);
			}
		}
		int maxFd;
		if(!no_stdin){
			FD_SET(STDIN_FILENO, &watching);
			maxFd = (STDIN_FILENO>gatewaySocket)?STDIN_FILENO:gatewaySocket;
		}else{
			maxFd = (0>gatewaySocket)?0:gatewaySocket;
		}
//		struct timeval tv = {0, 20};
//		int result = select(maxFd+1, &watching, NULL, NULL, &tv);
		struct timespec ts = {0, 500000};
		int result = pselect(maxFd+1, &watching, NULL, NULL, &ts, NULL);
		if(result == -1){
			perror("pselect() error\n");
			break;
		}
//		else if(result != 0){
//			debug("pselect unblocked\n");
//		}
		
		if(!no_stdin && FD_ISSET(STDIN_FILENO, &watching)){
			// keyboard input
			int input = getUtillNewLine(STDIN_FILENO);
			char* input_str = referer_getAddr(input);
			if(strcmp(input_str,"/p-forward-table") == 0){
				printConnectingTable();
			}else{
				error("keyboard input unrecognizable\n");
			}
			arrayList_free(input);
		}
		if(FD_ISSET(gatewaySocket, &watching)){
			// new message from gateway
			int message = gateway_receive();
			if(message >= 0){// new line received, send to process
				localReceive_networkPacket(message);
			}else if(message == -1){// disconnected
				info("Gateway disconnected\n");
			}else if(message == -2){// partial received, continue
				
			}
			arrayList_free(message);
		}
		while(rf24Radio_hasIncoming()){
			debug("radio received\n");
			// RF24 radio received
			RF24Packet newPacket;
			int packetSize = rf24Radio_receive(&newPacket, sizeof(RF24Packet));
			forwarding_forward(&newPacket, packetSize);
		}
	}
	return -1;
}