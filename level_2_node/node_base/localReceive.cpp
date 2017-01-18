#include <string.h>
#include <Arduino.h>

#include "config.h"
#include "localReceive.h"
#include "rf24Radio.h"
#include "tools.h"



char switch_1_status = 0;
char switch_2_status = 0;

void localReceive_init(){
	
}

void localReceive_process(RF24Packet* newPacket, size_t packetSize){
	
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