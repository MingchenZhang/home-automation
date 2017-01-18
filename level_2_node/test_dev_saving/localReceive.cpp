#include <string.h>
#include <Arduino.h>

#include "config.h"
#include "localReceive.h"
#include "rf24Radio.h"
#include "tools.h"

#define SWITCH_1_PIN 2
#define SWITCH_2_PIN 4

char switch_1_status = 0;
char switch_2_status = 0;

void localReceive_init(){
	pinMode(SWITCH_1_PIN, INPUT);
	pinMode(SWITCH_2_PIN, INPUT);
}

void localReceive_process(RF24Packet* newPacket, size_t packetSize){
	if(packetSize < 17) return;
	if(newPacket->type == 1){// command packet
		char* new_status;
		char old_status;
		if(newPacket->content[4] == 1){// target switch 1
			old_status = switch_1_status;
			new_status = &switch_1_status;
			if(newPacket->content[5] == 1){// turn on
				pinMode(SWITCH_1_PIN, OUTPUT);
				switch_1_status = 1;
			}else if(newPacket->content[5] == 2){// turn off
				pinMode(SWITCH_1_PIN, INPUT);
				switch_1_status = 0;
			}else return;
		}else if(newPacket->content[4] == 2){// target switch 2
			old_status = switch_2_status;
			new_status = &switch_2_status;
			if(newPacket->content[5] == 1){// turn on
				pinMode(SWITCH_2_PIN, OUTPUT);
				switch_2_status = 1;
			}else if(newPacket->content[5] == 2){// turn off
				pinMode(SWITCH_2_PIN, INPUT);
				switch_2_status = 0;
			}else return;
		}else return;
		RF24Packet reply;
		reply.type = 2;
		memmove(reply.desAddress, newPacket->scrAddress, 5);
		memmove(reply.scrAddress, newPacket->desAddress, 5);
		memmove(reply.content, newPacket->content, 4);
		if(old_status == *new_status)reply.content[4] = 2;
		else reply.content[4] = 0;
		rf24Radio_send(&reply, 16);
	}
}

void localReceive_statusCheck(){
	
}

// This function will be called upon awaking from sleeping mode if sleeping mode is defined active.
// This unit should send some packet to level 0 node to notify it that this unit is active now, 
// so that level 0 node could send instructions in a short active period. 
// Such a packet should not be send anywhere else, since it could be send right before the unit is 
// entering sleeping mode. In such case the respond command will arrive in sleeping period and get lost. 
#ifdef POWER_SAVING_MODE
static const unsigned char ready_content[7] = {0,0,0,0,0,0,255};
void localReceive_justAwake(){
	unsigned char reportContent[7] = {0,0,0,0,1,1,2};
	reportContent[6] = switch_1_status;
	rf24Radio_fastSend(reportContent, 7, 4);
	reportContent[4] = 2;
	reportContent[6] = switch_2_status;
	rf24Radio_fastSend(reportContent, 7, 4);
	rf24Radio_fastSend(ready_content, 7, 4);
}
#endif

//------------------ predefined helper method ----------------------

/*

// send packet to gateway
bool rf24Radio_send(void* rawPacket, size_t packetSize);

// send to address specified in packet
int rf24Radio_send_smart(void* rawPacket, size_t packetSize);

// send to a specific address (dest)
bool rf24Radio_sendTo(void* rawPacket, size_t packetSize, uint64_t dest);

// check if this device has connected to a level 1 device (a gateway)
int rf24Radio_isConn();

*/