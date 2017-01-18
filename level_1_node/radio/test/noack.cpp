#include <stdio.h>
#include <unistd.h>
#include <RF24/RF24.h>

RF24 radio(RPI_V2_GPIO_P1_22, RPI_V2_GPIO_P1_24, BCM2835_SPI_SPEED_4MHZ);

bool sendTest(int input);

int main(int argc, char** argv){
	printf("started\n");
	radio.begin();
	radio.setDataRate(RF24_250KBPS);
	radio.setAutoAck(false);
	radio.enableDynamicPayloads();
	radio.openReadingPipe(1, 0xF0F0F0F0F0);
	radio.openWritingPipe(0xF0F0F0F0F1);
	radio.stopListening();
	radio.printDetails();
	printf("\nno ack test initiated\n");
	
	while(1){
		int input;
		scanf("%d", &input);
		if(input != 0){ // normal mode
			sendTest(input);
		}else{ // ping mode
			int success = 0;
			for(int i=0; i<30; i++){
				if(sendTest(0)) success++;
				usleep(50000);
			}
			printf("successfully received: %d/30\n", success);
		}
	}
}

bool sendTest(int input){
	int received;
	radio.write(&input, sizeof(int));
	printf("message send\n");
	radio.startListening();
	int i=0;
	for(; i<250; i++){
		if(radio.available()){
			radio.read(&received, sizeof(int));
			printf("message received: %d\n", received);
			break;
		}
		usleep(1000);
	}
	radio.stopListening();
	if(i == 250) return false;
	else return true;
}