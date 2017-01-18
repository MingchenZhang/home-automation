#include <string.h>
#include <WProgram.h>
#include <Arduino.h>

#include "localReceive.h"
#include "rf24Radio.h"
#include "config.h"

int light_status = 0;

void localReceive_init(){
	pinMode(5, OUTPUT);
	digitalWrite(5, LOW);
}

void localReceive_process(RF24Packet* newPacket, size_t packetSize){
	int old_status = light_status;
	if(packetSize < 17) return;
	if(newPacket->type == 1 && newPacket->content[4] == 1){
		if(newPacket->content[5] == 1){
			digitalWrite(5, HIGH);
			light_status = 1;
			RF24Packet reply;
			reply.type = 2;
			memmove(reply.desAddress, newPacket->scrAddress, 5);
			memmove(reply.scrAddress, newPacket->desAddress, 5);
			memmove(reply.content, newPacket->content, 4);
			if(old_status == light_status)reply.content[4] = 2;
			else reply.content[4] = 0;
			rf24Radio_send(&reply, 16);
		}else if(newPacket->content[5] == 2){
			digitalWrite(5, LOW);
			light_status = 0;
			RF24Packet reply;
			reply.type = 2;
			memmove(reply.desAddress, newPacket->scrAddress, 5);
			memmove(reply.scrAddress, newPacket->desAddress, 5);
			memmove(reply.content, newPacket->content, 4);
			if(old_status == light_status)reply.content[4] = 2;
			else reply.content[4] = 0;
			rf24Radio_send(&reply, 16);
		}
	}
}

void localReceive_statusCheck(){
	
}
