
#include <string.h>
#include <SPI.h>
#include <RF24.h>

#include "config.h"
#include "rf24Radio.h"
#include "localReceive.h"
#include "handler.h"

RF24 radio(9,10);

typedef struct DiscoveryPacket{
	char type;
	char scrAddress[5];
	char desAddress[5];
	char progress;
}DiscoveryPacket;

uint64_t localAddress;
uint64_t serverAddress;
uint64_t gatewayAddress = 0;
#define discoveryAddress LOCAL_BASE_ADDRESS

static void translateAddressToBig(uint64_t address8, char* address5){
	int i;
	for(i=5; i>=1; i--) {address5[i-1] = ( address8 >> (8*(5-i)) ) & 0xFF;}
}

static uint64_t translateAddressToNum(char* address5){
	uint64_t address8 = 0;
	int i;
	for(i=5; i>=1; i--) {address8 += ((uint64_t)(address5[i-1] & 0xFF) << (8*(5-i)));}
	return address8;
}

static int rf24Radio_receiveNonCheck(void* buffer, size_t bufferSize){
	size_t payloadSize = radio.getDynamicPayloadSize();
	radio.read(buffer, bufferSize);
	return payloadSize;
}

static void printHex(char num) {
	char format[5];
	sprintf(format, "%02hhX", num & 0xFF);
	p(format);
}

void printPacket(void* packet, size_t size){
	char* cursor = (char*)packet;
	p("packet:");
	size_t i;
	for(i=0; i<size; i++) {
		p(' ');
		printHex(*(cursor+i));
	}
	p('\n');
}

void rf24Radio_init(int localAddrOffset, int serverAddrOffset){
	localAddress = LOCAL_BASE_ADDRESS + localAddrOffset;
	serverAddress = LOCAL_BASE_ADDRESS + serverAddrOffset;
	rf24Radio_reinit();
}

void rf24Radio_reinit(){
	radio.begin();
	radio.setChannel(126);
	radio.setPALevel(RF24_PA_HIGH);
	radio.setDataRate(RF24_250KBPS);
	radio.enableDynamicPayloads();
	radio.openReadingPipe(1, localAddress);
	radio.setAutoAck(0, true);
	radio.setAutoAck(1, true);
	radio.setRetries(10,10);
	radio.startListening();
}

void rf24Radio_enterSleepMode(){
	radio.powerDown();
}

void rf24Radio_leaveSleepMode(){
	radio.powerUp();
}

int rf24Radio_conn(){// todo: optimize, auto select prefered radio
	int i;
	uint64_t _gatewayAddress = 0;
	DiscoveryPacket packet = {5,{0},{0},1};
	DiscoveryPacket reply;
	translateAddressToBig(localAddress, packet.scrAddress);
	translateAddressToBig(discoveryAddress, packet.desAddress);
	radio.setAutoAck(0, false);
	radio.setAutoAck(1, false);
	
	analogWrite(CONNECTION_INDICATOR, 255);
	delay(100);
	
	analogWrite(CONNECTION_INDICATOR, 2); // dim the CONNECTION_INDICATOR
	packet.progress = 1;
	for(i=0; i<20; i++){
		rf24Radio_sendTo(&packet, sizeof(packet), discoveryAddress);
		delay(200);
		if(radio.available()){
			int size = rf24Radio_receiveNonCheck(&reply, sizeof(reply));
			debug("progress 1 received");
			if(size == 12 && reply.progress == 2) {
				debug(" valid\n");
				_gatewayAddress = translateAddressToNum(reply.scrAddress);
				//debug("get gateway address: ");
				//DEBUG_S(printPacket(reply.scrAddress, 5));
				break;// todo: accept multiple addresses
			}else debug(" invalid\n");
		}
		if(i==19) goto conn_failed;
	}
	delay(500);
	
	analogWrite(CONNECTION_INDICATOR, 4);
	packet.progress = 3;
	translateAddressToBig(_gatewayAddress, packet.desAddress);
	for(i=0; i<20; i++){
		analogWrite(CONNECTION_INDICATOR, 100);
		rf24Radio_sendTo(&packet, sizeof(packet), _gatewayAddress);
		delay(10);
		analogWrite(CONNECTION_INDICATOR, 4);
		delay(90);
	}
	
	analogWrite(CONNECTION_INDICATOR, 6);
	packet.progress = 4;
	translateAddressToBig(_gatewayAddress, packet.desAddress);
	for(i=0; i<20; i++){
		rf24Radio_sendTo(&packet, sizeof(packet), _gatewayAddress);
		delay(200);
		if(radio.available()){
			int size = rf24Radio_receiveNonCheck(&reply, sizeof(reply));
			if(size == 12 && reply.progress == 5) {
				break;
			}
		}
		if(i==19) goto conn_failed;
	}
	delay(500);
	
	analogWrite(CONNECTION_INDICATOR, 8);
	packet.progress = 6;
	for(i=0; i<20; i++){
		rf24Radio_sendTo(&packet, sizeof(packet), discoveryAddress);
		delay(200);
		if(radio.available()){
			int size = rf24Radio_receiveNonCheck(&reply, sizeof(reply));
			if(size == 12 && reply.progress == 7) {
				break;
			}
		}
		if(i==19) goto conn_failed;
	}
	delay(500);
	
	gatewayAddress = _gatewayAddress;
	analogWrite(CONNECTION_INDICATOR, 10);
	radio.setAutoAck(0, true);
	radio.setAutoAck(1, true);
	return 0;
	
	conn_failed:
	analogWrite(CONNECTION_INDICATOR, 0);
	radio.setAutoAck(0, true);
	radio.setAutoAck(1, true);
	return -1;
}

