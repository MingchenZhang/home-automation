#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "RF24/RF24.h"
#include "util_debug.h"
#include "util_tools.h"

#define RADIO_SPEED RF24_250KBPS
//#define RADIO_DETAIL_DISPLAY //could be commented out

#include "radio_level1_rf24Radio.h"

//#define MULTITHREAD

#ifdef MULTITHREAD
pthread_mutex_t radioEngaging;
#define LOCK() pthread_mutex_lock(&radioEngaging)
#define UNLOCK() pthread_mutex_unlock(&radioEngaging)
#else
#define LOCK() 
#define UNLOCK() 
#endif

RF24 *radio;
int _channel;

static int rf24Radio_initiateRadio(){
	//Prepare the radio module
	LOCK();
	info("\nPreparing radio interface\n");
	if(!radio->begin()){
		UNLOCK();
		return -1;
	}
	radio->failureDetected = 0;
	radio->powerUp();
	radio->setChannel(_channel);
	radio->setPALevel(RF24_PA_MAX);
	radio->setDataRate(RADIO_SPEED);
	radio->enableDynamicPayloads();
	radio->openWritingPipe(localAddress+1);
	radio->openReadingPipe(1,localAddress);
	radio->openReadingPipe(2,discoveryAddress);
	radio->setAutoAck(0, true);
	radio->setAutoAck(1, true);
	radio->setAutoAck(2, false);
	radio->setRetries(10,10);
	radio->startListening();
	#ifdef RADIO_DETAIL_DISPLAY
	radio->printDetails();
	fflush(stdout);
	#endif
	success("\nRadio reset OK.\n");
	UNLOCK();
	
	if(radio->failureDetected){
		return -1;
	}else{
		return 0;
	}
}

int rf24Radio_init(uint64_t _localAddress, uint64_t _discoveryAddress, int channel){
	radio = new RF24(RPI_V2_GPIO_P1_22, RPI_V2_GPIO_P1_24, BCM2835_SPI_SPEED_4MHZ);  //spi device, speed and CSN,only CSN is NEEDED in RPI
	#ifdef MULTITHREAD
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&radioEngaging, &attr);
	#endif
	localAddress = _localAddress;
	discoveryAddress = _discoveryAddress;
	_channel = channel;
	if(rf24Radio_initiateRadio()!= 0) return -1;
	else return 0;
}

void rf24Radio_changeLocalAddress(uint64_t address){
	LOCK();
	radio->powerUp();
	radio->stopListening();
	radio->openReadingPipe(1,address);
	localAddress = address;
	radio->startListening();
	UNLOCK();
}

int rf24Radio_send(uint64_t targetAddress, void* rawContent, size_t sizeToSend){
	#ifdef RADIO_DETAIL_DISPLAY
	fprintf(stdout, "radio sending package: 0x");
	for(size_t i=0; i<sizeToSend; i++){
		fprintf(stdout, "%02x", ((uint8_t*)rawContent)[i]);
	}
	fprintf(stdout, ", %ld, ", get_nanosecond());
	fflush(stdout);
	#endif
	LOCK();
	resend:
	radio->stopListening();
	radio->openWritingPipe(targetAddress);
	bool result = radio->write(rawContent, sizeToSend);
	#ifdef RADIO_DETAIL_DISPLAY
	if(result) fprintf(stdout, "succeed\n");
	else fprintf(stdout, "failed\n");
	fflush(stdout);
	#endif
	radio->startListening();
	
	if(radio->failureDetected){
		warn("failure detected, re-initialize radio\n");
		if(rf24Radio_initiateRadio()!= 0){
			error("re-initialize radio failed, exiting\n");
			exit(EXIT_FAILURE);
		}else{
			goto resend;
		}
	}
	UNLOCK();
	
	return 0;
}

int rf24Radio_send_wo_aa(uint64_t targetAddress, void* rawContent, size_t sizeToSend){
	radio->setAutoAck(0, false);
	int result = rf24Radio_send(targetAddress, rawContent, sizeToSend);
	radio->setAutoAck(0, true);
	return result;
}

int rf24Radio_hasIncoming(){
	LOCK();
	resend:
	int a = radio->available();
	
	if(radio->failureDetected){
		warn("failure detected, re-initialize radio\n");
		if(rf24Radio_initiateRadio()!= 0){
			error("re-initialize radio failed, exiting\n");
			exit(EXIT_FAILURE);
			goto resend;
		}
	}
	UNLOCK();
	return a;
}

int rf24Radio_receive(void* buffer, size_t size){
	LOCK();
	int realSize;
	if(radio->available()){
		radio->read(buffer, size);
		realSize = radio->getDynamicPayloadSize();
		#ifdef RADIO_DETAIL_DISPLAY
			fprintf(stdout, "radio receiving package: 0x");
			for(int i=0; i<realSize; i++){
				fprintf(stdout, "%02x", ((uint8_t*)buffer)[i]);
			}
			fprintf(stdout, ", %ld\n", get_nanosecond());
			fflush(stdout);
		#endif
	}else{
		realSize = -1;
	}
	
	if(radio->failureDetected){
		warn("failure detected, re-initialize radio\n");
		if(rf24Radio_initiateRadio()!= 0){
			error("re-initialize radio failed, exiting\n");
			exit(EXIT_FAILURE);
		}
	}
	UNLOCK();
	return realSize;
}