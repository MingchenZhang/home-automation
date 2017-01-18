/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include <time.h>
#include <inttypes.h>

#include "util_array_list.h"
#include "radio_level1_forwarding.h"
#include "radio_level1_rf24Radio.h"
#include "radio_level1_localReceive.h"
#include "util_debug.h"

void broadcastProcess_init(){
	
}

void broadcastProcess_discoveryAccept(void* rawPacket){
	RF24Packet* packet = rawPacket;
	switch(packet->type){
	case 5:{
		DiscoveryPacket* disPacket = rawPacket;
		uint64_t scrAddr = translateAddressToNum(disPacket->scrAddress);
		switch(disPacket->progress){
		case 1:
			localReceive_addNewConn(scrAddr);
			break;
		default:
			break;
		}
		break;
	}
	default:
		break;
	}
	
}