int rf24Radio_fastSend(const void* rawContent, size_t contentSize, int type){
	if(gatewayAddress == 0 || contentSize>21) return -1;
	RF24Packet packet;
	packet.type = type;
	translateAddressToBig(localAddress, packet.scrAddress);
	translateAddressToBig(serverAddress, packet.desAddress);
	memmove(packet.content, rawContent, contentSize);
	return rf24Radio_send(&packet, contentSize+11)?0:-1;
}

bool rf24Radio_send(const void* rawPacket, size_t packetSize){
	if(gatewayAddress == 0) return false;
	return rf24Radio_sendTo(rawPacket, packetSize, gatewayAddress);
}

int rf24Radio_send_smart(const void* rawPacket, size_t packetSize){
	if(packetSize < 11) return -1;
	uint64_t dest = translateAddressToNum(((RF24Packet*)rawPacket)->desAddress);
	printPacket(&dest, 8);
	if(rf24Radio_sendTo(rawPacket, packetSize, dest)) return 0;
	else return -1;
}

bool rf24Radio_sendTo(const void* rawPacket, size_t packetSize, uint64_t dest){
	radio.stopListening();
	radio.openWritingPipe(dest);
	bool result = radio.write(rawPacket, packetSize);
	/* if(!result) {
		rf24Radio_reinit();
		result = radio.write(rawPacket, packetSize);
		while(!result){
			// dead loop
			analogWrite(CONNECTION_INDICATOR, 0);
			delay(500);
			analogWrite(CONNECTION_INDICATOR, 100);
			delay(500);
		}
	} */
	debug("packet send: ");
	DEBUG_S(printPacket(rawPacket, packetSize));
	radio.startListening();
	return result;
}

char lastEventId_set = 0;
char lastEventId[4];
RF24Packet lastAck;
size_t lastAck_size;
int rf24Radio_receive(void* buffer, size_t bufferSize){
	if(!radio.available()) {return -1;}
	size_t payloadSize = radio.getDynamicPayloadSize();
	radio.read(buffer, bufferSize);
	if(((char*)buffer)[0] > 0 && ((char*)buffer)[0] <= 4 && lastEventId_set && memcmp(((RF24Packet*)buffer)->content, lastEventId, 4) == 0){
		rf24Radio_send(&lastAck, lastAck_size);
		return -1;
	}
	debug("packet received: ");
	DEBUG_S(printPacket(buffer, payloadSize));
	return payloadSize;
}

int rf24Radio_setLatestPacket(char* event_id, RF24Packet* ack_packet, size_t size){
	lastEventId_set = 1;
	memcpy(lastEventId, event_id, 4);
	memcpy(&lastAck, ack_packet, size);
	lastAck_size = size;
}

int rf24Radio_isConn(){
	return gatewayAddress != 0;
}

void rf24Radio_connectionCheck(){
	pl("checking");
	if(gatewayAddress == 0){
		int i;
		pl("reconnecting");
		for(i=0;i<1 && rf24Radio_conn()<0; i++);
	}else{
		pl("sending pulse");
		RF24Packet pulse;
		RF24Packet reply;
		int replySize = -1;
		pulse.type = 6;
		translateAddressToBig(localAddress, pulse.scrAddress);
		translateAddressToBig(gatewayAddress, pulse.desAddress);
		pulse.content[0] = 0;
		int i;
		for(i=0; i<500; i++){
			if(i%100 == 0) rf24Radio_send(&pulse, 12);
			delayMicroseconds(300);
			replySize = rf24Radio_receive(&reply, 12);
			if(replySize < 1) continue;
			if(replySize == 12 && reply.type == 6) break;
			else localReceive_process((RF24Packet*)&reply, replySize);
		}
		if(i < 500){
			pl("1connection confirmed");
			// connection still exist
			#ifdef POWER_SAVING_MODE
			enable_sleep();
			#endif
			return;
		}else{
			pl("2connection lost");
			gatewayAddress = 0;
			analogWrite(CONNECTION_INDICATOR, 0);
			#ifdef POWER_SAVING_MODE
			disable_sleep();
			#endif
			return;
		}
	}
}