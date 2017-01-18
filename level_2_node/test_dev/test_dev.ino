#include <Timer.h>
#include <SPI.h>
#include <RF24.h>

#include "rf24Radio.h"
#include "localReceive.h"
#include "config.h"

Timer t;
void* receiveBuffer;

void setup(){
	Serial.begin(57600);
	pinMode(CONNECTION_INDICATOR, OUTPUT);
	pinMode(2, OUTPUT);
	digitalWrite(2, LOW);
	rf24Radio_init(20, 1);
	//rf24Radio_conn();
	receiveBuffer = malloc(32);
	
	// check new message from network
	t.every(MESSAGE_CHECK_INTERVAL, radioReceiveCheck);
	
	// check node internal status
	t.every(STATUS_CHECK_INTERVAL, localReceive_statusCheck);
	
	// check network connectivity
	rf24Radio_connectionCheck();
	t.every(LIVE_PULSE_INTERVAL, rf24Radio_connectionCheck);
	
	localReceive_init();
}

void loop(){
	t.update();
}

void radioReceiveCheck(){
	int result = rf24Radio_receive(receiveBuffer, 32);
	if(result>0 && rf24Radio_isConn()){
		localReceive_process((RF24Packet*)receiveBuffer, result);
	}else{
		return;
	}
}
