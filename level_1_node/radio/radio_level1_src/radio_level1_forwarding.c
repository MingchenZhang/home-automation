#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include <inttypes.h>
#include <jansson.h> // printf uint64_t

#include "radio_level1_init.h"
#include "radio_level1_forwarding.h"
#include "util_array_list.h"
#include "radio_level1_rf24Radio.h"
#include "util_debug.h"
#include "radio_level1_broadcastProcess.h"
#include "radio_level1_localReceive.h"
#include "util_tools.h"
#include "radio_level1_gatewayConnector.h"

static uint64_t gatewayAddress;

void forwarding_init(uint64_t _gatewayAddress){
	gatewayAddress = _gatewayAddress;
	connectedTable = arrayList_init(5, sizeof(forwarding_entry));
}

void forwarding_addEntry(uint64_t address){
	int i=0;
	for(; i<arrayList_getSize(connectedTable); i++){
		forwarding_entry* entry = (forwarding_entry*)arrayList_get(i, connectedTable);
		if(entry->address == address) break;
	}
	
	if(i == arrayList_getSize(connectedTable)){
		forwarding_entry newEntry;
		newEntry.address = address;
		newEntry.lastSeen = time(NULL);
		arrayList_add(&newEntry, connectedTable);
	}else{
		forwarding_entry* entry = (forwarding_entry*)arrayList_get(i, connectedTable);
		entry->lastSeen = time(NULL);
	}
	debug("forwarding table new entry added: %"PRIx64"\n", address);
}

int forwarding_refreshEntry(uint64_t address){
	int i=0;
	for(; i<arrayList_getSize(connectedTable); i++){
		forwarding_entry* entry = (forwarding_entry*)arrayList_get(i, connectedTable);
		if(entry->address == address) break;
	}
	
	if(i == arrayList_getSize(connectedTable)){
		return -1;
	}else{
		forwarding_entry* entry = (forwarding_entry*)arrayList_get(i, connectedTable);
		entry->lastSeen = time(NULL);
		debug("forwarding entry %d to %"PRIx64" has been refreshed\n", i, address);
		return 0;
	}
}

static int findInTable(uint64_t address){
	int i=0;
	for(; i<arrayList_getSize(connectedTable); i++){
		forwarding_entry* entry = (forwarding_entry*)arrayList_get(i, connectedTable);
		if(entry->address == address && time(NULL)-entry->lastSeen<=210) return i;
	}
	return -1;
}
static inline void sendRF24ToGateway(void* rawPacket, size_t packetSize){
	char buffer[packetSize*2+1];
	binToHex(rawPacket, packetSize, buffer);
	json_t* sendUp = json_object();
	json_object_set_new(sendUp, "type", json_string("rf24_packet"));
	json_object_set_new(sendUp, "packet", json_string(buffer));
	char* replyStr = json_dumps(sendUp, 0);
	debug("RF24 Packet forwarding to gateway: `%s'\n", replyStr);
	gateway_send(replyStr);
	json_decref(sendUp);
}
int forwarding_forward(void* rawPacket, size_t packetSize){
	if(packetSize <= 11) return -1;
	RF24Packet* packet = rawPacket;
	uint64_t des = translateAddressToNum(packet->desAddress);
	if(des == gatewayAddress){
		// todo: send up
		debug("Packet to %"PRIx64" forwarded to gateway\n", des);
		sendRF24ToGateway(rawPacket, packetSize);
		return 0;
	}else if(des == localAddress){
		debug("Packet to %"PRIx64" received locally\n", des);
		return localReceive_rf24Packet(rawPacket, packetSize);
	}else if(des == discoveryAddress){
		debug("Discovery packet to %"PRIx64" received locally\n", des);
		broadcastProcess_discoveryAccept(rawPacket);
		return 0;
	}else{
		int entryNum = findInTable(des);
		if(entryNum >= 0) {
			rf24Radio_send(des, rawPacket, packetSize);
			debug("Packet to %"PRIx64" forwarded (entry:%d)\n", des, entryNum);
			return 0;
		}else{
			// unknown: send to gateway
			sendRF24ToGateway(rawPacket, packetSize);
			debug("Packet to %"PRIx64" forwarded to gateway (default)\n", des);
			return 0;
		}
	}
}

void translateAddressToBig(uint64_t address8, char* address5){
	int i;
	for(i=5; i>=1; i--) {address5[i-1] = ( address8 >> (8*(5-i)) ) & 0xFF;}
}

uint64_t translateAddressToNum(char* address5){
	uint64_t address8 = 0;
	int i;
	for(i=5; i>=1; i--) {address8 += ((uint64_t)address5[i-1] << (8*(5-i)));}
	return address8;
}