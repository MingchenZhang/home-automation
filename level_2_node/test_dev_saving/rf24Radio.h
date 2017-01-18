#ifndef RF24RADIO_H
#define RF24RADIO_H

#include <string.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define LOCAL_BASE_ADDRESS 0xaaaaaaaa00LL

typedef struct RF24Packet{
	char type;
	char scrAddress[5];
	char desAddress[5];
	char content[21];
}RF24Packet;

void printPacket(void* packet, size_t size);

void rf24Radio_init(int localAddrOffset, int serverAddrOffset);

void rf24Radio_reinit();

int rf24Radio_conn();

int rf24Radio_fastSend(const void* rawContent, size_t contentSize, int type);

bool rf24Radio_send(const void* rawPacket, size_t packetSize);

int rf24Radio_send_smart(const void* rawPacket, size_t packetSize);

bool rf24Radio_sendTo(const void* rawPacket, size_t packetSize, uint64_t dest);

int rf24Radio_receive(void* buffer, size_t bufferSize);

int rf24Radio_setLatestPacket(char* event_id, RF24Packet* ack_packet, size_t size);
	
int rf24Radio_isConn();

void rf24Radio_connectionCheck();

#ifdef POWER_SAVING_MODE
void rf24Radio_enterSleepMode();
void rf24Radio_leaveSleepMode();
#endif 

#ifdef __cplusplus
}
#endif

#endif /* RF24RADIO_H */

