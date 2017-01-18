#ifndef LOCALRECEIVE_H
#define LOCALRECEIVE_H

#include "rf24Radio.h"
#include <string.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void localReceive_init();

void localReceive_process(RF24Packet* newPacket, size_t packetSize);

void localReceive_statusCheck();

#ifdef POWER_SAVING_MODE
void localReceive_justAwake();
#endif

#ifdef __cplusplus
}
#endif

#endif /* LOCALRECEIVE_H */

