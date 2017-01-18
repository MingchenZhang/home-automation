#include <Arduino.h>
#include "config.h"
#include "tools.h"
#include "rf24Radio.h"
#include "localReceive.h"
#include "handler.h"

Timer* loop_timer;
Timer t;
void* receiveBuffer;
int connection_check_timer = -1;

void program_init(){
	Serial.begin(57600);
	pl("initializing");
	pinMode(CONNECTION_INDICATOR, OUTPUT);
	pinMode(ERROR_INDICATOR, OUTPUT);
	loop_timer = &t;
	rf24Radio_init(DEVICE_NO, 1);
	receiveBuffer = malloc(32);
	#ifdef POWER_SAVING_MODE
	sleep_disabled = 0;
	#endif
	
	// check new message from network
	loop_timer->every(MESSAGE_CHECK_INTERVAL, radioReceiveCheck);
	
	// check node internal status
	#if STATUS_CHECK_INTERVAL > 0
	loop_timer->every(STATUS_CHECK_INTERVAL, localReceive_statusCheck);
	#endif
	
	// check network connectivity
	rf24Radio_connectionCheck();
	connection_check_timer = loop_timer->every(LIVE_PULSE_INTERVAL, rf24Radio_connectionCheck);
	
	localReceive_init();
}

void program_loop(){
	loop_timer->update();
}

void radioReceiveCheck(){
	int result = rf24Radio_receive(receiveBuffer, 32);
	if(result>0 && rf24Radio_isConn()){
		pl("packet proc");
		localReceive_process((RF24Packet*)receiveBuffer, result);
	}else{
		return;
	}
}

#ifdef POWER_SAVING_MODE
int power_saving_interval_timer = -1;
int sleep_disabled = 0;
char conn_check_in_sleep_counter = 0;

void sleep_handler(){
	if(sleep_disabled){return;}
	
	if(++conn_check_in_sleep_counter >= POWER_SAVING_LIVE_PULSE_INTERVAL){
		rf24Radio_connectionCheck();
		conn_check_in_sleep_counter = 0;
	}
	if(!rf24Radio_isConn()) return;
	
	analogWrite(CONNECTION_INDICATOR, 0);
	
	// sleep now
	rf24Radio_enterSleepMode();
	sleep_2_seconds();
	rf24Radio_leaveSleepMode();
	
	analogWrite(CONNECTION_INDICATOR, 10);
	
	// subscribe for next sleep
	localReceive_justAwake();
	power_saving_interval_timer = loop_timer->after(POWER_SAVING_LIVE_TIME, sleep_handler);
}

void disable_sleep(){
	if(power_saving_interval_timer != -1){
		loop_timer->stop(power_saving_interval_timer);
		power_saving_interval_timer = -1;
	}
	if(connection_check_timer == -1){
		connection_check_timer = loop_timer->every(LIVE_PULSE_INTERVAL, rf24Radio_connectionCheck);
	}
}

void enable_sleep(){
	if(power_saving_interval_timer == -1){
		power_saving_interval_timer = loop_timer->after(POWER_SAVING_LIVE_TIME, sleep_handler);
	}
	if(connection_check_timer != -1){
		loop_timer->stop(connection_check_timer);
		connection_check_timer = -1;
	}
	conn_check_in_sleep_counter = 0;
}
#endif