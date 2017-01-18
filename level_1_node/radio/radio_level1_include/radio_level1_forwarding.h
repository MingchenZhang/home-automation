
#ifndef FORWARDING_H
#define FORWARDING_H

#ifdef __cplusplus
extern "C" {
#endif

#include <time.h>
#include <stdint.h>

typedef struct forwarding_entry{
	time_t lastSeen;
	uint64_t address;
}forwarding_entry;

int connectedTable;
#define CONNECTED_TABLE_EXP_TIME 30

void forwarding_init(uint64_t _gatewayAddress);
void forwarding_addEntry(uint64_t address);
int forwarding_refreshEntry(uint64_t address);
int forwarding_forward(void* rawPacket, size_t packetSize);
	
// translate number to big endian, write to address5
void translateAddressToBig(uint64_t address8, char* address5);

// translate big endian to number, return the result
uint64_t translateAddressToNum(char* address5);

#ifdef __cplusplus
}
#endif

#endif /* FORWARDING_H */