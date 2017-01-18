/* 
 * File:   radio_level1_gatewayConnector.h
 * Author: mingchen
 *
 * Created on June 26, 2016, 11:31 PM
 */

#ifndef RADIO_LEVEL1_GATEWAYCONNECTOR_H
#define RADIO_LEVEL1_GATEWAYCONNECTOR_H

#ifdef __cplusplus
extern "C" {
#endif

int gateway_conn(char* server, char* port);
void gateway_conn_in_thread(char* server, char* port);
int gateway_getFd();

// return positive represent a valid result
// -1 means unable to read from this fd any more (may disconnected)
// -2 means partial info received, not enough to generate a valid result
int gateway_receive();

int gateway_send(char* content);

#ifdef __cplusplus
}
#endif

#endif /* RADIO_LEVEL1_GATEWAYCONNECTOR_H */

