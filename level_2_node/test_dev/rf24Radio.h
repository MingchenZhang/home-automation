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

int rf24Radio_conn();

bool rf24Radio_send(void* rawPacket, size_t packetSize);

int rf24Radio_send_smart(void* rawPacket, size_t packetSize);

bool rf24Radio_sendTo(void* rawPacket, size_t packetSize, uint64_t dest);

int rf24Radio_receive(void* buffer, size_t bufferSize);
	
int rf24Radio_isConn();

void rf24Radio_connectionCheck();

#ifdef __cplusplus
}
#endif

#endif /* RF24RADIO_H */

