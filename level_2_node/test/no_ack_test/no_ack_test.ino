#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <RF24_config.h>

RF24 radio(9,10);

void setup(void){
  radio.begin();
  radio.setDataRate(RF24_250KBPS);
  radio.setAutoAck(false);
  radio.enableDynamicPayloads();
  radio.openReadingPipe(1, 0xF0F0F0F0F1LL);
  radio.openWritingPipe(0xF0F0F0F0F0LL);
  radio.startListening();
}

void loop(){
  if(radio.available()){
    //size_t payloadSize = radio.getDynamicPayloadSize();
    char buffer[4];
    radio.read(buffer, 4);
    //delay(50);
    radio.stopListening();
    radio.write(buffer, 4);
    radio.startListening();
  }
  delay(1);
}
