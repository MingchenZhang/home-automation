#ifndef RF24RADIO_H
#define RF24RADIO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdint.h>

uint64_t localAddress;
uint64_t discoveryAddress;

struct RF24Packet{
	char type;
	char scrAddress[5];
	char desAddress[5];
	char content[21];
}__attribute__((packed));
typedef struct RF24Packet RF24Packet;

int rf24Radio_init(uint64_t _localAddress, uint64_t _discoveryAddress, int channel);

void rf24Radio_changeLocalAddress(uint64_t address);

int rf24Radio_send(uint64_t targetAddress, void* rawContent, size_t sizeToSend);

int rf24Radio_send_wo_aa(uint64_t targetAddress, void* rawContent, size_t sizeToSend);

int rf24Radio_hasIncoming();

int rf24Radio_receive(void* buffer, size_t size);

#ifdef __cplusplus
}
#endif

#endif /* RF24RADIO_H */