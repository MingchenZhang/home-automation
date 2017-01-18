/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   radio_level1_broadcastProcess.h
 * Author: mingchen
 *
 * Created on June 20, 2016, 7:00 PM
 */

#ifndef RADIO_LEVEL1_BROADCASTPROCESS_H
#define RADIO_LEVEL1_BROADCASTPROCESS_H

#ifdef __cplusplus
extern "C" {
#endif

void broadcastProcess_init();
void broadcastProcess_discoveryAccept(void* rawPacket);

#ifdef __cplusplus
}
#endif

#endif /* RADIO_LEVEL1_BROADCASTPROCESS_H */

