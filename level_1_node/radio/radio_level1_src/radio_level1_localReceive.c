
#include <inttypes.h>
#include <jansson.h>
#include <string.h>

#include "radio_level1_rf24Radio.h"
#include "radio_level1_forwarding.h"
#include "radio_level1_localReceive.h"
#include "radio_level1_gatewayConnector.h"
#include "util_array_list.h"
#include "util_debug.h"
#include "util_address_referer.h"
#include "util_tools.h"

typedef struct ConnInfo{
	uint64_t address;
	time_t timer;
	int progress; // last status received
	int confirmationRec;
}ConnInfo;

int connectingTable;

void localReceive_init(){
	connectingTable = arrayList_init(5, sizeof(ConnInfo));
}

static ConnInfo* getConn(uint64_t address){
	int i;
	for(i=0; i<arrayList_getSize(connectingTable); i++){
		ConnInfo* info = arrayList_get(i, connectingTable);
		if(info->address == address){
			return info;
		}
	}
	return NULL;
}
static void removeConn(uint64_t address){
	int i;
	for(i=0; i<arrayList_getSize(connectingTable); i++){
		ConnInfo* info = arrayList_get(i, connectingTable);
		if(info->address == address){
			arrayList_remove(i, connectingTable);
		}
	}
}
static void clearOutDatedConn(){
	int i;
	for(i=0; i<arrayList_getSize(connectingTable); i++){
		ConnInfo* info = arrayList_get(i, connectingTable);
		if(info->timer-time(NULL)>60){
			arrayList_remove(i, connectingTable);
			i--;
		}
	}
}

void localReceive_addNewConn(uint64_t scrAddr){
	ConnInfo info;
	info.address = scrAddr;
	info.progress = 1;
	info.timer = time(NULL);
	info.confirmationRec = 0;
	arrayList_add(&info, connectingTable);
	RF24Packet reply;
	reply.type = 5;
	translateAddressToBig(localAddress, reply.scrAddress);
	translateAddressToBig(info.address, reply.desAddress);
	reply.content[0] = 2; // respond to connection request
	rf24Radio_send_wo_aa(info.address, &reply, 12);
	debug("new connection to %"PRIx64" initiating\n", info.address);
}

int localReceive_rf24Packet(void* rawPacket, size_t packetSize){
	if(packetSize < 12) return -1;
	RF24Packet* recPacket = rawPacket;
	switch(recPacket->type){
	case 5:{
		clearOutDatedConn();
		DiscoveryPacket* disPacket = rawPacket;
		uint64_t scrAddr = translateAddressToNum(disPacket->scrAddress);
		switch(disPacket->progress){
		case 1:
			localReceive_addNewConn(scrAddr);
			break;
		case 3:
			{
				ConnInfo* existProg = getConn(scrAddr);
				if(existProg==NULL || existProg->progress!=1) break;
				existProg->confirmationRec++;
			}
			break;
		case 4:
			{
				ConnInfo* existProg = getConn(scrAddr);
				if(existProg==NULL) break;
				if(existProg->confirmationRec >= 16){
					existProg->progress = 4;
					RF24Packet reply;
					reply.type = 5;
					translateAddressToBig(localAddress, reply.scrAddress);
					translateAddressToBig(existProg->address, reply.desAddress);
					reply.content[0] = 5;
					rf24Radio_send_wo_aa(existProg->address, &reply, 12);
				}else{
					info("node %"PRIx64" does not achieve target signal quality."
							" dropping connection request. \n", existProg->address);
				}
			}
			break;
		case 6:
			{
				ConnInfo* existProg = getConn(scrAddr);
				if(existProg==NULL || existProg->progress!=4) break;
				RF24Packet reply;
				reply.type = 5;
				translateAddressToBig(localAddress, reply.scrAddress);
				translateAddressToBig(existProg->address, reply.desAddress);
				reply.content[0] = 7;
				rf24Radio_send_wo_aa(existProg->address, &reply, 12);
				forwarding_addEntry(existProg->address);// todo: what if progress 7 gets lost
				removeConn(existProg->address); // remove this address out of connecting stage, into connected
			}
			break;
		default:
			break;
		}
		break;
	}
	case 6:{
		uint64_t remoteAddress = translateAddressToNum(recPacket->scrAddress);
		if(forwarding_refreshEntry(remoteAddress) == 0 ){
			RF24Packet reply;
			reply.type = 6;
			translateAddressToBig(localAddress, reply.scrAddress);
			translateAddressToBig(remoteAddress, reply.desAddress);
			reply.content[0] = 0;
			rf24Radio_send(remoteAddress, &reply, 12);
		}
		// todo: bidirectional last pulse support
		return 0;
	}
	default:
		break;
	}
	return -1;
}

int localReceive_networkPacket(int message){
	char* messageStr = referer_getAddr(message);
	debug("new message from gateway: `%s'\n", messageStr);
	json_t* root = json_loads(messageStr, 0, NULL);
	const char* type = json_string_value(json_object_get(root, "type"));
	if(strcmp(type, "device_list_get") == 0){
		json_t* devList = json_array();
		int i;
		for(i=0; i<arrayList_getSize(connectedTable); i++){
			forwarding_entry* info = arrayList_get(i, connectedTable);
			if(time(NULL) - info->lastSeen > CONNECTED_TABLE_EXP_TIME) continue;
			json_t* device = json_object();
			json_object_set_new(device, "address", json_integer((json_int_t)info->address));
			json_array_append_new(devList, device);
		}
		json_t* reply = json_object();
		json_object_set_new(reply, "device_list", devList);
		json_object_set_new(reply, "type", json_string("device_list"));
		char* replyStr = json_dumps(reply, 0);
		if(replyStr == NULL) {json_decref(reply); goto failure;}
		gateway_send(replyStr);
		json_decref(reply);
		goto success;
	}else if(strcmp(type, "rf24_packet") == 0){
		const char* packet = json_string_value(json_object_get(root, "packet"));
//		debug("packet str: %s\n", packet);
		uint8_t packetRaw[(strlen(packet)+1)/2];
		hexToBin(packet, strlen(packet), packetRaw);
//		printMemory(packetRaw, (strlen(packet)+1)/2);
//		printf("\n");
		forwarding_forward(packetRaw, (strlen(packet)+1)/2);
		goto success;
	}else{
		warn("gateway message type `%s' is known, discarded. \n", type);
		goto failure;
	}
	
	failure:
	json_decref(root);
	return -1;
	
	success:
	json_decref(root);
	return 0;
}

void printConnectingTable(){
	if(connectingTable < 0) return;
	info("following address(es) is(are) in the table\n");
	int i;
	for(i=0; i<arrayList_getSize(connectingTable); i++){
		ConnInfo* info = arrayList_get(i, connectingTable);
		printf("%"PRIx64"\n", info->address);
	}
}