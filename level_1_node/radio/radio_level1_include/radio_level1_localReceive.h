/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   radio_level1_localReceive.h
 * Author: mingchen
 *
 * Created on June 21, 2016, 2:19 PM
 */

#ifndef RADIO_LEVEL1_LOCALRECEIVE_H
#define RADIO_LEVEL1_LOCALRECEIVE_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct DiscoveryPacket{
	char type;
	char scrAddress[5];
	char desAddress[5];
	char progress;
	char space[20];
}DiscoveryPacket;

void localReceive_init();

void localReceive_addNewConn(uint64_t scrAddr);

int localReceive_rf24Packet(void* rawPakcet, size_t packetSize);

int localReceive_networkPacket(int message);

void printConnectingTable();

#ifdef __cplusplus
}
#endif

#endif /* RADIO_LEVEL1_LOCALRECEIVE_H */

